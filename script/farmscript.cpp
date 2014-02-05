#include "farmscript.h"
#include "../xegame/chit.h"
#include "../grinliz/glrectangle.h"
#include "../xegame/spatialcomponent.h"
#include "../game/lumoschitbag.h"
#include "../script/plantscript.h"
#include "../script/itemscript.h"
#include "../game/gameitem.h"

using namespace grinliz;

static const int	FARM_SCRIPT_CHECK = 2000;
static const float	GROW_RAD = 2.2f;
// FIXME: longer time to grow fruit
static const int	GROW_FRUIT = 5000;	// the most productive plants grow every GROW_PERIOD seconds
// FIXME: longer time to self destruct
static const int	FRUIT_SELF_DESTRUCT = 10*1000;

FarmScript::FarmScript() : timer( 2000 )
{
}


void FarmScript::Serialize( XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	timer.Serialize( xs, "timer" );
	XarcClose( xs );
}


void FarmScript::Init()
{
	timer.SetPeriod( FARM_SCRIPT_CHECK + scriptContext->chit->random.Rand(FARM_SCRIPT_CHECK/16));
}


int FarmScript::DoTick( U32 delta )
{
	if ( timer.Delta( delta ) && scriptContext->chit->GetSpatialComponent() ) {
		Rectangle2F bounds;
		bounds.min = bounds.max = scriptContext->chit->GetSpatialComponent()->GetPosition2D();
		bounds.Outset( GROW_RAD );

		PlantFilter filter;
		plantArr.Clear();
		scriptContext->chitBag->QuerySpatialHash( &plantArr, bounds, 0, &filter );

		for( int i=0; i<plantArr.Size(); ++i ) {
			Chit* chit = plantArr[i];
			int type = 0, stage = 0;
			GameItem* plantItem = PlantScript::IsPlant( chit, &type, &stage );
			GLASSERT( plantItem );

			int num   = (stage+1)*(stage+1);
			int denom = PlantScript::NUM_STAGE*PlantScript::NUM_STAGE;
			int add   = Max( 1, int(delta)*num/denom );

			int growth = plantItem->keyValues.Add( "fruitGrowth", add );
			if ( growth > GROW_FRUIT ) {
				plantItem->keyValues.Set( "fruitGrowth", "d", 0 );
				
				const GameItem& def = ItemDefDB::Instance()->Get( "fruit" );
				GameItem* gameItem = new GameItem( def );
				scriptContext->chitBag->NewItemChit( chit->GetSpatialComponent()->GetPosition(),
													 gameItem,
													 true,
													 true,
													 FRUIT_SELF_DESTRUCT );
			}
		}
	}
	return timer.Next();
}
