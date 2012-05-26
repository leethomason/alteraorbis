#ifndef XEITEM_INCLUDED
#define XEITEM_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"

class GameItem;

//	XEItem
//		Item
//			WeaponItem
//				MeleeWeaponItem
//				RangedWeaponItem
//			ShieldItem
//	may regret this: part of component system? sibling system??

class XEItem
{
public:
	XEItem() : resource(0)	{}
	XEItem( const char* _name, const char* _resource ) : name( _name ), resource( _resource ) {}
	virtual ~XEItem()	{}

	const char* name;
	const char* resource;

	virtual GameItem* ToGameItem() { return 0; }
};

#endif // XEITEM_INCLUDED