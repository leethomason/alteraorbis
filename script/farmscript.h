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

#ifndef Farm_SCRIPT_INCLUDED
#define Farm_SCRIPT_INCLUDED

#include "../xegame/cticker.h"
#include "../gamui/gamui.h"
#include "../xegame/component.h"


class FarmScript : public Component
{
	typedef Component super;
public:
	FarmScript();
	virtual ~FarmScript()	{}

	virtual void OnAdd(Chit* chit, bool init);
	virtual void Serialize( XStream* xs );
	virtual int DoTick( U32 delta );
	virtual const char* Name() const { return "FarmScript"; }

	static int GrowFruitTime( int plantStage, int nPlants );

private:

	enum {
		// These are general "guess" values. The only one that is 
		// used is GROWTH_NEEDED
		NUM_PLANTS		= 10,
		NOMINAL_STAGE	= 3,
		// Tricky to get right.
		// 60: 4 good farms can sustain 4 denizens. Puts a little too much pressure on farming. Ideal(2,0) = 4.1
		// 40: shouldn't have made so much of a difference...but way easy. But also later game & higher tech.
		FRUIT_TIME		= 50*1000,		// Ideal(2,0) = 5.4 possibly not enough...
		GROWTH_NEEDED	= NUM_PLANTS * (NOMINAL_STAGE+1)*(NOMINAL_STAGE+1) * FRUIT_TIME
	};

	void ComputeFarmBound();

	CTicker timer;
	int fruitGrowth;

	// Not serialized:
	grinliz::Rectangle2I farmBounds;
	int	efficiency;

	gamui::Image baseImage;
};

#endif // PLANT_SCRIPT_INCLUDED
