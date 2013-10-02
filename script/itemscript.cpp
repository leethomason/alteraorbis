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


int ItemDefDB::CalcItemValue( const GameItem* item )
{
	static const float EFFECT_BONUS = 1.5f;
	static const float MELEE_VALUE  = 20;
	static const float RANGED_VALUE = 30;
	static const float SHIELD_VALUE = 20;

	CArray<int, 16> value;
	value.Push( 10 );

	if ( item->ToMeleeWeapon() ) {
		float dptu = BattleMechanics::MeleeDPTU( 0, item->ToMeleeWeapon() );

		const GameItem& basic = Get( "ring" );
		float refDPTU = BattleMechanics::MeleeDPTU( 0, basic.ToMeleeWeapon() );

		float v = dptu / refDPTU * MELEE_VALUE;
		if ( item->flags & GameItem::EFFECT_FIRE ) v *= EFFECT_BONUS;
		if ( item->flags & GameItem::EFFECT_SHOCK ) v *= EFFECT_BONUS;
		if ( item->flags & GameItem::EFFECT_EXPLOSIVE ) v *= EFFECT_BONUS;
		value.Push( (int)v );
	}

	if ( item->ToRangedWeapon() ) {
		float radAt1 = BattleMechanics::ComputeRadAt1( 0, item->ToRangedWeapon(), false, false );
		float dptu = BattleMechanics::RangedDPTU( item->ToRangedWeapon(), true );
		float er   = BattleMechanics::EffectiveRange( radAt1 );

		const GameItem& basic = Get( "blaster" );
		float refRadAt1 = BattleMechanics::ComputeRadAt1( 0, basic.ToRangedWeapon(), false, false );
		float refDPTU = BattleMechanics::RangedDPTU( basic.ToRangedWeapon(), true );
		float refER   = BattleMechanics::EffectiveRange( refRadAt1 );

		float v = ( dptu * er ) / ( refDPTU * refER ) * RANGED_VALUE;
		if ( item->flags & GameItem::EFFECT_FIRE ) v *= EFFECT_BONUS;
		if ( item->flags & GameItem::EFFECT_SHOCK ) v *= EFFECT_BONUS;
		if ( item->flags & GameItem::EFFECT_EXPLOSIVE ) v *= EFFECT_BONUS;
		value.Push( (int)v );
	}

	if ( item->ToShield() ) {
		int rounds = item->ClipCap();
		const GameItem& basic = Get( "shield" );
		int refRounds = basic.ClipCap();

		// Currently, shield effects don't do anything, although that would be cool.
		float v = float(rounds) / float(refRounds) * SHIELD_VALUE;
		value.Push( int(v) );
	}
	return value.Max();
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
				fprintf( fp, "%5.1f %5.1f ", dd.damage, BattleMechanics::MeleeDPTU( &humanMale, arr[j]->ToMeleeWeapon()));
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

				float dps  = BattleMechanics::RangedDPTU( arr[j]->ToRangedWeapon(), false );
				float cdps = BattleMechanics::RangedDPTU( arr[j]->ToRangedWeapon(), true );

				fprintf( fp, "%5.1f %5.1f %5.1f %5.1f", arr[j]->rangedDamage, dps, cdps, effectiveRange );
				fprintf( fp, "\n" ); 
			}
		}
	}
	fclose( fp );
}
