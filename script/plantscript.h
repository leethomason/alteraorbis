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

#include "scriptcomponent.h"
#include "../grinliz/glrandom.h"
#include "../xegame/cticker.h"

class WorldMap;
class Engine;
class Weather;
class GameItem;
class Chit;
class Sim;

class PlantScript : public Component
{
	typedef Component super;
public:
	PlantScript( int type );
	virtual ~PlantScript()	{}

	virtual void Serialize( XStream* xs );
	virtual void OnAdd(Chit* parent, bool initialize);
	virtual void OnRemove();

	virtual int DoTick( U32 delta );
	virtual const char* Name() const { return "PlantScript"; }

	// Plant specific:
	static GameItem* IsPlant( Chit* chit, int* type, int* stage );
	virtual PlantScript*  ToPlantScript()	{ return this; }

	int Type() const	{ return type; }
	int Stage() const	{ return stage; }

	void SetStage( int stage );	// for debugging; not intended for general use

	enum {
		NUM_STAGE = 4,
		MAX_HEIGHT = NUM_STAGE,
		SHORT_PLANTS_START = 6
	};

private:
	void SetRenderComponent();
	const GameItem* GetResource();

	grinliz::Vector2I	lightTap;		// where to check for a shadow. (only check one spot.)
	int			type;		// 0-7, fern, tree, etc.
	int			stage;		// 0-3
	U32			age;
	U32			ageAtStage;
	CTicker		growTimer;
	CTicker		sporeTimer;
};

#endif // PLANT_SCRIPT_INCLUDED