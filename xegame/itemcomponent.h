#ifndef ITEMCOMPONENT_INCLUDED
#define ITEMCOMPONENT_INCLUDED

#include "xeitem.h"

class ItemComponent : public Component
{
public:
	ItemComponent( XEItem* _item ) : item( _item ) {}
	virtual ~ItemComponent() {
		delete item;
	}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "ItemComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	XEItem* GetItem() { return item; }
	virtual void DebugStr( grinliz::GLString* str ) {
		str->Format( "[Item] %s ", item->name );
	}

private:
	XEItem* item;
};

#endif // ITEMCOMPONENT_INCLUDED