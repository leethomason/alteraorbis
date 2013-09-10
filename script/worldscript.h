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

#ifndef WORLD_SCRIPT_INCLUDED
#define WORLD_SCRIPT_INCLUDED

#include "../grinliz/glcontainer.h"
#include "../grinliz/glrectangle.h"

class Chit;
class Engine;
class Model;

class WorldScript
{
public:
	// Return true if this is a "top level" chit that sits in the world.
	static Chit* IsTopLevel( Model* model );

	// Query for the top level chits in a rectangular area
	//static void QueryChits( const grinliz::Rectangle2F& bounds, Engine* engine, grinliz::CDynArray<Chit*> *chitArr );

};


#endif // WORLD_SCRIPT_INCLUDED
