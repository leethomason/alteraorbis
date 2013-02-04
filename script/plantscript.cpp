#include "plantscript.h"
#include "itemscript.h"

#include "../engine/serialize.h"
#include "../engine/model.h"

#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"

#include "../grinliz/glstringutil.h"

#include "../game/weather.h"

using namespace grinliz;


/*static*/ GameItem* PlantScript::IsPlant( Chit* chit, int* type, int* stage )
{
	GameItem* item = chit->GetItem();
	ScriptComponent* sc = GET_COMPONENT( chit, ScriptComponent );

	if ( item && sc && StrEqual( sc->Script()->ScriptName(), "PlantScript" )) {
		PlantScript* plantScript = static_cast< PlantScript* >( sc->Script() );
		*type = plantScript->Type();
		*stage = plantScript->Stage();
		return item;
	}
	return 0;
}


PlantScript::PlantScript( Engine* p_engine, WorldMap* p_map, Weather* p_weather, int p_type ) : 
	engine( p_engine ), 
	worldMap( p_map ), 
	weather( p_weather ),
	type( p_type )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
	timer = 0;
}


void PlantScript::SetRenderComponent( Chit* chit )
{
	CStr<10> str = "plant0.0";
	str[5] = '0' + type;
	str[7] = '0' + stage;

	if ( chit->GetRenderComponent() ) {
		const ModelResource* res = chit->GetRenderComponent()->MainResource();
		if ( res && res->header.name == str.c_str() ) {
			// No change!
			return;
		}
	}

	if ( chit->GetRenderComponent() ) {
		chit->Remove( chit->GetRenderComponent() );
	}

	chit->Add( new RenderComponent( engine, str.c_str() ));

	// Set the mass to be consistent with rendering.
	GameItem* item = chit->GetItem();
	GLASSERT( item );
	if ( item ) {
		item->mass = resource->mass * (float)((stage+1)*(stage+1));
	}
}


void PlantScript::Init( const ScriptContext& ctx )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();

	CStr<10> str = "plant0.0";
	str[5] = '0' + type;
	resource = &itemDefDB->Get( str.c_str() );

	ctx.chit->Add( new ItemComponent( *resource ));
	SetRenderComponent( ctx.chit );
}


void PlantScript::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( type );
	XE_ARCHIVE( stage );
	XE_ARCHIVE( age );
	XE_ARCHIVE( ageAtStage );
}


void PlantScript::Load( const ScriptContext& ctx, const tinyxml2::XMLElement* element )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
	timer = 0;
	GLASSERT( element );
	Archive( 0, element );
}


void PlantScript::Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "PlantScript" );
	Archive( printer, 0 );
	printer->CloseElement();
}


bool PlantScript::DoTick( const ScriptContext& ctx, U32 delta )
{
	static const float HP_PER_TICK = 10.0f;
	static const int   TIME_TO_GROW = 4 * (1000 * 60);	// minutes

	timer += delta;
	if ( timer < 1000 ) return true;
	timer -= 1000;
	age += 1000;
	ageAtStage += 1000;

	SpatialComponent* sc = ctx.chit->GetSpatialComponent();
	GLASSERT( sc );
	if ( !sc ) return false;

	GameItem* item = ctx.chit->GetItem();
	GLASSERT( item );
	if ( !item ) return false;

	Vector2F posF = sc->GetPosition2D();
	Vector2I pos = { (int)posF.x, (int)posF.y };

	float h = (float)(stage+1);

	float rainFraction	= weather->RainFraction( pos.x, pos.y );	// FIXME: switch to actual "isRaining"
	float sunHeight		= h;												// FIXME: account for shadows
	float sun			= sunHeight * (1.0f-rainFraction);

	// FIXME: account for pools, sea edge
	// FIXME: account for nearby rock limiting root depth
	float water = rainFraction * h;				// h also models root depth

	float sunScale = 0.5f;
	item->GetValue( "sun", &sunScale );

	float sunNeeded   = h * sunScale;
	float waterNeeded = h * (1.0f-sunScale);

	float temp = weather->Temperature( pos.x, pos.y );
	float plantTemp = 0.5f;
	item->GetValue( "temperature", &plantTemp );
	float tempError = 1.0f - fabsf( plantTemp - temp );

	sun   *= tempError;
	water *= tempError;

	if ( sun > sunNeeded && water > waterNeeded ) {
		// Heal.
		ChitMsg healMsg( ChitMsg::CHIT_HEAL );
		healMsg.dataF = HP_PER_TICK;
		ctx.chit->SendMessage( healMsg );
		
		// Grow
		if (    item->HPFraction() > 0.9f 
			 && ageAtStage > TIME_TO_GROW 
			 && stage < 3 ) 
		{
			++stage;
			ageAtStage = 0;
			SetRenderComponent( ctx.chit );
		}
	}
	else if ( sun < sunNeeded * 0.5f && water < waterNeeded * 0.5f ) {
		DamageDesc dd( HP_PER_TICK, 0 );
		ChitMsg damage( ChitMsg::CHIT_DAMAGE, 0, &dd );
		ctx.chit->SendMessage( damage );
	}

	// If healthy create other plants.

	return true;
}


