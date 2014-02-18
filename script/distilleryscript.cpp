#include "distilleryscript.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../game/lumoschitbag.h"
#include "corescript.h"

using namespace grinliz;

DistilleryScript::DistilleryScript() : progressTick( 2000 ), progress(0)
{
}


void DistilleryScript::Serialize( XStream* xs )
{
	XarcOpen( xs, ScriptName() );
	XARC_SER( xs, progress );
	progressTick.Serialize( xs, "progressTick" );
	XarcClose( xs );
}


int DistilleryScript::ElixirTime( int tech )
{
	return ELIXIR_TIME * TECH_MAX / (tech+1);
}



int DistilleryScript::DoTick( U32 delta )
{
	int n = progressTick.Delta( delta );

	if ( n ) { 
		SpatialComponent* sc = scriptContext->chit->GetSpatialComponent();
		GLASSERT( sc );
		Vector2I sector = ToSector( sc->GetPosition2DI() );
		CoreScript* cs = scriptContext->chitBag->GetCore( sector );
	
		int tech = cs->GetTechLevel();
		int dProg = (tech+1) * progressTick.Period() / TECH_MAX;

		progress += dProg*n;

		while ( progress >= ELIXIR_TIME ) {
			progress -= ELIXIR_TIME;

			ItemComponent* ic = scriptContext->chit->GetItemComponent();
			if ( ic ) {
				int index = ic->FindItem( IStringConst::fruit );
				if ( index >= 0 ) {
					cs->nElixir += ELIXIR_PER_FRUIT;
					GameItem* item = ic->RemoveFromInventory( index );
					delete item;
				}
			}
		}
	}
	return progressTick.Next();
}


