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

#ifndef DEBUG_STATE_COMPONENT
#define DEBUG_STATE_COMPONENT

#include "../xegame/component.h"
#include "../gamui/gamui.h"

class WorldMap;
class LumosGame;

class DebugStateComponent : public Component
{
private:
	typedef Component super;

public:
	DebugStateComponent( WorldMap* _map );
	~DebugStateComponent()	{}

	virtual const char* Name() const { return "DebugStateComponent"; }

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );
	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );

private:
	WorldMap* map;
	gamui::DigitalBar healthBar;
	gamui::DigitalBar ammoBar;
	gamui::DigitalBar shieldBar;
};

#endif // DEBUG_STATE_COMPONENT
