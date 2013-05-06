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
class Census;

struct ScriptContext
{
	ScriptContext() : initialized(false), lastTime( 0 ), time( 0 ), chit( 0 ), census( 0 ) {}

	bool	initialized;
	U32		lastTime;		// time at last tick
	U32		time;			// time at this tick
	Chit*	chit;			// null at load
	Census* census;			// valid at load
};


class IScript
{
public:
	// The first time this is turned on:
	virtual void Init( const ScriptContext& heap )	= 0;
	virtual void OnAdd( const ScriptContext& ctx ) = 0;
	virtual void OnRemove( const ScriptContext& ctx ) = 0;
	virtual void Serialize( const ScriptContext& ctx, XStream* xs )	= 0;
	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since ) = 0;
	virtual const char* ScriptName() = 0;
};


class ScriptComponent : public Component
{
private:
	typedef Component super;

public:
	ScriptComponent( IScript* p_script, Census* p_census ) : script( p_script ), factory( 0 )	{
		context.census = p_census;
	}
	ScriptComponent( const ComponentFactory* f, Census* p_census );
	virtual ~ScriptComponent()									{ delete script; }

	virtual const char* Name() const { return "ScriptComponent"; }
	virtual ScriptComponent* ToScriptComponent() { return this; }

	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void DebugStr( grinliz::GLString* str )		{ str->Format( "[Script] " ); }
	virtual int DoTick( U32 delta, U32 since );

	// Obviously dangerous; used for casting.	
	IScript* Script() { return script; }
private:
	ScriptContext context;
	IScript* script;
	const ComponentFactory* factory;
};

#endif // VOLCANO_COMPONENT_INCLUDED
