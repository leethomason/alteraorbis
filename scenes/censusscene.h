#ifndef CENSUS_SCENE_INCLUDED
#define CENSUS_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../script/itemscript.h"
#include "../grinliz/glstringutil.h"

class LumosGame;
class ItemComponent;
class LumosChitBag;
class GameItem;

struct ItemHistoryScore
{
	// sort descending
	static bool Equal(const ItemHistory& a, const ItemHistory& b) { return a.score == b.score; }
	static bool Less(const ItemHistory& a, const ItemHistory& b) { return a.score > b.score; }
};


struct ItemHistoryKills
{
	// sort descending
	static bool Equal(const ItemHistory& a, const ItemHistory& b) { return a.kills == b.kills; }
	static bool Less(const ItemHistory& a, const ItemHistory& b) { return a.kills > b.kills; }
};


struct ItemHistoryGreaterKills
{
	// sort descending
	static bool Equal(const ItemHistory& a, const ItemHistory& b) { return a.greater == b.greater; }
	static bool Less(const ItemHistory& a, const ItemHistory& b) { return a.greater > b.greater; }
};


struct ItemHistoryCrafting
{
	// sort descending
	static bool Equal(const ItemHistory& a, const ItemHistory& b) { return a.crafted == b.crafted; }
	static bool Less(const ItemHistory& a, const ItemHistory& b) { return a.crafted > b.crafted; }
};


struct ItemHistoryItems
{
	// sort descending
	static bool Equal(const ItemHistory& a, const ItemHistory& b) { return a.value == b.value; }
	static bool Less(const ItemHistory& a, const ItemHistory& b) { return a.value > b.value; }
};


class CensusSceneData : public SceneData
{
public:
	CensusSceneData( LumosChitBag* _chitBag )				// apply markup, costs
		: SceneData(), chitBag(_chitBag) { GLASSERT(chitBag); }

	LumosChitBag* chitBag;
};


class CensusScene : public Scene
{
	typedef Scene super;

public:
	CensusScene( LumosGame* game, CensusSceneData* data );
	virtual ~CensusScene();

	virtual void Resize();

	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		return ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void HandleHotKey( int value );

private:
	void Scan();
	void ScanItem( ItemComponent* ic, const GameItem* );
	void DoLayout(bool first);
	void SetItem(int i, const char* label, const ItemHistory& history);

	enum {
		MOB_DENIZEN,
		MOB_GREATER,
		MOB_LESSER,
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

	enum {
		GROUP_NOTABLE,
		GROUP_KILLS,
		GROUP_GREATER_KILLS,
		GROUP_ITEMS,
		GROUP_CRAFTING,
		GROUP_DOMAINS,
		GROUP_DATA,
		NUM_GROUPS
	};

	enum {
		MAX_ROWS = ITEM_COUNT + MOB_COUNT
	};

	LumosGame*			lumosGame;
	LumosChitBag*		chitBag;
	grinliz::GLString	str;		// avoid allocations
	gamui::PushButton	okay;
	gamui::PushButton	link[MAX_ROWS];
	gamui::TextLabel	label[MAX_ROWS];
	gamui::ToggleButton	radio[NUM_GROUPS];
	gamui::TextLabel	censusData;

	grinliz::HashTable<int, Chit*>	itemIDToChitMap;

	// stuff to scan for:
	TransactAmt	allWallet, mobWallet;

	void AddToHistory(const ItemHistory& h);
	grinliz::SortedDynArray<ItemHistory, grinliz::ValueSem, ItemHistoryScore> domains;
	grinliz::SortedDynArray<ItemHistory, grinliz::ValueSem, ItemHistoryKills> kills;
	grinliz::SortedDynArray<ItemHistory, grinliz::ValueSem, ItemHistoryGreaterKills> greaterKills;
	grinliz::SortedDynArray<ItemHistory, grinliz::ValueSem, ItemHistoryCrafting> crafted;
	grinliz::SortedDynArray<ItemHistory, grinliz::ValueSem, ItemHistoryItems> items;

	struct Info {
		const GameItem*			item;
		const ItemComponent*	ic;
	};

	Info mobActive[MOB_COUNT];
	Info itemActive[ITEM_COUNT];
};

#endif // CENSUS_SCENE_INCLUDED

