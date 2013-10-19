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

#ifndef CHARACTER_SCENE_INCLUDED
#define CHARACTER_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glcontainer.h"
#include "../engine/screenport.h"
#include "../game/gamelimits.h"
#include "../widget/moneywidget.h"
#include "../widget/facewidget.h"

class LumosGame;
class Engine;
class ItemComponent;
class GameItem;


class CharacterSceneData : public SceneData
{
public:
	CharacterSceneData( ItemComponent* ic ) : SceneData(), itemComponent(ic) {}
	ItemComponent*							itemComponent;
};


class CharacterScene : public Scene
{
public:
	CharacterScene( LumosGame* game, CharacterSceneData* itemComponent );
	virtual ~CharacterScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual gamui::RenderAtom	CharacterScene::DragStart( const gamui::UIItem* item );
	virtual void				DragEnd( const gamui::UIItem* start, const gamui::UIItem* end );

	virtual void DoTick( U32 deltaTime );
	virtual void Draw3D( U32 delatTime );

private:
	void SetButtonText();
	void SetItemInfo( const GameItem* item, const GameItem* user );

	enum { 
		NUM_ITEM_BUTTONS = INVERTORY_SLOTS,

		KV_STR = 0,
		KV_WILL,
		KV_CHR,
		KV_INT,
		KV_DEX,
		NUM_TEXT_KV = KV_DEX+10
	};
	
	LumosGame*		lumosGame;
	Engine*			engine;
	ItemComponent*	itemComponent;	// what item or inventory are we displaying?
	Model*			model;

	Screenport			screenport;
	gamui::PushButton	okay;
	gamui::ToggleButton itemButton[NUM_ITEM_BUTTONS];
	int					itemButtonIndex[NUM_ITEM_BUTTONS];	// the index in the ItemComponent
	gamui::TextBox		desc;
	gamui::TextLabel	textKey[NUM_TEXT_KV];
	gamui::TextLabel	textVal[NUM_TEXT_KV];
	gamui::PushButton	dropButton;
	MoneyWidget			moneyWidget;
	FaceToggleWidget	faceWidget;
};


#endif // CHARACTER_SCENE_INCLUDED
