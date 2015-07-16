/*
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "volcanoscript.h"

#include "../engine/serialize.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/chitcontext.h"

#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../game/reservebank.h"
#include "../game/lumoschitbag.h"

#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

static const U32 SPREAD_RATE = 4000;

VolcanoScript::VolcanoScript(int _maxRad, EVolcanoType _type) : maxRad(_maxRad), type(_type), spreadTicker(SPREAD_RATE)
{
	size = 0;
	rad = 0;
}


void VolcanoScript::Serialize(XStream* xs)
{
	BeginSerialize(xs, Name());
	XARC_SER(xs, maxRad);
	XARC_SER_ENUM(xs, EVolcanoType, type);
	XARC_SER(xs, size);
	XARC_SER(xs, rad);
	spreadTicker.Serialize(xs, "SpreadTicker");
	EndSerialize(xs);
}


int VolcanoScript::DoTick(U32 delta)
{
	int n = spreadTicker.Delta(delta);
	for (int i = 0; i < n; ++i) {

		++rad;

		WorldMap* worldMap = Context()->worldMap;
		Vector2I pos2i = ToWorld2I(parentChit->Position());

		bool done = rad > maxRad;
		rad = Min(rad, maxRad);

		Rectangle2I rOuter, rInner;
		rOuter.min = rOuter.max = pos2i;
		rOuter.Outset(rad);

		rInner = rOuter;
		rInner.Outset(-1);

		rOuter.DoIntersection(worldMap->Bounds());
		rInner.DoIntersection(worldMap->Bounds());

		if (done) {
			// Everything off.
			for (Rectangle2IIterator it(rOuter); !it.Done(); it.Next()) {
				worldMap->SetMagma(it.Pos().x, it.Pos().y, false);
			}
			parentChit->QueueDelete();
		}
		else {
			// Inner off.
			for (Rectangle2IEdgeIterator it(rInner); !it.Done(); it.Next()) {
				worldMap->SetMagma(it.Pos().x, it.Pos().y, false);
			}
			// Center on.
			worldMap->SetPlant(pos2i.x, pos2i.y, 0, 0);
			worldMap->SetMagma(pos2i.x, pos2i.y, true);

			if (type == EVolcanoType::VOLCANO) {
				// Edge on to nominnal rock height.
				for (Rectangle2IEdgeIterator it(rOuter); !it.Done(); it.Next()) {
					const WorldGrid& wg = worldMap->GetWorldGrid(it.Pos());
					if (!wg.Plant() && !wg.IsBlocked() && !wg.FluidSink()) {
						worldMap->SetRock(it.Pos().x, it.Pos().y, -1, true, 0);
					}
				}
			}
			else if (type == EVolcanoType::MOUNTAIN) {
				for (Rectangle2IEdgeIterator it(rOuter); !it.Done(); it.Next()) {
					float fraction = float(maxRad - rad) / float(maxRad);
					int h = 1 + int(fraction * float(MAX_ROCK_HEIGHT)) + parentChit->random.Bit();
					h = Clamp(h, 1, int(MAX_ROCK_HEIGHT));

					const WorldGrid& wg = worldMap->GetWorldGrid(it.Pos());
					if (!wg.Plant() && !wg.IsBlocked() && !wg.FluidSink()) {
						worldMap->SetRock(it.Pos().x, it.Pos().y, Max(int(wg.RockHeight()), h), true, 0);
					}
				}
			}
			else if (type == EVolcanoType::CRATER) {
				for (Rectangle2IEdgeIterator it(rOuter); !it.Done(); it.Next()) {
					// Remove from inside.
					if (rInner.Width() > 2 && rInner.Height() > 2) {
						Vector2I inside = { rInner.min.x + parentChit->random.Rand(rInner.Width() - 2),
							rInner.min.y + parentChit->random.Rand(rInner.Height() - 2) };

						const WorldGrid& wgInside = worldMap->GetWorldGrid(inside);
						if (wgInside.RockHeight()) {
							worldMap->SetRock(inside.x, inside.y, wgInside.RockHeight() - 1, true, 0);
						}
					}

					// Add to outside.
					const WorldGrid& wgEdge = worldMap->GetWorldGrid(it.Pos());
					if (!wgEdge.Plant() && !wgEdge.IsBlocked() && !wgEdge.FluidSink()) {
						if (wgEdge.RockHeight() < MAX_ROCK_HEIGHT) {
							worldMap->SetRock(it.Pos().x, it.Pos().y, wgEdge.RockHeight() + 1, true, 0);
						}
					}
				}
			}
		}
	}

	return spreadTicker.Next();
}


void VolcanoScript::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
}


void VolcanoScript::OnRemove()
{
	super::OnRemove();
}
