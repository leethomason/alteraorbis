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

#ifndef XENOENGINE_COMPONENT_INCLUDED
#define XENOENGINE_COMPONENT_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glvector.h"
#include "../tinyxml2/tinyxml2.h"
#include "../xarchive/glstreamer.h"
#include "xegamelimits.h"

// The primary components:
class SpatialComponent;
class RenderComponent;
class MoveComponent;
class Chit;
class ChitBag;
class ChitEvent;
class ChitMsg;

class Component
{
public:
	Component() : parentChit( 0 ), id( idPool++ )	{}
	virtual ~Component()			{}

	int ID() const { return id; }

	virtual void OnAdd( Chit* chit )	{	parentChit = chit; }
	virtual void OnRemove()				{	parentChit = 0;    }

	virtual void Load( const tinyxml2::XMLElement* element ) = 0;
	virtual void Save( tinyxml2::XMLPrinter* printer ) = 0;
	virtual void Serialize( XStream* xs ) = 0;

	virtual const char* Name() const = 0;
	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, Name() ) ) return this;
		return 0;
	}

	// Utility
	Chit* ParentChit() { return parentChit; }
	Chit* GetChit( int id );

	virtual int DoTick( U32 delta, U32 since )			{ return VERY_LONG_TICK; }
	virtual void DoUpdate()								{}
	virtual void DebugStr( grinliz::GLString* str )		{}

	virtual void OnChitEvent( const ChitEvent& event )			{}
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );

protected:
	float Travel( float rate, U32 time ) const {
		return rate * ((float)time) * 0.001f;
	}

	void BeginSave( tinyxml2::XMLPrinter* printer, const char* name );
	void EndSave( tinyxml2::XMLPrinter* printer );
	void BeginLoad( const tinyxml2::XMLElement* element, const char* name ); 
	void EndLoad( const tinyxml2::XMLElement* element )	{}

	void BeginSerialize( XStream* xs, const char* name );
	void EndSerialize( XStream* sx );

	ChitBag* GetChitBag();
	Chit* parentChit;
	int id;

	static int idPool;
};


// Abstract at the XenoEngine level.
class MoveComponent : public Component
{
private:
	typedef Component super;
public:
	virtual MoveComponent*		ToMove()		{ return this; }

	virtual const char* Name() const { return "MoveComponent"; }

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );
	virtual void Serialize( XStream* xs );

	virtual bool IsMoving() const				{ return false; }
	// approximate, may lag, etc. useful for AI
	virtual void CalcVelocity( grinliz::Vector3F* v ) { v->Set( 0,0,0 ); }
};


#endif // XENOENGINE_COMPONENT_INCLUDED