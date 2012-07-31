#ifndef XEITEM_INCLUDED
#define XEITEM_INCLUDED

#if 0
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
	XEItem() : name(0), resource(0), coolTime(0)	{}
	XEItem( const char* _name, const char* _resource ) : name( _name ), resource( _resource ), coolTime( 0 ) {}
	virtual ~XEItem()	{}

	const char* name;
	const char* resource;
	U32 coolTime;			// the (absolute) time when this item is next available for use. 

	virtual GameItem* ToGameItem() { return 0; }
};
#endif
#endif // XEITEM_INCLUDED