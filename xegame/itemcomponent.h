#ifndef ITEMCOMPONENT_INCLUDED
#define ITEMCOMPONENT_INCLUDED

#include "component.h"

// Hack. It begins. Engine code including game code.
#include "../game/gameitem.h"

class ItemComponent : public Component
{
public:
	ItemComponent( const GameItem& _item ) : item( _item ) {}
	virtual ~ItemComponent() {}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "ItemComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual void DebugStr( grinliz::GLString* str ) {
		str->Format( "[Item] %s ", item.Name() );
	}

	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );

	virtual bool DoTick( U32 delta );

	GameItem* GetItem() { return &item; }
	GameItem item;
};

#endif // ITEMCOMPONENT_INCLUDED