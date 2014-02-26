#include "farmscript.h"
#include "../xegame/chit.h"
#include "../grinliz/glrectangle.h"
#include "../xegame/spatialcomponent.h"
#include "../game/mapspatialcomponent.h"
#include "../game/lumoschitbag.h"
#include "../script/plantscript.h"
#include "../script/itemscript.h"
#include "../game/gameitem.h"

using namespace grinliz;

static const int	FARM_SCRIPT_CHECK = 2000;
// FIXME: not tuned
static const int	FRUIT_SELF_DESTRUCT = 60*1000;

FarmScript::FarmScript() : timer( 2000 )
{
	fruitGrowth = 0;
}


void FarmScript::Serialize( XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XARC_SER( xs, fruitGrowth );
	timer.Serialize( xs, "timer" );
	XarcClose( xs );
}


void FarmScript::Init()
{
	timer.SetPeriod( FARM_SCRIPT_CHECK + scriptContext->chit->random.Rand(FARM_SCRIPT_CHECK/16));
}


int FarmScript::GrowFruitTime( int stage, int nPlants )
{
	int stage2   = (stage+1)*(stage+1);
	int time = GROWTH_NEEDED / (stage2 * nPlants);
	return time;
}


int FarmScript::DoTick( U32 delta )
{
	int n = timer.Delta( delta );
	MapSpatialComponent* msc = GET_SUB_COMPONENT( scriptContext->chit, SpatialComponent, MapSpatialComponent );

	while ( n && msc ) {
		--n;

		Rectangle2F bounds;
		bounds.min = bounds.max = scriptContext->chit->GetSpatialComponent()->GetPosition2D();
		bounds.Outset( FARM_GROW_RAD );

		PlantFilter filter;
		plantArr.Clear();
		scriptContext->chitBag->QuerySpatialHash( &plantArr, bounds, 0, &filter );
		
		int growth = 0;

		for( int i=0; i<plantArr.Size(); ++i ) {
			Chit* chit = plantArr[i];
			int type = 0, stage = 0;
			GameItem* plantItem = PlantScript::IsPlant( chit, &type, &stage );
			GLASSERT( plantItem );

			growth += (stage+1)*(stage+1);
		}

		fruitGrowth += growth * timer.Period();
	}

	while ( fruitGrowth >= GROWTH_NEEDED ) {
		fruitGrowth -= GROWTH_NEEDED;

		Rectangle2I r = msc->PorchPos();

		const GameItem& def = ItemDefDB::Instance()->Get( "fruit" );
		GameItem* gameItem = new GameItem( def );
		scriptContext->chitBag->NewItemChit( ToWorld3F( r.min ),
											 gameItem,
											 true,
											 true,
											 FRUIT_SELF_DESTRUCT );
	}
	return timer.Next();
}
