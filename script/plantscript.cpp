#include "plantscript.h"
#include "itemscript.h"

#include "../engine/serialize.h"
#include "../engine/model.h"
#include "../engine/engine.h"

#include "../xegame/itemcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

#include "../grinliz/glstringutil.h"

#include "../game/weather.h"
#include "../game/worldmap.h"
#include "../game/sim.h"
#include "../game/mapspatialcomponent.h"

using namespace grinliz;


/*static*/ GameItem* PlantScript::IsPlant( Chit* chit, int* type, int* stage )
{
	GLASSERT( chit );
	GameItem* item = chit->GetItem();
	ScriptComponent* sc = GET_COMPONENT( chit, ScriptComponent );

	if ( item && sc && StrEqual( sc->Script()->ScriptName(), "PlantScript" )) {
		GLASSERT( sc->Script() );
		PlantScript* plantScript = static_cast< PlantScript* >( sc->Script() );
		*type = plantScript->Type();
		*stage = plantScript->Stage();
		return item;
	}
	return 0;
}


PlantScript::PlantScript( Sim* p_sim, Engine* p_engine, WorldMap* p_map, Weather* p_weather, int p_type ) : 
	sim( p_sim ),
	engine( p_engine ), 
	worldMap( p_map ), 
	weather( p_weather ),
	type( p_type )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
	growTimer = sporeTimer = 0;
}


void PlantScript::SetRenderComponent( Chit* chit )
{
	CStr<10> str = "plant0.0";
	str[5] = '0' + type;
	str[7] = '0' + stage;

	GLASSERT( chit );
	if ( chit->GetRenderComponent() ) {
		const ModelResource* res = chit->GetRenderComponent()->MainResource();
		if ( res && res->header.name == str.c_str() ) {
			// No change!
			return;
		}
	}

	if ( chit->GetRenderComponent() ) {
		RenderComponent* rc = chit->GetRenderComponent();
		chit->Remove( rc );
		delete rc;
	}

	if ( !chit->GetRenderComponent() ) {
		chit->Add( new RenderComponent( engine, str.c_str() ));
	}
}


const GameItem* PlantScript::GetResource()
{
	CStr<10> str = "plant0";
	str[5] = '0' + type;

	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	const GameItem* resource = &itemDefDB->Get( str.c_str() );
	GLASSERT( resource );
	return resource;
}


void PlantScript::Init( const ScriptContext& ctx )
{
	const GameItem* resource = GetResource();
	ctx.chit->Add( new ItemComponent( *resource ));
	SetRenderComponent( ctx.chit );

	const Vector3F& light = engine->lighting.direction;
	float norm = Max( fabs( light.x ), fabs( light.z ));
	lightTap.x = LRintf( light.x / norm );
	lightTap.y = LRintf( light.z / norm );
	lightTapYMult = sqrtf( light.x*light.x + light.z*light.z ) / light.y;
}


void PlantScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XARC_SER( xs, type );
	XARC_SER( xs, stage );
	XARC_SER( xs, age );
	XARC_SER( xs, ageAtStage );
	XARC_SER( xs, growTimer );
	XARC_SER( xs, sporeTimer );
	XarcClose( xs );
}


int PlantScript::DoTick( const ScriptContext& ctx, U32 delta, U32 since )
{
	static const float	HP_PER_SECOND = 10.0f;
	static const int	TIME_TO_GROW  = 4 * (1000 * 60);	// minutes
	static const int	TIME_TO_SPORE = 1 * (1000 * 60); 

	age += since;
	ageAtStage += since;

	const int tick = MINUTE + ctx.chit->random.Rand( 1024*16 );

	growTimer += since;
	if ( growTimer < MINUTE ) 
		return tick;

	growTimer = 0;
	float seconds = (float)since / 1000.0f;

	MapSpatialComponent* sc = GET_COMPONENT( ctx.chit, MapSpatialComponent );
	GLASSERT( sc );
	if ( !sc ) return tick;

	GameItem* item = ctx.chit->GetItem();
	GLASSERT( item );
	if ( !item ) return tick;

	Vector2I pos = sc->MapPosition();

	float h = (float)(stage+1);

	float rainFraction	= weather->RainFraction( pos.x, pos.y );

	// rain,sun = [0,1]

	// Shadows.
	

	// FIXME: account for shadows
	float sunHeight		= h;												
	float sun			= sunHeight * (1.0f-rainFraction) / h;

	// FIXME: account for pools, sea edge
	// FIXME: account for nearby rock limiting root depth
	// FIXME: model root depth
	float rain = rainFraction; // * maxdepth / depth

	float temp = weather->Temperature( pos.x, pos.y );

	Vector3F actual = { sun, rain, temp };
	Vector3F optimal = { 0.5f, 0.5f, 0.5f };

	item->GetValue( "sun", &optimal.x );
	item->GetValue( "rain", &optimal.y );
	item->GetValue( "temp", &optimal.z );

	float distance = ( optimal - actual ).Length();

	// FIXME adjust distance for size.
	// FIXME scale hp for size2

	if ( distance < 0.4f ) {
		// Heal.
		ChitMsg healMsg( ChitMsg::CHIT_HEAL );
		healMsg.dataF = HP_PER_SECOND*seconds;
		ctx.chit->SendMessage( healMsg );

		sporeTimer += 1000;
		int nStage = 4;
		item->GetValue( "nStage", &nStage );

		// Grow
		if (    item->HPFraction() > 0.9f 
			 && ageAtStage > TIME_TO_GROW 
			 && stage < (nStage-1) ) 
		{
			++stage;
			ageAtStage = 0;
			SetRenderComponent( ctx.chit );

			// Set the mass to be consistent with rendering.
			const GameItem* resource = GetResource();
			item->mass = resource->mass * (float)((stage+1)*(stage+1));
			item->hp   = item->TotalHP() * 0.5f;

			sc->SetMode( stage < 2 ? MapSpatialComponent::USES_GRID : MapSpatialComponent::BLOCKS_GRID );
		}

		// Spore
		if ( sporeTimer > TIME_TO_SPORE ) {
			sporeTimer -= TIME_TO_SPORE;
			int dx = ctx.chit->random.Sign();
			int dy = ctx.chit->random.Sign();

			sim->CreatePlant( pos.x+dx, pos.y+dy, -1 );
		}
	}
	else if ( distance > 0.8f ) {
		DamageDesc dd( HP_PER_SECOND * seconds, 0 );
		ChitMsg damage( ChitMsg::CHIT_DAMAGE, 0, &dd );
		ctx.chit->SendMessage( damage );

		sporeTimer = 0;
	}

	// If healthy create other plants.
	return tick;
}


