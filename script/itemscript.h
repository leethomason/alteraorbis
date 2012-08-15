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

#endif // ITEM_SCRIPT_INCLUDED