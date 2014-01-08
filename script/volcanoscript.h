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

#include "scriptcomponent.h"
#include "../grinliz/glvector.h"

class WorldMap;

class VolcanoScript : public IScript
{
public:
	VolcanoScript( WorldMap* map, int maxSize );
	virtual ~VolcanoScript()			{}

	virtual void Init();
	virtual void Serialize( XStream* xs );
	virtual void OnAdd()	{}
	virtual void OnRemove()	{}

	virtual int DoTick( U32 delta );
	virtual const char* ScriptName() { return "VolcanoScript"; }

private:
	WorldMap*	worldMap;
	int			size;
	int			maxSize;
};


#endif // VOLCANO_SCRIPT_INCLUDED