#include "distilleryscript.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/rendercomponent.h"
#include "../game/lumoschitbag.h"
#include "corescript.h"
#include "evalbuildingscript.h"

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


void DistilleryScript::SetInfoText()
{
	CStr<32> str;

	ItemComponent* ic   = scriptContext->chit->GetItemComponent();
	RenderComponent* rc = scriptContext->chit->GetRenderComponent();

	if ( ic && rc ) {
		int nFruit = 0;
		for( int i=1; i<ic->NumItems(); ++i ) {
			if ( ic->GetItem( i )->IName() == IStringConst::fruit ) {
				++nFruit;
			}
		}
		double fraction = double(progress) / double(ELIXIR_TIME);
		if (nFruit == 0)
			fraction = 0;

		str.Format( "Fruit: %d [%0d%%]", nFruit, int(fraction*100.0) );
		rc->SetDecoText( str.c_str() );

	}
}


int DistilleryScript::DoTick( U32 delta )
{
	int n = progressTick.Delta( delta );
	ItemComponent* ic = scriptContext->chit->GetItemComponent();

	if ( n ) { 
		SpatialComponent* sc = scriptContext->chit->GetSpatialComponent();
		GLASSERT( sc );
		Vector2I sector = ToSector( sc->GetPosition2DI() );
		CoreScript* cs = CoreScript::GetCore( sector );
		EvalBuildingScript* evalScript = (EvalBuildingScript*) scriptContext->chit->GetScript("EvalBuildingScript");
	
//		int tech = cs->GetTechLevel();
		float tech = Min(cs->GetTech(), 0.8f);
		double dProg = double(tech) * double(progressTick.Period()) / double(TECH_MAX);
		double p = dProg*double(n);
		if (evalScript) {
			p *= 0.5 + 0.5*evalScript->EvalIndustrial(false);
		}

		progress += int(p);

		while (progress >= ELIXIR_TIME) {
			progress -= ELIXIR_TIME;

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
	
	if (ic && ic->FindItem(IStringConst::fruit) < 0) {
		// No fruit, no progress.
		progress = 0;
	}

	SetInfoText();
	return progressTick.Next();
}


