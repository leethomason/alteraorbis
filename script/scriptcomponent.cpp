#include "../engine/serialize.h"

#include "../xegame/componentfactory.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"

#include "scriptcomponent.h"
#include "volcanoscript.h"
#include "plantscript.h"
#include "corescript.h"
#include "countdownscript.h"

#include "../game/lumoschitbag.h"

using namespace grinliz;


ScriptComponent::ScriptComponent( IScript* p_script ) : script( p_script ), factory( 0 )	
{
}


ScriptComponent::ScriptComponent( const ComponentFactory* f ) : script( 0 ), factory( f )
{
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
			script = new PlantScript( factory->GetSim(), factory->GetWeather(), 0 );
		}
		else if ( StrEqual( name, "CoreScript" )) {
			script = new CoreScript( factory->GetWorldMap(), factory->GetChitBag(), factory->GetEngine() );
		}
		else if ( StrEqual( name, "CountDownScript" )) {
			script = new CountDownScript( 1000 );
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
	const ChitContext* cc = GetChitContext();

	// Do NOT set these. Will stomp on the Load()
	//context.initialized = false;
	//context.time = 0;
	context.chit	= chit;
	context.census	= &chit->GetLumosChitBag()->census;
	context.chitBag = chit->GetLumosChitBag();
	context.engine	= cc->engine;
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

