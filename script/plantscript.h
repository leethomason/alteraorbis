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

#ifndef PLANT_SCRIPT_INCLUDED
#define PLANT_SCRIPT_INCLUDED

#include "../grinliz/glrandom.h"
#include "../xegame/component.h"
#include "../game/gamelimits.h"

class WorldMap;
struct ChitContext;
class GameItem;

class PlantScript
{
public:
	PlantScript(const ChitContext*);
	void DoTick(U32 delta);

	static const GameItem* PlantDef(int plant0Based);

private:
	static const GameItem* plantDef[NUM_PLANT_TYPES];

	U32 index;
	const ChitContext* context;
	grinliz::Random random;
};


#endif // PLANT_SCRIPT_INCLUDED