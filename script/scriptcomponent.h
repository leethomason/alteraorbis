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

#if 0

class ComponentFactory;
class Census;
class CoreScript;
class PortalScript;
class Engine;
class LumosChitBag;
class PlantScript;
class EvalBuildingScript;

struct ScriptContext
{
	ScriptContext() : initialized(false), time(0), chit(0), census(0), chitBag(0), engine(0) {}

	bool	initialized;
	U32		time;			// "local to script" time. set by DoTick. For absolute time, use ChitBag::AbsTime()
	Chit*	chit;			// null at load
	Census* census;			// valid at load
	LumosChitBag* chitBag;
	Engine* engine;
};


class IScript
{
public:
	IScript() : scriptContext( 0 ) {}
	virtual ~IScript() {}

	void SetContext( const ScriptContext* context ) { scriptContext = context; }

	Chit* ParentChit() { return scriptContext->chit; }

	// The first time this is turned on:
	virtual void Init()	= 0;
	virtual void OnAdd() = 0;
	virtual void OnRemove() = 0;
	virtual void Serialize( XStream* xs )	= 0;
	virtual int DoTick( U32 delta ) = 0;
	virtual const char* ScriptName() = 0;

	// Safe casting.
	virtual CoreScript*   ToCoreScript()	{ return 0; }
	virtual PortalScript* ToPortalScript()	{ return 0; }
	virtual PlantScript*  ToPlantScript()	{ return 0; }
	virtual EvalBuildingScript* ToEvalBuildingScript() { return 0;  }

protected:
	const ScriptContext* scriptContext;
};


class ScriptComponent : public Component
{
private:
	typedef Component super;

public:
	ScriptComponent( IScript* p_script );
	ScriptComponent( const ComponentFactory* f );

	virtual ~ScriptComponent()									{ delete script; }

	virtual const char* Name() const { return "ScriptComponent"; }
	virtual ScriptComponent* ToScriptComponent() { return this; }

	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void DebugStr( grinliz::GLString* str )		{ str->AppendFormat( "[Script] %s ", script->ScriptName() ); }
	virtual int DoTick( U32 delta );

	IScript* Script() { return script; }

	static IScript* Factory( grinliz::IString name );

private:
	ScriptContext context;
	IScript* script;
	const ComponentFactory* factory;
};
#endif
#endif // VOLCANO_COMPONENT_INCLUDED
