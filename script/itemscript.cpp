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
#include "../xegame/istringconst.h"
#include "../game/lumosmath.h"
#include "../game/news.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/cgame.h"
#include "corescript.h"

using namespace tinyxml2;
using namespace grinliz;

ItemDefDB* ItemDefDB::instance = 0;

void ItemDefDB::Load( const char* path )
{
	XMLDocument doc;
	doc.LoadFile( path );
	GLASSERT( !doc.Error() );
	if ( !doc.Error() ) {
		const XMLElement* itemsEle = doc.RootElement();
		GLASSERT( itemsEle );
		for( const XMLElement* itemEle = itemsEle->FirstChildElement();
			 itemEle;
			 itemEle = itemEle->NextSiblingElement())
		{
			GameItem* item = GameItem::Factory(itemEle->Name());
			GLASSERT(item);
			if (!item) continue;

			item->Load( itemEle );
			item->key = item->IName();
			GLASSERT( !map.Query( item->key.c_str(), 0 ));
			map.Add( item->key.c_str(), item );
			topNames.Push( item->IName() );

			IString mob = item->keyValues.GetIString( ISC::mob );
			if ( mob == ISC::greater ) {
				greaterMOBs.Push( item->IName() );
			}
			else if ( mob == ISC::lesser ) {
				lesserMOBs.Push( item->IName() );
			}

			const XMLElement* intrinsicEle = itemEle->FirstChildElement( "intrinsics" );
			if ( intrinsicEle ) {
				int nSub = 0;
				for( const XMLElement* subItemEle = intrinsicEle->FirstChildElement();
					 subItemEle;
					 subItemEle = subItemEle->NextSiblingElement(), ++nSub )
				{
					GameItem* subItem = GameItem::Factory(subItemEle->Name());
					GLASSERT(subItem);
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
	GLASSERT(item);
	return *item;
}


/*static*/ void ItemDefDB::GetProperty( const char* name, const char* prop, int* value )
{
	GameItemArr arr;
	Instance()->Get( name, &arr );
	GLASSERT( arr.Size() > 0 );
	
//	float v = 0;
	arr[0]->keyValues.Get( prop, value );
}


bool ItemDefDB::Has( const char* name )
{
	return map.Query( name, 0 );
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
	const GameItem& humanMale = this->Get( "humanMale" );

	GLString path;
	GetSystemPath(GAME_SAVE_DIR, "weapons.txt", &path);
	FILE* fp = fopen( path.c_str(), "w" );
	GLASSERT( fp );

	fprintf( fp, "%-25s %-5s %-5s %-5s %-5s %-10s\n", "Name", "Dam", "DPS", "C-DPS", "EffR", "ClosingD" );
	for( int i=0; i<topNames.Size(); ++i ) {
		const char* name = topNames[i].c_str();
		GameItemArr arr;
		Get( name, &arr );

		for( int j=0; j<arr.Size(); ++j ) {
			const MeleeWeapon* meleeWeapon = arr[j]->ToMeleeWeapon();
			const RangedWeapon* rangedWeapon = arr[j]->ToRangedWeapon();

			if (meleeWeapon) {

				DamageDesc dd;
				if ( j>0 ) {
					fprintf( fp, "%-12s:%-12s ", arr[0]->Name(), meleeWeapon->Name() );
					BattleMechanics::CalcMeleeDamage( arr[0], meleeWeapon, &dd );
				}
				else {			
					fprintf( fp, "%-25s ", meleeWeapon->Name() );
					BattleMechanics::CalcMeleeDamage( &humanMale, meleeWeapon, &dd );
				}
				fprintf( fp, "%5.1f %5.1f ", dd.damage, BattleMechanics::MeleeDPTU( &humanMale, meleeWeapon));
				fprintf( fp, "\n" ); 
			}
			if (rangedWeapon) {
				float radAt1 = 1;

				if ( j>0 ) {
					fprintf( fp, "%-12s:%-12s ", arr[0]->Name(), rangedWeapon->Name() );
					radAt1 = BattleMechanics::ComputeRadAt1( arr[0], rangedWeapon, false, false );
				}
				else {
					fprintf( fp, "%-25s ", rangedWeapon->Name() );
					radAt1 = BattleMechanics::ComputeRadAt1( &humanMale, rangedWeapon, false, false );
				}

				float effectiveRange = BattleMechanics::EffectiveRange( radAt1 );

				float dps  = BattleMechanics::RangedDPTU( rangedWeapon, false );
				float cdps = BattleMechanics::RangedDPTU( rangedWeapon, true );
				
				float time = effectiveRange / DEFAULT_MOVE_SPEED;
				float closingD = cdps * time;

				fprintf( fp, "%5.1f %5.1f %5.1f %5.1f %8.1f", rangedWeapon->Damage(), dps, cdps, effectiveRange, closingD );
				fprintf( fp, "\n" ); 
			}
		}
	}
	fclose( fp );
}


void ItemHistory::Set( const GameItem* gi )
{
	this->itemID		= gi->ID();
	this->titledName	= gi->INameAndTitle();
	this->level			= gi->Traits().Level();
	this->value			= gi->GetValue();

	int k = 0, c = 0, g = 0, s = 0;

	gi->historyDB.Get(ISC::Kills, &k);
	gi->historyDB.Get(ISC::Greater, &g);
	gi->historyDB.Get(ISC::Crafted, &c);
	gi->keyValues.Get(ISC::score, &s);

	this->kills = k;
	this->greater = g;
	this->crafted = c;
	this->score = s;
}

void ItemHistory::Serialize( XStream* xs )
{
	int aLevel = level;	// U16 to int conversion, and back.
	int aValue = value;
	int aKills = kills;
	int aCrafted = crafted;
	int aGreater = greater;
	int aScore = score;

	XarcOpen( xs, "ItemHistory" );
	XARC_SER( xs, itemID );
	XARC_SER( xs, titledName );
	XARC_SER( xs, aLevel );
	XARC_SER( xs, aValue );
	XARC_SER( xs, aKills );
	XARC_SER( xs, aGreater );
	XARC_SER(xs, aCrafted);
	XARC_SER(xs, aScore);
	XarcClose( xs );

	level = aLevel;
	value = aValue;
	kills = aKills;
	greater = aGreater;
	crafted = aCrafted;
	score = aScore;
}


void ItemHistory::AppendDesc( GLString* str, NewsHistory* history, const char* separator ) const
{
	if ( !itemID ) {
		str->append( "(none)" );
	}
	else {
		GLASSERT( !titledName.empty() );
		CStr<16> idStr = "";
#if 0
		idStr.Format("[%d]", itemID);
#endif
		if (score)
			str->AppendFormat("%s %s %s ", CoreAchievement::CivTechDescription(score), titledName.c_str(), idStr.c_str());
		else
			str->AppendFormat( "%s %s ", titledName.c_str(), idStr.c_str() );
		
		if (separator)
			str->append(separator);

		if ( level )
			str->AppendFormat( "Level=%d ", level );
		if ( value )
			str->AppendFormat( "Value=%d ", value );
		if ( kills )
			str->AppendFormat( "%s=%d ", MOB_KILLS, kills );
		if ( greater ) 
			str->AppendFormat( "Greater=%d ", greater );
		if ( crafted )
			str->AppendFormat( "Crafted=%d ", crafted );
		if (score)
			str->AppendFormat("Score=%d ", score);

		if (history) {
			NewsHistory::Data data;
			history->FindItem(itemID, 0, 0, &data);
			//GLASSERT(data.bornOrNamed);	// lessers don't have create/destroy dates.

			if (data.born) {
				if (data.died) {
					str->AppendFormat("(%s%.2f %s%.2f) ", 
									  DATE_ABBR_BORN, double(data.born) / double(AGE_IN_MSEC), 
									  DATE_ABBR_DIED, double(data.died) / double(AGE_IN_MSEC));
				}
				else {
					str->AppendFormat("(%s%.2f) ", DATE_ABBR_BORN, double(data.born) / double(AGE_IN_MSEC));
				}
			}
		}
	}
}


ItemDB* ItemDB::instance = 0;

void ItemDB::Serialize( XStream* xs ) 
{
	XarcOpen( xs, "ItemDB" );
	// Don't serialize 'itemMap'. It is for active
	// game items and is created on demand.
	// FIXME: serialize hashtable general solution?

	if (xs->Saving()) {
		int size = itemRegistry.Size();
		XARC_SER_KEY(xs, "itemRegistry.size", size);
		for (int i = 0; i < itemRegistry.Size(); ++i) {
			ItemHistory h = itemRegistry.GetValue(i);
			h.Serialize(xs);
		}
	}
	else {
		int size = 0;
		XARC_SER_KEY(xs, "itemRegistry.size", size);
		for (int i = 0; i < size; ++i) {
			ItemHistory h;
			h.Serialize(xs);
			itemRegistry.Add(h.itemID, h);
		}
	}
	XarcClose( xs );
}

void ItemDB::Add( const GameItem* gi )
{
	int id = gi->ID();
	GLASSERT( id >= 0 );

	// History
	if ( gi->Significant() ) {
		ItemHistory h;
		h.Set( gi );
		itemRegistry.Add(h.itemID, h);
	}

	// Current
	itemMap.Add( id, gi );
}


void ItemDB::Remove( const GameItem* gi )
{
	int id = gi->ID();
	GLASSERT( id >= 0 );

	// History:
	if ( gi->Significant() ) {
		ItemHistory h;
		h.Set( gi );
		itemRegistry.Add(h.itemID, h);
	}

	// Current:
	itemMap.Remove( id );
}


void ItemDB::Update( const GameItem* gi )
{
	int id = gi->ID();
	GLASSERT( id >= 0 );

	if ( gi->Significant() ) {
		ItemHistory h;
		h.Set( gi );
		itemRegistry.Add(h.itemID, h);
	}
}


const GameItem*	ItemDB::Active( int id )
{
	const GameItem* gi = 0;
	itemMap.Query( id, &gi );
	return gi;
}

bool ItemDB::Registry( int id, ItemHistory* h )
{
	return itemRegistry.Query(id, h);
}

