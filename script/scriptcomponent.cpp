#include "scriptcomponent.h"
#include "../engine/serialize.h"


void ScriptComponent::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( context.time );
}

void ScriptComponent::Load( const tinyxml2::XMLElement* element )
{
	this->BeginLoad( element, "ScriptComponent" );
	Archive( 0, element );
	GLASSERT( 0 );	// need factory
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
	context.chit = chit;
	context.time = 0;
	script->OnAdd( context );
}


void ScriptComponent::OnRemove()
{
	super::OnRemove();
}


bool ScriptComponent::DoTick( U32 delta )
{
	bool result = script->DoTick( context, delta );
	context.time += delta;
	return result;
}

