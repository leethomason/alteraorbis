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