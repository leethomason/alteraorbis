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
		CHARACTER,
		VAULT,
		MARKET,
		EXCHANGE
	};

	CharacterSceneData(	ItemComponent* ic,		// character
						ItemComponent* ic2,		// vault, market, exchange
						int _type,				// apply markup, costs
						Wallet* _taxRecipiant)	// who gets sales tax
		: SceneData(), itemComponent(ic), storageIC(ic2), taxRecipiant(_taxRecipiant), type(_type) { GLASSERT(ic); }

	ItemComponent*	itemComponent;
	ItemComponent*	storageIC;			// vault, chest, storage, etc.				
	Wallet*			taxRecipiant;
	int				type;

	bool IsCharacter() const	{ return type == CHARACTER; }
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

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual gamui::RenderAtom	CharacterScene::DragStart( const gamui::UIItem* item );
	virtual void				DragEnd( const gamui::UIItem* start, const gamui::UIItem* end );

	virtual void DoTick( U32 deltaTime );
	virtual void Draw3D( U32 delatTime );
	virtual grinliz::Color4F ClearColor();

private:
	void SetButtonText();
	void SetExchangeButtonText();
	void SetItemInfo(const GameItem* item, const GameItem* user);
	void CalcCost( int* bought, int* sold, int* salesTax );
	void ResetInventory();
	void CalcCrystalValue();

	enum { 
		NUM_ITEM_BUTTONS = INVERTORY_SLOTS,

	};
	
	LumosGame*			lumosGame;
	Engine*				engine;
	CharacterSceneData*	data;
	Model*				model;
	int					nStorage;	// 1 for character, 2 for vault

	Screenport			screenport;
	gamui::PushButton	okay, cancel, reset;
	gamui::ToggleButton itemButton[2][NUM_ITEM_BUTTONS];
	int					itemButtonIndex[2][NUM_ITEM_BUTTONS];	// the index in the ItemComponent
	gamui::TextLabel	desc;
	gamui::PushButton	dropButton;
	MoneyWidget			moneyWidget[2];		// left is character, right is market
	FaceToggleWidget	faceWidget;
	ItemDescWidget		itemDescWidget;
	gamui::TextLabel	billOfSale;
	gamui::TextLabel	helpText;
	gamui::PushButton	crystalButton[2][NUM_CRYSTAL_TYPES];
	int					crystalValue[NUM_CRYSTAL_TYPES];

	grinliz::CDynArray< const GameItem* > boughtList, soldList;

};


#endif // CHARACTER_SCENE_INCLUDED
