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


class ItemStorage
{
public:
	ItemStorage()	{}
	~ItemStorage()	{}

	void Load( const char* path );

	const GameItem* Get( const char* name, int intrinsic=-1 );

	typedef grinliz::CArray<GameItem*, 16> GameItemArr;
	// Query an item and all its intrinsics; main item is
	// index 0, intrinsic items follow.
	void Get( const char* name, GameItemArr* arr );

private:
	grinliz::HashTable< const char*, GameItem*, grinliz::CompCharPtr, grinliz::OwnedPtrSem > map;
};


class GunGenerator
{
public:
	// Base gun & level
	//	Features: scope, melee, clip, driver (?)
	//  Traits:   effect
	enum {
		BLASTER,
		SHOTGUN,
		ASSAULT,
		SNIPER
	};
	enum {
		SCOPE,
		MELEE
	};
	void Generate( GameItem* item, int type, int featureFlags, int effectFlags, int level, U32 seed );
};


#endif // ITEM_SCRIPT_INCLUDED