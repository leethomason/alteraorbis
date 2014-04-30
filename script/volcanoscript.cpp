#include "volcanoscript.h"

#include "../engine/serialize.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../game/reservebank.h"
#include "../game/lumoschitbag.h"

#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

static const U32 SPREAD_RATE = 4000;

VolcanoScript::VolcanoScript() : spreadTicker(SPREAD_RATE)
{
	size = 0;
	rad = 0;
}


void VolcanoScript::Serialize( XStream* xs )
{
	BeginSerialize(xs, Name());
	XARC_SER( xs, size );
	XARC_SER(xs, rad);
	spreadTicker.Serialize(xs, "SpreadTicker");
	EndSerialize(xs);
}


int VolcanoScript::DoTick( U32 delta )
{
	SpatialComponent* sc = parentChit->GetSpatialComponent();
	WorldMap* worldMap = Context()->worldMap;

	Vector2I pos2i = { 0,  0 };
	GLASSERT( sc );
	if ( sc ) {
		pos2i = sc->GetPosition2DI();
	}
	else {
		parentChit->QueueDelete();
	}

	int n = spreadTicker.Delta(delta);
	for (int i = 0; i<n; ++i) {
		Rectangle2I b = worldMap->Bounds();
		++rad;

		// Cool (and set) the inner rectangle, make the new rectangle magma.
		// The origin stays magma until we're done.
		size = Min( rad, (int)MAX_RAD );
		
		Rectangle2I r;
		r.min = r.max = pos2i;
		r.Outset( size );

		r.DoIntersection(b);

		if (rad > MAX_RAD) {
			// Everything off.
			for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
				worldMap->SetMagma(it.Pos().x, it.Pos().y, false);
			}
			worldMap->SetMagma(pos2i.x, pos2i.y, false);
			parentChit->QueueDelete();
		}
		else {
			// Inner off.
			// Center on.
			// Edge on.

			// Inner off.
			Rectangle2I rSmall = r;
			r.Outset(-1);
			for (Rectangle2IIterator it(rSmall); !it.Done(); it.Next()) {
				worldMap->SetMagma(it.Pos().x, it.Pos().y, false);
			}

			// Center on.
			worldMap->SetRock(pos2i.x, pos2i.y, -1, true, 0);

			// Edge on.
			for (Rectangle2IEdgeIterator it(r); !it.Done(); it.Next()) {
				worldMap->SetRock(it.Pos().x, it.Pos().y, -1, true, 0);
			}
		}
	}
	
	return spreadTicker.Next();
}


void VolcanoScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		SpatialComponent* sc = parentChit->GetSpatialComponent();
		GLASSERT(sc);
		if (sc) {
			Vector2F pos = sc->GetPosition2D();

			NewsEvent event(NewsEvent::VOLCANO, pos);
			Context()->chitBag->GetNewsHistory()->Add(event);
		}
	}
}


void VolcanoScript::OnRemove()
{
	super::OnRemove();
	// spatial component already deleted. *sigh*
	/*
	// Defensive programming. Volcanoes are leaving
	// magma all around the world. The next step
	// is to track each volcano, not let them overlap,
	// and check against the magma.
	const ChitContext* context = scriptContext->chitBag->GetContext();
	SpatialComponent* sc = scriptContext->chit->GetSpatialComponent();
	WorldMap* worldMap = context->worldMap;

	Vector2I pos2i = sc->GetPosition2DI();

	Rectangle2I r;
	r.min = r.max = pos2i;
	r.Outset(MAX_RAD);

	Rectangle2I b = worldMap->Bounds();
	r.DoIntersection(b);

	// Everything off.
	for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
		worldMap->SetMagma(it.Pos().x, it.Pos().y, false);
	}
	*/
}
