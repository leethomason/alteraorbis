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
#include "../game//lumoschitbag.h"

using namespace grinliz;


/*static*/ GameItem* PlantScript::IsPlant( Chit* chit, int* type, int* stage )
{
	GLASSERT( chit );
	GameItem* item = chit->GetItem();
	ScriptComponent* sc = chit->GetScriptComponent();

	if ( item && sc && StrEqual( sc->Script()->ScriptName(), "PlantScript" )) {
		GLASSERT( sc->Script() );
		PlantScript* plantScript = static_cast< PlantScript* >( sc->Script() );
		if ( type )
			*type = plantScript->Type();
		if ( stage )
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


void PlantScript::SetRenderComponent()
{
	CStr<10> str = "plant0.0";
	str[5] = '0' + type;
	str[7] = '0' + stage;

	GLASSERT( scriptContext->chit );
	if ( scriptContext->chit->GetRenderComponent() ) {
		const ModelResource* res = scriptContext->chit->GetRenderComponent()->MainResource();
		if ( res && res->header.name == str.c_str() ) {
			// No change!
			return;
		}
	}

	if ( scriptContext->chit->GetRenderComponent() ) {
		RenderComponent* rc = scriptContext->chit->GetRenderComponent();
		const char* name = rc->MainResource()->Name();
		GLASSERT( strlen( name ) == 8 );
		int t = name[5] - '0';
		int s = name[7] - '0';
		scriptContext->census->plants[t][s] -= 1;

		scriptContext->chit->Remove( rc );
		delete rc;
	}

	if ( !scriptContext->chit->GetRenderComponent() ) {
		RenderComponent* rc = new RenderComponent( engine, str.c_str() );
		rc->SetSerialize( false );
		scriptContext->chit->Add( rc );

		scriptContext->census->plants[type][stage] += 1;
	}
	GameItem* item = scriptContext->chit->GetItem();
	scriptContext->chit->GetRenderComponent()->SetSaturation( item->HPFraction() );
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


void PlantScript::Init()
{
	const GameItem* resource = GetResource();
	scriptContext->chit->Add( new ItemComponent( engine, worldMap, *resource ));
	scriptContext->chit->GetItem()->traits.Roll( scriptContext->chit->random.Rand() );
	SetRenderComponent();

	scriptContext->chit->GetSpatialComponent()->SetYRotation( (float)scriptContext->chit->random.Rand( 360 ));
}


void PlantScript::Serialize(  XStream* xs )
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


int PlantScript::DoTick( U32 delta, U32 since )
{
	static const float	HP_PER_SECOND = 1.0f;
	static const int	TIME_TO_GROW  = 4 * (1000 * 60);	// minutes
	static const int	TIME_TO_SPORE = 1 * (1000 * 60); 

	// Need to generate when the PlantScript loads. (It doesn't save
	// the render component.) This is over-checking, but currently
	// don't have an onAdd.
	SetRenderComponent();

	age += since;
	ageAtStage += since;

	const int tick = MINUTE + scriptContext->chit->random.Rand( 1024*16 );

	growTimer += since;
	if ( growTimer < MINUTE ) 
		return tick;

	growTimer = 0;
	float seconds = (float)since / 1000.0f;

	MapSpatialComponent* sc = GET_SUB_COMPONENT( scriptContext->chit, SpatialComponent, MapSpatialComponent );
	GLASSERT( sc );
	if ( !sc ) return tick;

	GameItem* item = scriptContext->chit->GetItem();
	GLASSERT( item );
	if ( !item ) return tick;

	Vector2I pos = sc->MapPosition();
	float h = (float)(stage+1);

	float rainFraction	= weather->RainFraction( (float)pos.x+0.5f, (float)pos.y+0.5f );
	Rectangle2I bounds = worldMap->Bounds();

	// ------ Sun -------- //
	const Vector3F& light = engine->lighting.direction;
	float norm = Max( fabs( light.x ), fabs( light.z ));
	lightTap.x = LRintf( light.x / norm );
	lightTap.y = LRintf( light.z / norm );

	float sunHeight		= h * item->traits.NormalLeveledTrait( GameTrait::INT );	
	Vector2I tap = pos + lightTap;

	if ( bounds.Contains( tap )) {
		// Check for model or rock. If looking at a model, take 1/2 the
		// height of the first thing we find.
		const WorldGrid& wg = worldMap->GetWorldGrid( tap.x, tap.y );
		if ( wg.RockHeight() > 0 ) {
			sunHeight = Min( sunHeight, (float)wg.RockHeight() * 0.5f );
		}
		else {
			Rectangle2F r;
			r.Set( (float)tap.x, (float)tap.y, (float)(tap.x+1), (float)(tap.y+1) );
			CChitArray query;

			ChitAcceptAll all;
			scriptContext->chit->GetChitBag()->QuerySpatialHash( &query, r, 0, &all );
			for( int i=0; i<query.Size(); ++i ) {
				RenderComponent* rc = query[i]->GetRenderComponent();
				if ( rc ) {
					Rectangle3F aabb = rc->MainModel()->AABB();
					sunHeight = Min( h - aabb.max.y*0.5f, sunHeight );
					break;
				}
			}
		}
	}

	float sun = sunHeight * (1.0f-rainFraction) / h;
	sun = Clamp( sun, 0.0f, 1.0f );

	// ---------- Rain ------- //
	float rootDepth = h * item->traits.NormalLeveledTrait( GameTrait::DEX );

	for( int j=-1; j<=1; ++j ) {
		for( int i=-1; i<=1; ++i ) {
			tap.Set( pos.x+i, pos.y+j );
			if ( bounds.Contains( tap )) {
				const WorldGrid& wg = worldMap->GetWorldGrid( tap.x, tap.y );
				// Underground rocks limit root dethp.
				rootDepth = Min( rootDepth, (float)MAX_HEIGHT - wg.RockHeight() );
				if ( wg.IsWater() ) {
					rainFraction += 0.25f;
				}
			}
		}
	}
	float rain = Clamp( rainFraction * rootDepth / h, 0.0f, 1.0f );
	rootDepth = Clamp( rootDepth, 0.1f, h );

	// ------- Temperature ----- //
	float temp = weather->Temperature( (float)pos.x+0.5f, (float)pos.y+0.5f );

	// ------- calc ------- //
	Vector3F actual = { sun, rain, temp };
	Vector3F optimal = { 0.5f, 0.5f, 0.5f };
	
	item->keyValues.Fetch( "sun",  "f", &optimal.x );
	item->keyValues.Fetch( "rain", "f", &optimal.y );
	item->keyValues.Fetch( "temp", "f", &optimal.z );

	float distance = ( optimal - actual ).Length();

	float GROW = Lerp( 0.2f, 0.1f, (float)stage / (float)(NUM_STAGE-1) );
	float DIE  = 0.4f;

	float toughness = item->traits.Toughness();

	if ( distance < GROW * toughness ) {
		// Heal.
		ChitMsg healMsg( ChitMsg::CHIT_HEAL );
		healMsg.dataF = HP_PER_SECOND*seconds*item->traits.NormalLeveledTrait( GameTrait::STR );
		scriptContext->chit->SendMessage( healMsg );

		sporeTimer += since;
		int nStage = 4;
		item->keyValues.Fetch( "nStage", "d", &nStage );

		// Grow
		if (    item->HPFraction() > 0.9f 
			 && ageAtStage > TIME_TO_GROW 
			 && stage < (nStage-1) ) 
		{
			++stage;
			ageAtStage = 0;
			SetRenderComponent();

			// Set the mass to be consistent with rendering.
			const GameItem* resource = GetResource();
			item->mass = resource->mass * (float)((stage+1)*(stage+1));
			item->hp   = item->TotalHP() * 0.5f;

			sc->SetMode( stage < 2 ? GRID_IN_USE : GRID_BLOCKED );
		}

		// Spore
		if ( sporeTimer > TIME_TO_SPORE ) {
			sporeTimer -= TIME_TO_SPORE;
			int dx = -1 + scriptContext->chit->random.Rand(4);	// [-1,2]
			int dy = -1 + scriptContext->chit->random.Rand(3);	// [-1,1]

			sim->CreatePlant( pos.x+dx, pos.y+dy, -1 );
		}
	}
	else if ( distance > DIE * toughness ) {
		DamageDesc dd( HP_PER_SECOND * seconds, 0 );

		ChitDamageInfo info( dd );
		info.originID = scriptContext->chit->ID();
		info.awardXP  = false;
		info.isMelee  = true;
		info.isExplosion = false;
		info.originOfImpact.Zero();

		ChitMsg damage( ChitMsg::CHIT_DAMAGE, 0, &info );
		scriptContext->chit->SendMessage( damage );

		sporeTimer = 0;
	}

	scriptContext->chit->GetRenderComponent()->SetSaturation( item->HPFraction() );

	return tick;
}


