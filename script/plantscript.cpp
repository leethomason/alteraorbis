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

static const int	TIME_TO_GROW  = 4 * (1000 * 60);	// minutes
static const int	TIME_TO_SPORE = 3 * (1000 * 60); 

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


PlantScript::PlantScript( int p_type ) : 
	type( p_type ),
	growTimer( TIME_TO_GROW ),
	sporeTimer( TIME_TO_SPORE )
{
	stage = 0;
	age = 0;
	ageAtStage = 0;
}


void PlantScript::SetRenderComponent()
{
	CStr<10> str = "plant0.0";
	str[5] = '0' + type;
	str[7] = '0' + stage;

	int t = -1;
	int s = -1;

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
		t = name[5] - '0';
		s = name[7] - '0';

		scriptContext->chit->Remove( rc );
		delete rc;
	}

	const ChitContext* context = scriptContext->chitBag->GetContext();
	if ( !scriptContext->chit->GetRenderComponent() ) {
		RenderComponent* rc = new RenderComponent( str.c_str() );
		rc->SetSerialize( false );
		scriptContext->chit->Add( rc );
	}

	if ( t >= 0 ) {
		GLASSERT( s >= 0 );
		scriptContext->census->plants[t][s]			-= 1;
		scriptContext->census->plants[type][stage]	+= 1;
	}

	GameItem* item = scriptContext->chit->GetItem();
	scriptContext->chit->GetRenderComponent()->SetSaturation( item->HPFraction() );
}


void PlantScript::OnAdd()
{
	scriptContext->census->plants[type][stage] += 1;
}


void PlantScript::OnRemove()
{
	scriptContext->census->plants[type][stage] -= 1;
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
	const ChitContext* context = scriptContext->chitBag->GetContext();
	scriptContext->chit->Add( new ItemComponent( *resource ));
	scriptContext->chit->GetItem()->GetTraitsMutable()->Roll( scriptContext->chit->random.Rand() );
	SetRenderComponent();

	scriptContext->chit->GetSpatialComponent()->SetYRotation( (float)scriptContext->chit->random.Rand( 360 ));

	// Add some fuzz to the timers.
	growTimer.SetPeriod(  TIME_TO_GROW + scriptContext->chit->random.Rand( TIME_TO_GROW/10 ));
	sporeTimer.SetPeriod( TIME_TO_SPORE + scriptContext->chit->random.Rand( TIME_TO_SPORE/10 ));
}


void PlantScript::Serialize(  XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XARC_SER( xs, type );
	XARC_SER( xs, stage );
	XARC_SER( xs, age );
	XARC_SER( xs, ageAtStage );
	growTimer.Serialize( xs, "growTimer" );
	sporeTimer.Serialize( xs, "sporeTimer" );
	XarcClose( xs );
}


