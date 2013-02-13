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

#ifndef VOLCANO_COMPONENT_INCLUDED
#define VOLCANO_COMPONENT_INCLUDED

#include "../xegame/component.h"

class ComponentFactory;

struct ScriptContext
{
	ScriptContext() : initialized(false), time( 0 ), chit( 0 ) {}

	bool	initialized;
	U32		lastTime;		// time at last tick
	U32		time;			// time at this tick
	Chit*	chit;
};


class IScript
{
public:
	virtual void Init( const ScriptContext& heap )	= 0;
	virtual void Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element ) = 0;
	virtual void Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer )       = 0;
	virtual void Serialize( const ScriptContext& ctx, DBItem item )	= 0;
	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since ) = 0;
	virtual const char* ScriptName() = 0;
};


class ScriptComponent : public Component
{
private:
	typedef Component super;

public:
	ScriptComponent( IScript* p_script ) : script( p_script ), factory( 0 )	{}
	ScriptComponent( const ComponentFactory* f ) : script( 0 ), factory( f )	{}
	virtual ~ScriptComponent()									{ delete script; }

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "ScriptComponent" ) ) return this;
		return super::ToComponent( name );
	}

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );
	virtual void Serialize( DBItem parent );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void DebugStr( grinliz::GLString* str )		{ str->Format( "[Script] " ); }
	virtual int DoTick( U32 delta, U32 since );

	// Obviously dangerous; used for casting.	
	IScript* Script() { return script; }
private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );

	ScriptContext context;
	IScript* script;
	const ComponentFactory* factory;
};

#endif // VOLCANO_COMPONENT_INCLUDED
