#include "farmscript.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/istringconst.h"

#include "../engine/uirendering.h"
#include "../engine/engine.h"

#include "../grinliz/glrectangle.h"

#include "../game/mapspatialcomponent.h"
#include "../game/lumoschitbag.h"
#include "../game/gameitem.h"
#include "../game/worldmap.h"

#include "../script/plantscript.h"
#include "../script/itemscript.h"

using namespace grinliz;
using namespace gamui;

static const int	FRUIT_SELF_DESTRUCT = 60*1000;

FarmScript::FarmScript() : timer( 2000 )
{
	fruitGrowth = 0;
	farmBounds.Zero();
	efficiency = 0;
}


void FarmScript::Serialize( XStream* xs )
{
	BeginSerialize(xs, Name());
	XARC_SER( xs, fruitGrowth );
	timer.Serialize( xs, "timer" );
	EndSerialize(xs);
}




void FarmScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		timer.Randomize(parentChit->random.Rand());
	}
	//const ChitContext* context = scriptContext->chit->GetChitContext();
	TextureManager* tm = TextureManager::Instance();
	const Texture* t = tm->GetTexture("farmzone");

	RenderAtom atom((const void*)WorldMap::RENDERSTATE_MAP_TRANSLUCENT,
		(const void*)t,
		0, 0, 1, 1 );
	baseImage.Init(&Context()->engine->GetMap()->overlay0, atom, true);
	baseImage.SetSlice(true);
}


int FarmScript::GrowFruitTime( int stage, int nPlants )
{
	int stage2   = (stage+1)*(stage+1);
	int time = GROWTH_NEEDED / (stage2 * nPlants);
	return time;
}


void FarmScript::ComputeFarmBound()
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT(parentChit, SpatialComponent, MapSpatialComponent);
	GLASSERT(msc);
	if (!msc) return;
	Vector2I pos2i = msc->MapPosition();

	// Other farms clip this if they are too close.
	// 01234567
	// FffgG		If within 4.eps, may impact us.

	float rad = float(FARM_GROW_RAD)*2.f + 0.1f;
	ItemNameFilter filter(IStringConst::farm, IChitAccept::MAP);
	CChitArray array;
	Context()->chitBag->QuerySpatialHash(&array, ToWorld2F(pos2i), rad, parentChit, &filter);

	// This bound needs to trim to qBounds. Sort of a tricky
	// algorithm. Similar to the "block" algorithm. Also, what
	// happens when the center of one farm is within the center
	// of another? Don't want dependencies between. Tried 
	// different approaches, ended up with a symmetric 
	// "round down" approach.

	int r = FARM_GROW_RAD;
	for (int i = 0; i < array.Size(); ++i) {
		Chit* chit = array[i];
		Vector2I q2i = chit->GetSpatialComponent()->GetPosition2DI();

		int d = Max(abs(q2i.x - pos2i.x), abs(q2i.y - pos2i.y));
		// 5->2, 4->1 3->1 2->0
		r = Min(r, (d - 1)/2);
	}
	GLASSERT(r >= 0);
	farmBounds.min = farmBounds.max = pos2i;
	farmBounds.Outset(r);

	baseImage.SetSize(float(farmBounds.Width()), float(farmBounds.Height()));
	baseImage.SetPos(float(farmBounds.min.x), float(farmBounds.min.y));
}

int FarmScript::DoTick( U32 delta )
{
	int n = timer.Delta( delta );
	MapSpatialComponent* msc = GET_SUB_COMPONENT(parentChit, SpatialComponent, MapSpatialComponent);
	if (!msc) return VERY_LONG_TICK;	// static analysis

	while ( n && msc ) {
		--n;
		ComputeFarmBound();
		int growth = 0;

		for (Rectangle2IIterator it(farmBounds); !it.Done(); it.Next()) {
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos().x, it.Pos().y);

			if (wg.Plant()) {
				int type = wg.Plant() - 1;
				int stage = wg.PlantStage();
				const GameItem* plantItem = PlantScript::PlantDef(type);

				int g = (stage + 1)*(stage + 1);
				if (   (plantItem->flags & GameItem::FLAMMABLE) 
					|| (plantItem->flags & GameItem::SHOCKABLE)) 
				{
					g *= 2;	// bonus for volitility
				}
				growth += g;
			}
		}

		fruitGrowth += growth * timer.Period();
		const int AREA = (FARM_GROW_RAD * 2 + 1)*(FARM_GROW_RAD * 2 + 1) - 1;
		efficiency = 100 * growth / (MAX_PLANT_STAGES * MAX_PLANT_STAGES * AREA);
	}

	while ( fruitGrowth >= GROWTH_NEEDED ) {
		fruitGrowth -= GROWTH_NEEDED;

		Rectangle2I r = msc->PorchPos();

		const GameItem& def = ItemDefDB::Instance()->Get( "fruit" );
		GameItem* gameItem = new GameItem( def );
		Context()->chitBag->NewItemChit( ToWorld3F( r.min ),
											 gameItem,
											 true,
											 true,
											 FRUIT_SELF_DESTRUCT );
	}

	RenderComponent* rc = parentChit->GetRenderComponent();
	if (rc) {
		CStr<32> str;
		//str.Format("%d,%d-%d,%d e=%d", farmBounds.min.x, farmBounds.min.y, farmBounds.max.x, farmBounds.max.y, efficiency);
		str.Format("Eff=%d%% [%d%%]", efficiency, int(double(fruitGrowth)*100.0/double(GROWTH_NEEDED)));
		rc->SetDecoText(str.c_str());
	}

	return timer.Next();
}
