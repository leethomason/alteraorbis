/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "itemscript.h"
#include "battlemechanics.h"
#include "../engine/model.h"
#include "../engine/animation.h"
#include "../tinyxml2/tinyxml2.h"

using namespace tinyxml2;
using namespace grinliz;

/*static*/ ItemDefDB* ItemDefDB::instance = 0;

void ItemDefDB::Load( const char* path )
{
	XMLDocument doc;
	doc.LoadFile( path );
	GLASSERT( !doc.Error() );
	if ( !doc.Error() ) {
		const XMLElement* itemsEle = doc.RootElement();
		GLASSERT( itemsEle );
		for( const XMLElement* itemEle = itemsEle->FirstChildElement( "item" );
			 itemEle;
			 itemEle = itemEle->NextSiblingElement( "item" ) )
		{
			GameItem* item = new GameItem();
			item->Load( itemEle );
			item->key = item->name;
			GLASSERT( !map.Query( item->key.c_str(), 0 ));
			map.Add( item->key.c_str(), item );
			topNames.Push( item->name );

			const XMLElement* intrinsicEle = itemEle->FirstChildElement( "intrinsics" );
			if ( intrinsicEle ) {
				int nSub = 0;
				for( const XMLElement* subItemEle = intrinsicEle->FirstChildElement( "item" );
					 subItemEle;
					 subItemEle = subItemEle->NextSiblingElement( "item" ), ++nSub )
				{
					GameItem* subItem = new GameItem();
					subItem->Load( subItemEle );

					// Patch the name to make a sub-item
					GLString str;
					str.Format( "%s.%d", item->Name(), nSub );
					subItem->key = StringPool::Intern( str.c_str() );
					GLASSERT( !map.Query( subItem->key.c_str(), 0 ));
					map.Add( subItem->key.c_str(), subItem );

					item->Apply( subItem );
				}
			}
		}
	}
}


const GameItem& ItemDefDB::Get( const char* name, int intrinsic )
{
	GameItem* item=0;
	if ( intrinsic >= 0 ) {
		grinliz::CStr< MAX_ITEM_NAME > n;
		n.Format( "%s.%d", name, intrinsic );
		map.Query( n.c_str(), &item );
	}
	else {
		map.Query( name, &item );
	}
	if ( item ) {
		return *item;
	}
	GLASSERT( 0 );
	return nullItem;
}


/*static*/ void ItemDefDB::GetProperty( const char* name, const char* prop, int* value )
{
	GameItemArr arr;
	Instance()->Get( name, &arr );
	GLASSERT( arr.Size() > 0 );
	
	float v = 0;
	arr[0]->GetValue( prop, value );
}


void ItemDefDB::Get( const char* name, GameItemArr* arr )
{
	GameItem* item=0;
	map.Query( name, &item );
	GLASSERT( item );
	arr->Push( item );

	for( int i=0; true; ++i ) {
		grinliz::CStr< MAX_ITEM_NAME > n;
		n.Format( "%s.%d", name, i );
		item = 0;
		map.Query( n.c_str(), &item );

		if ( !item ) 
			break;
		arr->Push( item );
	}
}



void ItemDefDB::DumpWeaponStats()
{
	GameItem humanMale = this->Get( "humanMale" );


	FILE* fp = fopen( "weapons.txt", "w" );
	GLASSERT( fp );
	GameItem** items = map.GetValues();

	fprintf( fp, "%-25s %-5s %-5s %-5s %-5s\n", "Name", "Dam", "DPS", "C-DPS", "EffR" );
	for( int i=0; i<topNames.Size(); ++i ) {
		const char* name = topNames[i].c_str();
		GameItemArr arr;
		Get( name, &arr );

		int meleeTime = 1000;
		const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( arr[0]->ResourceName(), false );
		if ( res ) {
			IString animName = res->header.animation;
			if ( !animName.empty() ) {
				const AnimationResource* animRes = AnimationResourceManager::Instance()->GetResource( animName.c_str() );
				if ( animRes ) {
					meleeTime = animRes->Duration( ANIM_MELEE );
				}
			}
		}

		for( int j=0; j<arr.Size(); ++j ) {
			if ( arr[j]->ToMeleeWeapon() ) {

				DamageDesc dd;
				if ( j>0 ) {
					fprintf( fp, "%-12s:%-12s ", arr[0]->Name(), arr[j]->Name() );
					BattleMechanics::CalcMeleeDamage( arr[0], arr[j]->ToMeleeWeapon(), &dd );
				}
				else {			
					fprintf( fp, "%-25s ", arr[j]->Name() );
					BattleMechanics::CalcMeleeDamage( &humanMale, arr[j]->ToMeleeWeapon(), &dd );
				}
				fprintf( fp, "%5.1f %5.1f ", 
						    dd.damage, dd.damage*1000.0f/(float)meleeTime );
				fprintf( fp, "\n" ); 
			}
			if ( arr[j]->ToRangedWeapon() ) {
				float radAt1 = 1;

				if ( j>0 ) {
					fprintf( fp, "%-12s:%-12s ", arr[0]->Name(), arr[j]->Name() );
					radAt1 = BattleMechanics::ComputeRadAt1( arr[0], arr[j]->ToRangedWeapon(), false, false );
				}
				else {
					fprintf( fp, "%-25s ", arr[j]->Name() );
					radAt1 = BattleMechanics::ComputeRadAt1( &humanMale, arr[j]->ToRangedWeapon(), false, false );
				}

				float effectiveRange = BattleMechanics::EffectiveRange( radAt1 );

				float secpershot  = 1000.0f / (float)arr[j]->cooldown.Threshold();
				float csecpershot = (float)arr[j]->clipCap * 1000.0f / (float)(arr[j]->cooldown.Threshold()*arr[j]->clipCap + arr[j]->reload.Threshold()); 

				float dps  = arr[j]->rangedDamage * 0.5f * secpershot;
				float cdps = arr[j]->rangedDamage * 0.5f * csecpershot;

				fprintf( fp, "%5.1f %5.1f %5.1f %5.1f", arr[j]->rangedDamage, dps, cdps, effectiveRange );
				fprintf( fp, "\n" ); 
			}
		}
	}
	fclose( fp );
}
