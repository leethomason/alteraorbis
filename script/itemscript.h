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

#ifndef ITEM_SCRIPT_INCLUDED
#define ITEM_SCRIPT_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

#include "../game/gameitem.h"


class ItemDefDB
{
public:
	static ItemDefDB* Instance() { if ( !instance ) instance = new ItemDefDB(); return instance; }
	~ItemDefDB()	{ instance = 0; }

	void Load( const char* path );

	bool Has( const char* name );
	const GameItem& Get( const char* name, int intrinsic=-1 );

	typedef grinliz::CArray<GameItem*, 16> GameItemArr;
	// Query an item and all its intrinsics; main item is
	// index 0, intrinsic items follow.
	void Get( const char* name, GameItemArr* arr );

	// Get the 'value' of the property 'prop' from the item with 'name'
	static void GetProperty( const char* name, const char* prop, int* value );

	void DumpWeaponStats();
	void AssignWeaponStats( const int* roll, const GameItem& base, GameItem* item );

private:
	ItemDefDB()		{}

	static ItemDefDB* instance;
	GameItem nullItem;

	grinliz::HashTable< const char*, GameItem*, grinliz::CompCharPtr, grinliz::OwnedPtrSem > map;
	// Names of all the items in the DefDB - "top" because "cyclops" is in the list, but not "cyclops claw"
	grinliz::CDynArray< grinliz::IString > topNames;
};

// Needs to be small - lots of these to save.
struct ItemHistory
{
	bool operator<(const ItemHistory& rhs) const { return this->itemID < rhs.itemID; }
	bool operator==(const ItemHistory& rhs) const { return this->itemID == rhs.itemID; }

	int itemID;
	grinliz::IString name;			// echos GameItem:	blaster
	grinliz::IString properName;	//					Hruntar
	grinliz::IString desc;			//					Hruntar a superb blaster

	void Set( const GameItem* );
	void Serialize( XStream* xs );
};

/*
	As of this point, GameItems are owned by Chits. (Specifically ItemComponents). And
	occasionally other places. I'm questioning this design. However, we need to be
	able to look up items for history and such. It either is in use, or deleted, and
	we need a record of what it was.
*/
class ItemDB
{
public:
	ItemDB()	{ GLASSERT( instance == 0 ); instance = this; }
	~ItemDB()	{ GLASSERT( instance ); instance = 0; }

	static ItemDB* Instance() { return instance; }

	void Add( const GameItem* );
	void Update( const GameItem* );
	void Remove( const GameItem* );

	// Find an item by id; if null, can use History
	const GameItem*		Find( int id );
	const ItemHistory*	History( int id );

	void Serialize( XStream* xs );

private:
	static ItemDB* instance;
	grinliz::HashTable< int, const GameItem* > itemMap;	// map of all the active, allocated items.
	grinliz::SortedDynArray< ItemHistory > itemHistory;      // all the items ever created. id is the index; may need to compress in the future.
};




#endif // ITEM_SCRIPT_INCLUDED