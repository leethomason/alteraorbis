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

	const GameItem& Get( const char* name, int intrinsic=-1 );

	typedef grinliz::CArray<GameItem*, 16> GameItemArr;
	// Query an item and all its intrinsics; main item is
	// index 0, intrinsic items follow.
	void Get( const char* name, GameItemArr* arr );

	// Get the 'value' of the property 'prop' from the item with 'name'
	static void GetProperty( const char* name, const char* prop, int* value );

	void DumpWeaponStats();
	int  CalcItemValue( const GameItem* item );
	void AssignWeaponStats( const int* roll, const GameItem& base, GameItem* item );

private:
	ItemDefDB()		{}

	static ItemDefDB* instance;
	GameItem nullItem;

	grinliz::HashTable< const char*, GameItem*, grinliz::CompCharPtr, grinliz::OwnedPtrSem > map;
	grinliz::CDynArray< grinliz::IString > topNames;
};



#endif // ITEM_SCRIPT_INCLUDED