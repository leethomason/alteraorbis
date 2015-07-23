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
#include "../widget/itemdescwidget.h"

class LumosGame;
class Engine;
class ItemComponent;
class GameItem;


class CharacterSceneData : public SceneData
{
public:
	enum {
		AVATAR,
		CHARACTER_ITEM,
		VAULT,
		MARKET,
		EXCHANGE
	};

	CharacterSceneData(	ItemComponent* ic,		// character
						ItemComponent* ic2,		// vault, market, exchange
						int _type,				// apply markup, costs
						const GameItem* _select)
		: SceneData(), itemComponent(ic), storageIC(ic2), type(_type), selectItem(_select) { GLASSERT(ic); }

	ItemComponent*	itemComponent;
	ItemComponent*	storageIC;			// vault, chest, storage, etc.				
	int				type;
	const GameItem*		selectItem;

	bool IsAvatar() const					{ return type == AVATAR; }
	bool IsCharacterItem() const			{ return type == CHARACTER_ITEM; }
	bool IsAvatarCharacterItem() const		{ return type == AVATAR || type == CHARACTER_ITEM; }

	bool IsVault() const		{ return type == VAULT; }
	bool IsMarket() const		{ return type == MARKET; }
	bool IsExchange() const		{ return type == EXCHANGE;  }
};


class CharacterScene : public Scene
{
public:
	CharacterScene( LumosGame* game, CharacterSceneData* itemComponent );
	virtual ~CharacterScene();

	virtual void Resize();
	virtual void Activate();
	virtual void DeActivate();

	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		return ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual gamui::RenderAtom	DragStart( const gamui::UIItem* item );
	virtual void				DragEnd( const gamui::UIItem* start, const gamui::UIItem* end );

	virtual void DoTick( U32 deltaTime );
	virtual void Draw3D( U32 delatTime );

private:
	void SetButtonText(const GameItem* selectThis);
	void SetExchangeButtonText();
	void SetItemInfo(const GameItem* item, const GameItem* user);

	enum { 
		NUM_ITEM_BUTTONS = MAX_CARRIED_ITEMS,
	};
	
	LumosGame*			lumosGame;
	Engine*				engine;
	CharacterSceneData*	data;
	Model*				model;
	int					nStorage;	// 1 for character, 2 for vault

	Screenport			screenport;
	gamui::PushButton	okay;
	gamui::ToggleButton itemButton[2][NUM_ITEM_BUTTONS];
	int					itemButtonIndex[2][NUM_ITEM_BUTTONS];	// the index in the ItemComponent
	gamui::TextLabel	desc;
	gamui::PushButton	dropButton;
	MoneyWidget			moneyWidget[2];		// left is character, right is market
	FaceToggleWidget	faceWidget;
	ItemDescWidget		itemDescWidget;
	gamui::TextLabel	helpText;
	gamui::PushButton	crystalButton[2][NUM_CRYSTAL_TYPES];
	gamui::Image		dropTarget;
};


#endif // CHARACTER_SCENE_INCLUDED