int PlantScript::DoTick( U32 delta )
{
	static const float	HP_PER_SECOND = 1.0f;

	// Need to generate when the PlantScript loads. (It doesn't save
	// the render component.) This is over-checking, but currently
	// don't have an onAdd.
	SetRenderComponent();

	age += delta;
	ageAtStage += delta;

	int grow = growTimer.Delta( delta );
	int spore = sporeTimer.Delta( delta );
	int tick = Min( growTimer.Next(), sporeTimer.Next() );

	if ( !grow && !spore ) {
		if ( scriptContext->chit->GetRenderComponent() && scriptContext->chit->GetItem() ) {
			scriptContext->chit->GetRenderComponent()->SetSaturation( scriptContext->chit->GetItem()->HPFraction() );
		}
		return tick;
	}

	MapSpatialComponent* sc = GET_SUB_COMPONENT( scriptContext->chit, SpatialComponent, MapSpatialComponent );
	GLASSERT( sc );
	if ( !sc ) return tick;

	GameItem* item = scriptContext->chit->GetItem();
	GLASSERT( item );
	if ( !item ) return tick;

	Vector2I pos = sc->MapPosition();
	Vector2F pos2f = sc->GetPosition2D();
	const ChitContext* context = scriptContext->chitBag->GetContext();
	Weather* weather = Weather::Instance();
	Rectangle2I bounds = context->worldMap->Bounds();

	int nStage = 4;
	item->keyValues.Fetch( "nStage", "d", &nStage );

	if ( grow ) {
		// ------ Sun -------- //
		const float		h				= (float)(stage+1);
		const float		rainFraction	= weather->RainFraction( pos2f.x, pos2f.y );
		const Vector3F& light			= context->engine->lighting.direction;
		const float		norm			= Max( fabs( light.x ), fabs( light.z ));
		lightTap.x = LRintf( light.x / norm );
		lightTap.y = LRintf( light.z / norm );

		float sunHeight			= h;	
		Vector2I tap = pos + lightTap;

		if ( bounds.Contains( tap )) {
			// Check for model or rock. If looking at a model, take 1/2 the
			// height of the first thing we find.
			const WorldGrid& wg = context->worldMap->GetWorldGrid( tap.x, tap.y );
			if ( wg.RockHeight() > 0 ) {
				sunHeight = Min( sunHeight, (float)wg.RockHeight() * 0.5f );
			}
			else {
				CChitArray query;
				ChitAcceptAll all;
				// This is a 1x1 query - will sometimes ignore big buildings, which is a minor bug.
				scriptContext->chit->GetChitBag()->QuerySpatialHash( &query, ToWorld2F(pos), 0.5f, scriptContext->chit, &all );
				for( int i=0; i<query.Size(); ++i ) {
					RenderComponent* rc = query[i]->GetRenderComponent();
					if ( rc ) {
						Rectangle3F aabb = rc->MainModel()->AABB();
						sunHeight = Min( sunHeight, aabb.max.y*0.5f );
						break;
					}
				}
			}
		}

		float sunPerUnitH = sunHeight * (1.0f-rainFraction) / h;
		sunPerUnitH = Clamp( sunPerUnitH, 0.0f, 1.0f );

		// ---------- Rain ------- //
		float water = rainFraction;

		static const Vector2I check[4] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
		for( int i=0; i<GL_C_ARRAY_SIZE( check ); ++i ) {
			tap = pos + check[i];
			if ( bounds.Contains( tap )) {
				const WorldGrid& wg = context->worldMap->GetWorldGrid( tap.x, tap.y );
				// Water or rock runoff increase water.
				if ( wg.RockHeight() ) {
					water += 0.25f * rainFraction;
				}
				if ( wg.IsWater() ) {
					water += 0.25f;
				}
			}
		}
		water = Clamp( water, 0.f, 1.f );

		// ------- Temperature ----- //
		float temp = weather->Temperature( (float)pos.x+0.5f, (float)pos.y+0.5f );

		// ------- calc ------- //
		Vector3F actual = { sunPerUnitH, water, temp };
		Vector3F optimal = { 0.5f, 0.5f, 0.5f };
	
		item->keyValues.Fetch( "sun",  "f", &optimal.x );
		item->keyValues.Fetch( "rain", "f", &optimal.y );
		item->keyValues.Fetch( "temp", "f", &optimal.z );

		float distance = ( optimal - actual ).Length();

		const float GROW = Lerp( 0.2f, 0.1f, (float)stage / (float)(NUM_STAGE-1) );
		const float DIE  = 0.4f;

		float toughness = item->Traits().Toughness();
		float seconds = float( TIME_TO_GROW ) / 1000.0f;

		if ( distance < GROW * toughness ) {

			// Heal.
			ChitMsg healMsg( ChitMsg::CHIT_HEAL );
			healMsg.dataF = HP_PER_SECOND*seconds*item->Traits().NormalLeveledTrait( GameTrait::STR );
			scriptContext->chit->SendMessage( healMsg );

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
		}
	}
	// Spore (more likely if bigger plant)
	if (    spore 
		 && ( int(scriptContext->chit->random.Rand(nStage)) <= stage )) 
	{
		// Number range reflects wind direction.
		int dx = -1 + scriptContext->chit->random.Rand(4);	// [-1,2]
		int dy = -1 + scriptContext->chit->random.Rand(3);	// [-1,1]

		// Remember that create plant will favor creating
		// existing plants, so we don't need to specify
		// what to create.
		Sim::Instance()->CreatePlant( pos.x+dx, pos.y+dy, -1 );
	}

	scriptContext->chit->GetRenderComponent()->SetSaturation( item->HPFraction() );

	return tick;
}


