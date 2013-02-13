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

#ifndef ITEMCOMPONENT_INCLUDED
#define ITEMCOMPONENT_INCLUDED

// Hack. It begins. Engine code including game code.
#include "../game/gameitem.h"

#include "itembasecomponent.h"

class ItemComponent : public ItemBaseComponent
{
private:
	typedef ItemBaseComponent super;

public:
	ItemComponent( const GameItem& _item ) : item( _item ), slowTickTimer(0) {}
	virtual ~ItemComponent() {}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "ItemComponent" ) ) return this;
		return super::ToComponent( name );
	}

	virtual void DebugStr( grinliz::GLString* str ) {
		str->Format( "[Item] %s ", item.Name() );
	}

	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );
	virtual void Serialize( DBItem item );

	virtual int DoTick( U32 delta, U32 since );
	virtual void OnChitEvent( const ChitEvent& event );

	GameItem* GetItem() { return &item; }
	void EmitEffect( Engine* engine, U32 deltaTime );

private:
	void DoSlowTick();

	GameItem item;
	int slowTickTimer;
};

#endif // ITEMCOMPONENT_INCLUDED