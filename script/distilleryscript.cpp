#include "distilleryscript.h"
#include "../xarchive/glstreamer.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/rendercomponent.h"
#include "../game/lumoschitbag.h"
#include "../game/mapspatialcomponent.h"
#include "corescript.h"
#include "evalbuildingscript.h"
#include "../script/itemscript.h"

using namespace grinliz;

DistilleryScript::DistilleryScript() : progressTick( 2000 ), progress(0)
{
}


void DistilleryScript::Serialize( XStream* xs )
{
	BeginSerialize(xs, Name());
	XARC_SER( xs, progress );
	progressTick.Serialize( xs, "progressTick" );
	EndSerialize(xs);
}


int DistilleryScript::ElixirTime( int tech )
{
	return ELIXIR_TIME * TECH_MAX / (tech+1);
}


void DistilleryScript::SetInfoText()
{
	CStr<32> str;

	ItemComponent* ic   = parentChit->GetItemComponent();
	RenderComponent* rc = parentChit->GetRenderComponent();

	if ( ic && rc ) {
		int nFruit = 0;
		for( int i=1; i<ic->NumItems(); ++i ) {
			if ( ic->GetItem( i )->IName() == ISC::fruit ) {
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
	ItemComponent* ic = parentChit->GetItemComponent();

	if ( n ) { 
		MapSpatialComponent* msc = GET_SUB_COMPONENT(parentChit, SpatialComponent, MapSpatialComponent);
		GLASSERT( msc );
		Vector2I sector = msc->GetSector();
		Rectangle2I porch = msc->PorchPos();
		CoreScript* cs = CoreScript::GetCore( sector );
		GLASSERT(cs);
		if (!cs) return VERY_LONG_TICK;

		EvalBuildingScript* evalScript = (EvalBuildingScript*) parentChit->GetComponent("EvalBuildingScript");
	
		float tech = Max(cs->GetTech(), 0.8f);
		double dProg = double(tech) * double(progressTick.Period()) / double(TECH_MAX);
		double p = dProg*double(n);
		if (evalScript) {
			p *= 0.65 + 0.35*evalScript->EvalIndustrial(false);
		}

		progress += int(p);

		while (progress >= ELIXIR_TIME) {
			progress -= ELIXIR_TIME;

			if ( ic ) {
				int index = ic->FindItem( ISC::fruit );
				if ( index >= 0 ) {
					//cs->nElixir += ELIXIR_PER_FRUIT;
					GameItem* item = ic->RemoveFromInventory( index );
					delete item;

					const GameItem& def = ItemDefDB::Instance()->Get( "elixir" );

					for (int k = 0; k < ELIXIR_PER_FRUIT; ++k) {
						GameItem* gameItem = def.Clone();
						Vector2F pos2 = RandomInRect(porch, &parentChit->random);
						static const int ELIXIR_SELF_DESTRUCT = 60*1000;
						Context()->chitBag->NewItemChit(ToWorld3F(pos2), gameItem, false, true, ELIXIR_SELF_DESTRUCT);
					}
				}
			}
		}
	}
	
	if (ic && ic->FindItem(ISC::fruit) < 0) {
		// No fruit, no progress.
		progress = 0;
	}

	SetInfoText();
	return progressTick.Next();
}


