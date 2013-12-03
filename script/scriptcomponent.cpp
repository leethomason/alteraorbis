#include "../engine/serialize.h"

#include "../xegame/componentfactory.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"

#include "scriptcomponent.h"
#include "volcanoscript.h"
#include "plantscript.h"
#include "corescript.h"

using namespace grinliz;


ScriptComponent::ScriptComponent( IScript* p_script, Engine* engine, Census* p_census ) : script( p_script ), factory( 0 )	
{
	context.census = p_census;
	context.engine = engine;
	GLASSERT( context.engine );
}


ScriptComponent::ScriptComponent( const ComponentFactory* f, Census* c ) : script( 0 ), factory( f )
{
	context.census = c;
	context.engine = f->GetEngine();
	GLASSERT( context.engine );
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

		const StreamReader::Attribute* attr = xs->Loading()->Get( "ScriptName" );
		GLASSERT( attr );
		const char* name = xs->Loading()->Value( attr, 0 );

		if ( StrEqual( name, "VolcanoScript" )) {
			script = new VolcanoScript( factory->GetWorldMap(), 0 );
		}
		else if ( StrEqual( name, "PlantScript" )) {
			script = new PlantScript( factory->GetSim(), factory->GetEngine(), factory->GetWorldMap(), factory->GetWeather(), 0 );
		}
		else if ( StrEqual( name, "CoreScript" )) {
			script = new CoreScript( factory->GetWorldMap(), factory->GetChitBag(), factory->GetEngine() );
		}
		else {
			GLASSERT( 0 );
		}
		GLASSERT( script );
	}
	script->Serialize( xs );
	this->EndSerialize( xs );
}

	
void ScriptComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	
	// Do NOT set these. Will stomp on the Load()
	//context.initialized = false;
	//context.time = 0;
	context.chit = chit;
	context.chitBag = chit->GetChitBag()->ToLumos();
	script->SetContext( &context );
	script->OnAdd();
}


void ScriptComponent::OnRemove()
{
	script->OnRemove();
	super::OnRemove();
}


int ScriptComponent::DoTick( U32 delta )
{
	if ( !context.initialized ) {
		script->SetContext( &context );
		script->Init();
		context.initialized = true;
	}
	int result = script->DoTick( delta );
	context.time += delta;
	return result;
}

