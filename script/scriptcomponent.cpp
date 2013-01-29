#include "../engine/serialize.h"

#include "../xegame/componentfactory.h"

#include "scriptcomponent.h"
#include "volcanoscript.h"

using namespace grinliz;

void ScriptComponent::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( context.initialized );
	XE_ARCHIVE( context.time );
}

void ScriptComponent::Load( const tinyxml2::XMLElement* element )
{
	GLASSERT( !script );
	GLASSERT( factory );
	this->BeginLoad( element, "ScriptComponent" );
	Archive( 0, element );

	const tinyxml2::XMLElement* child = element->FirstChildElement();
	const char* name = child->Name();
	if ( StrEqual( name, "VolcanoScript" )) {
		script = new VolcanoScript( factory->GetWorldMap() );
	}
	else {
		GLASSERT( 0 );
	}
	GLASSERT( script );
	script->Load( context, child );

	//script->Load( context, element );
	this->EndLoad( element );
}


void ScriptComponent::Save( tinyxml2::XMLPrinter* printer )
{
	this->BeginSave( printer, "ScriptComponent" );
	Archive( printer, 0 );
	script->Save( context, printer );
	this->EndSave( printer );
}


void ScriptComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	context.initialized = false;
	context.time = 0;
	context.chit = chit;
}


void ScriptComponent::OnRemove()
{
	super::OnRemove();
}


bool ScriptComponent::DoTick( U32 delta )
{
	if ( !context.initialized ) {
		script->Init( context );
		context.initialized = true;
	}
	bool result = script->DoTick( context, delta );
	context.time += delta;
	return result;
}

