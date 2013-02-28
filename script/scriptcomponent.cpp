#include "../engine/serialize.h"

#include "../xegame/componentfactory.h"

#include "scriptcomponent.h"
#include "volcanoscript.h"
#include "plantscript.h"
#include "corescript.h"

using namespace grinliz;


ScriptComponent::ScriptComponent( const ComponentFactory* f, Census* c ) : script( 0 ), factory( f )
{
	context.census = c;
}

void ScriptComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, context.initialized );
	XARC_SER( xs, context.time );

	if ( xs->Saving() ) {
		xs->Saving()->Set( "ScriptName", script->ScriptName() );
	}
	else {
		GLASSERT( !script );
		GLASSERT( factory );

		const char* name = xs->Loading()->Get( "ScriptName" )->Str();

		if ( StrEqual( name, "VolcanoScript" )) {
			script = new VolcanoScript( factory->GetWorldMap(), 0 );
		}
		else if ( StrEqual( name, "PlantScript" )) {
			script = new PlantScript( factory->GetSim(), factory->GetEngine(), factory->GetWorldMap(), factory->GetWeather(), 0 );
		}
		else if ( StrEqual( name, "CoreScript" )) {
			script = new CoreScript( factory->GetWorldMap() );
		}
		else {
			GLASSERT( 0 );
		}
		GLASSERT( script );
	}
	script->Serialize( context, xs );
	this->EndSerialize( xs );
}

	
void ScriptComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	
	// Do NOT set these. Will stomp on the Load()
	//context.initialized = false;
	//context.time = 0;
	context.chit = chit;
	script->OnAdd( context );
}


void ScriptComponent::OnRemove()
{
	script->OnRemove( context );
	super::OnRemove();
}


int ScriptComponent::DoTick( U32 delta, U32 since )
{
	if ( !context.initialized ) {
		script->Init( context );
		context.initialized = true;
	}
	int result = script->DoTick( context, delta, since );
	context.time += since;
	return result;
}

