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

#ifndef VOLCANO_SCRIPT_INCLUDED
#define VOLCANO_SCRIPT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/cticker.h"
#include "../grinliz/glvector.h"

class WorldMap;

enum class EVolcanoType
{
	VOLCANO,	// standard, restores nominal rock heights
	MOUNTAIN,	// creates a mountain, ignores nominal
	CRATER		// creates a pool, ignores nominal
};

class VolcanoScript : public Component
{
	typedef Component super;

public:
	VolcanoScript(int maxRad = 6, bool round = false, EVolcanoType type = EVolcanoType::VOLCANO);
	virtual ~VolcanoScript()			{}

	virtual void Serialize( XStream* xs );
	virtual void OnAdd(Chit* chit, bool init);
	virtual void OnRemove();

	virtual int DoTick( U32 delta );
	virtual const char* Name() const { return "VolcanoScript"; }

private:
	int			maxRad;
	bool		round;
	EVolcanoType type;
	CTicker		spreadTicker;
	int			rad;
	int			size;
};


#endif // VOLCANO_SCRIPT_INCLUDED