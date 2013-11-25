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

#ifndef DIALOGSCENE_INCLUDED
#define DIALOGSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;
class ItemComponent;

class DialogScene : public Scene
{
public:
	DialogScene( LumosGame* game );
	virtual ~DialogScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual gamui::RenderAtom DragStart( const gamui::UIItem* item );
	virtual void DragEnd( const gamui::UIItem* start, const gamui::UIItem* end );
	virtual void SceneResult( int sceneID, int result, const SceneData* data );

private:
	enum { NUM_ITEMS = 4 };

	LumosGame* lumosGame;
	gamui::PushButton okay;
	gamui::PushButton itemArr[NUM_ITEMS];

	// Objects for testing various UI scenes.
	ItemComponent* itemComponent0;
	ItemComponent* itemComponent1;
	ItemComponent* marketComponent;

	enum { NUM_TOGGLES	= 2,
		   NUM_SUB		= 2,
		   CHARACTER	= 0,
		   VAULT,
		   FORGE,
		   MARKET,
		   NUM_SCENES
	};
	gamui::ToggleButton toggles[NUM_TOGGLES];
	gamui::ToggleButton subButtons[NUM_SUB*NUM_TOGGLES];

	gamui::PushButton	sceneButtons[NUM_SCENES];
};

#endif // DIALOGSCENE_INCLUDED