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
#include "../xegame/stackedsingleton.h"

class NewsHistory;

class ItemDefDB : public StackedSingleton< ItemDefDB >
{
public:
	ItemDefDB()		{ PushInstance( this ); }
	~ItemDefDB()	{ PopInstance( this ); }

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

	const grinliz::CDynArray< grinliz::IString >& GreaterMOBs() const { return greaterMOBs; }
	const grinliz::CDynArray< grinliz::IString >& LesserMOBs() const  { return lesserMOBs; }

private:
	GameItem nullItem;

	grinliz::HashTable< const char*, GameItem*, grinliz::CompCharPtr, grinliz::OwnedPtrSem > map;
	// Names of all the items in the DefDB - "top" because "cyclops" is in the list, but not "cyclops claw"
	grinliz::CDynArray< grinliz::IString > topNames;
	grinliz::CDynArray< grinliz::IString > greaterMOBs, lesserMOBs;
};

// Needs to be small - lots of these to save.
struct ItemHistory
{
	ItemHistory() : itemID(0), level(0), value(0), kills(0), greater(0), crafted(0), score(0) {}

	bool operator<(const ItemHistory& rhs) const { return this->itemID < rhs.itemID; }
	bool operator==(const ItemHistory& rhs) const { return this->itemID == rhs.itemID; }

	int					itemID;
	grinliz::IString	titledName;
	U16					level;
	U16					value;
	U16					kills;
	U16					greater;
	U16					crafted;
	U16					score;

	void Set( const GameItem* );
	void Serialize( XStream* xs );
	void AppendDesc( grinliz::GLString* str, NewsHistory* history );
};

/*
	As of this point, GameItems are owned by Chits. (Specifically ItemComponents). And
	occasionally other places. I'm questioning this design. However, we need to be
	able to look up items for history and such. It either is in use, or deleted, and
	we need a record of what it was.
*/
class ItemDB : public StackedSingleton< ItemDB >
{
public:
	ItemDB()	{ PushInstance( this ); }
	~ItemDB()	{ PopInstance( this );; }

	void Add( const GameItem* );		// start tracking (possibly insignificant)
	void Update( const GameItem* );		// item changed
	void Remove( const GameItem* );		// stop tracking

	// Find an item by id; if null, can use History
	const GameItem*		Find( int id );
	const ItemHistory*	History( int id );

	void Serialize( XStream* xs );

	int NumHistory() const { return itemHistory.Size(); }
	const ItemHistory& HistoryByIndex( int i ) { return itemHistory[i]; }

private:
	grinliz::HashTable< int, const GameItem* > itemMap;	// map of all the active, allocated items.
	grinliz::SortedDynArray< ItemHistory > itemHistory; // all the significant items ever created
};




#endif // ITEM_SCRIPT_INCLUDED