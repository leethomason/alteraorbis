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

#ifndef DEBUG_PATH_COMPONENT
#define DEBUG_PATH_COMPONENT

#include "../xegame/component.h"

class Engine;
class WorldMap;
class LumosGame;
class Model;
class ModelResource;

class DebugPathComponent : public Component
{
private:
	typedef Component super;

public:
	DebugPathComponent( Engine*, WorldMap*, LumosGame* );
	~DebugPathComponent();

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "DebugPathComponent" ) ) return this;
		return super::ToComponent( name );
	}

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual bool DoTick( U32 delta );

private:
	Engine* engine;
	WorldMap* map;
	LumosGame* game;

	const ModelResource* resource;
	Model* model;
};

#endif // DEBUG_PATH_COMPONENT
