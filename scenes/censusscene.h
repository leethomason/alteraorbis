#ifndef CENSUS_SCENE_INCLUDED
#define CENSUS_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../script/itemscript.h"

class LumosGame;
class ItemComponent;
class ChitBag;
class GameItem;


class CensusSceneData : public SceneData
{
public:
	CensusSceneData( ChitBag* _chitBag )				// apply markup, costs
		: SceneData(), chitBag(_chitBag) { GLASSERT(chitBag); }

	ChitBag* chitBag;
};


class CensusScene : public Scene
{
public:
	CensusScene( LumosGame* game, CensusSceneData* data );
	virtual ~CensusScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

private:
	void Scan();
	void ScanItem( ItemComponent* ic, const GameItem* );

	LumosGame*			lumosGame;
	ChitBag*			chitBag;
	gamui::PushButton	okay;
	gamui::TextLabel	text;

	// stuff to scan for:
	Wallet	allWallet, 
			mobWallet;

	enum {
		MOB_DENIZEN,
		MOB_GREATER,
		MOB_NORMAL,
		MOB_COUNT
	};

	enum {
		ITEM_PISTOL,
		ITEM_BLASTER,
		ITEM_PULSE,
		ITEM_BEAMGUN,
		ITEM_RING,
		ITEM_SHIELD,
		ITEM_COUNT
	};

	ItemHistory levelActive, 
				levelAny,
				valueActive,
				valueAny,
				killsActive,
				killsAny,
				greaterKillsActive,
				greaterKillsAny,
				craftedActive,
				craftedAny;

	struct Info {
		const GameItem*			item;
		const ItemComponent*	ic;
	};

	Info mobActive[MOB_COUNT];
	Info itemActive[ITEM_COUNT];
};

#endif // CENSUS_SCENE_INCLUDED

