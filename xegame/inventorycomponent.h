#ifndef INVENTORY_COMPONENT_INCLUDED
#define INVENTORY_COMPONENT_INCLUDED

#include "component.h"

#include "../grinliz/glcontainer.h"

class InventoryComponent : public Component
{
public:
	InventoryComponent()			{}
	virtual ~InventoryComponent()	{}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "InventoryComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual void DebugStr( grinliz::GLString* str );

	void AddToInventory( Chit* chit );
	void RemoveFromInventory( Chit* chit );

private:
	grinliz::CDynArray<Chit*> inventory;

};

#endif // INVENTORY_COMPONENT_INCLUDED