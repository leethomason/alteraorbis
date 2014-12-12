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

#ifndef LUMOS_VISITOR_INCLUDED
#define LUMOS_VISITOR_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "sectorport.h"

class XStream;
class WorldMap;
class ChitBag;
class LumosChitBag;
class Web;

// Each visitor has their own personality and memory, seperate
// from the chit AI.
struct VisitorData
{
	VisitorData();
	void Serialize( XStream* xs );

	void Connect() {
		kioskTime = 0;
		visited.Clear();
	}

	enum {	
			KIOSK_TIME = 5000,
			MEMORY = 8,
			NUM_KIOSK_TYPES = 4,
			KIOSK_N = 0,				// News, red
			KIOSK_M,					// Media, blue
			KIOSK_C,					// Commerce, green
			KIOSK_S						// Social, violet

	};

	int id;								// chit id, and whether in-world or not.
	U32 kioskTime;						// time spent standing at current kiosk
	int want;
	grinliz::CDynArray<grinliz::Vector2I> visited;

	grinliz::IString CurrentKioskWant();
	int CurrentKioskWantID()	{ return want; }
};


class Visitors
{
public:
	Visitors();
	~Visitors();

	void Serialize( XStream* xs );

	enum { NUM_VISITORS = 200 };
	VisitorData visitorData[NUM_VISITORS];

	static VisitorData* Get( int index );
	// only returns existing; doesn't create.
	static Visitors* Instance()	{ return instance; }

	SectorPort ChooseDestination(int index, const Web& web, ChitBag* chitBag, WorldMap* map);

private:
	static Visitors* instance;
	grinliz::Random random;
};

#endif // LUMOS_VISITOR_INCLUDED
