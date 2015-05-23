#include "censusscene.h"
#include "../game/lumoschitbag.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/chitcontext.h"
#include "../script/itemscript.h"
#include "../game/lumosgame.h"
#include "../game/reservebank.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../game/team.h"
#include "../script/corescript.h"
#include "../engine/engine.h"
#include "characterscene.h"

using namespace gamui;
using namespace grinliz;


CensusScene::CensusScene( LumosGame* game, CensusSceneData* d ) : Scene( game ), lumosGame( game ), chitBag(d->chitBag)
{
	InitStd( &gamui2D, &okay, 0 );

	memset( mobActive, 0, sizeof(*mobActive)*MOB_COUNT );
	memset( itemActive, 0, sizeof(*itemActive)*ITEM_COUNT );

	for (int i = 0; i < MAX_ROWS; ++i) {
		link[i].Init(&gamui2D, game->GetButtonLook(0));
		label[i].Init(&gamui2D);
		label[i].SetTab(100);
	}
	for (int i = 0; i < NUM_GROUPS; ++i) {
		static const char* NAME[NUM_GROUPS] = { "Notable", MOB_KILLS, "Greater\n" MOB_KILLS, "Items", "Crafting", "Domains", "Data" };
		radio[i].Init(&gamui2D, game->GetButtonLook(0));
		radio[i].SetText(NAME[i]);
		radio[0].AddToToggleGroup(&radio[i]);
	}
	radio[0].SetDown();
	censusData.Init(&gamui2D);

	Scan();

	// Run through and do the actually census data, place it in the text field.
	GLString simStr;
	simStr.Format("Population\n\n");

	const Census& census = d->chitBag->census;
	for (int i = 0; i < census.MOBItems().Size(); ++i) {
		const Census::MOBItem& mobItem = census.MOBItems()[i];
		simStr.AppendFormat("%s\t%d\n", mobItem.name.safe_str(), mobItem.count);
	}

	simStr.AppendFormat("\n\nDomains\n\n");

	CDynArray<Census::MOBItem> domains;
	for (int j = 0; j < NUM_SECTORS; j++) {
		for (int i = 0; i < NUM_SECTORS; i++) {
			Vector2I sector = { i, j };
			CoreScript* cs = CoreScript::GetCore(sector);
			if (cs && cs->ParentChit()->Team()) {
				int group = Team::Group(cs->ParentChit()->Team());
				IString team = Team::Instance()->TeamName(group);
				Census::MOBItem item;
				item.name = team; item.count = 1;
				
				int index = domains.Find(item);
				if (index >= 0) {
					domains[index].count++;
				}
				else {
					domains.Push(item);
				}
			}
		}
	}

	domains.Sort();
	for (int i = 0; i < domains.Size(); ++i) {
		const Census::MOBItem& item = domains[i];
		simStr.AppendFormat("%s\t%d\n", item.name.safe_str(), item.count);
	}
	simStr.append("\n\nReserve Bank\n\n");
	const Wallet& reserveWallet = *ReserveBank::Instance()->GetWallet();
	simStr.AppendFormat("Au\t%d\nGreen\t%d\nRed\t%d\nBlue\t%d\nViolet\t%d\n", reserveWallet.Gold(), reserveWallet.Crystal(1), reserveWallet.Crystal(1), reserveWallet.Crystal(2), reserveWallet.Crystal(3));


	simStr.append("\n\n");

	censusData.SetText(simStr.safe_str());
	censusData.SetVisible(false);

	DoLayout(true);
}


CensusScene::~CensusScene()
{
}


void CensusScene::Resize()
{
	PositionStd( &okay, 0 );

	LayoutCalculator layout = DefaultLayout();
	//const Screenport& port = lumosGame->GetScreenport();

	for (int i = 0; i < MAX_ROWS; ++i) {
		layout.PosAbs(&link[i], 2, i);
		layout.PosAbs(&label[i], 3, i);
	}
	for (int i = 0; i < NUM_GROUPS; ++i) {
		layout.PosAbs(&radio[i], 0, i);
	}

	layout.PosAbs(&censusData, 3, 0);
	//censusData.SetBounds(port.UIWidth() / 2, 0);
	censusData.SetTab(layout.Width()*2.0f);
}


void CensusScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
	for (int i = 0; i < MAX_ROWS; ++i) {
		if (item == &link[i]) {
			Chit* chit = 0; 
			int itemID = int(intptr_t(link[i].userData));
			itemIDToChitMap.Query(itemID, &chit);
			if (chit && chit->GetItemComponent()) {
				const GameItem* gi = ItemDB::Instance()->Active(itemID);


				CharacterSceneData* csd = new CharacterSceneData(chit->GetItemComponent(), 0, CharacterSceneData::CHARACTER_ITEM, gi);
				game->PushScene(LumosGame::SCENE_CHARACTER, csd);
				break;
			}
		}
	}
	for (int i = 0; i < NUM_GROUPS; ++i) {
		if (item == &radio[i]) {
			DoLayout(false);
		}
	}
}



void CensusScene::ScanItem(ItemComponent* ic, const GameItem* item)
{
	if (item->Intrinsic()) return;

	IString mobIStr = item->keyValues.GetIString(ISC::mob);
	if (!mobIStr.empty()) {
		mobWallet.Add(item->wallet);

		int slot = -1;
		if (mobIStr == ISC::denizen) slot = MOB_DENIZEN;
		else if (mobIStr == ISC::greater) slot = MOB_GREATER;
		else if (mobIStr == ISC::lesser)  slot = MOB_LESSER;

		if (slot >= 0) {
			if (!mobActive[slot].item || item->Traits().Level() > mobActive[slot].item->Traits().Level()) {
				mobActive[slot].item = item;
				mobActive[slot].ic = ic;
			}
		}
	}

	IString itemIStr = item->IName();
	int slot = -1;
	if (itemIStr == ISC::pistol)  slot = ITEM_PISTOL;
	else if (itemIStr == ISC::blaster) slot = ITEM_BLASTER;
	else if (itemIStr == ISC::pulse)   slot = ITEM_PULSE;
	else if (itemIStr == ISC::beamgun) slot = ITEM_BEAMGUN;
	else if (itemIStr == ISC::ring)    slot = ITEM_RING;
	else if (itemIStr == ISC::shield)  slot = ITEM_SHIELD;

	if (slot >= 0) {
		if (!itemActive[slot].item || item->GetValue() > itemActive[slot].item->GetValue()) {
			itemActive[slot].item = item;
			itemActive[slot].ic = ic;
		}
	}

	ItemHistory h;
	h.Set(item);
	GLASSERT(h.itemID);
	itemIDToChitMap.Add(h.itemID, ic->ParentChit());
	AddToHistory(h);
}

void CensusScene::AddToHistory(const ItemHistory& h)
{
	if ( h.kills ) {
		kills.Add(h);
	}
	if ( h.greater ) {
		greaterKills.Add(h);
	}
	if ( h.crafted ) {
		crafted.Add(h);
	}
	if (h.score) {
		domains.Add(h);
	}
	if (h.value) {
		items.Add(h);
	}
}


void CensusScene::Scan()
{
	int nChits = 0;
	CDynArray<Chit*> arr;

	for (int block = 0; block < chitBag->NumBlocks(); ++block) {
		chitBag->GetBlockPtrs(block, &arr);
		nChits += arr.Size();

		for (int i = 0; i < arr.Size(); ++i) {
			ItemComponent* ic = arr[i]->GetItemComponent();
			if (!ic) continue;

			for (int j = 0; j < ic->NumItems(); ++j) {
				const GameItem* item = ic->GetItem(j);
				if (!item) continue;

				allWallet.Add(item->wallet);
				ScanItem(ic, item);
			}
		}
	}
	GLOUTPUT(("Scan nChits=%d\n", nChits));

	ItemDB* itemDB = ItemDB::Instance();
	for (int i = 0; i<itemDB->NumRegistry(); ++i) {
		const ItemHistory& h = itemDB->RegistryByIndex(i);

		const GameItem* hItem = itemDB->Active(h.itemID);
		if (hItem && hItem->hp > 0) {
			// still alive.
			continue;
		}
		AddToHistory(h);
	}
}


void CensusScene::SetItem(int i, const char* prefix, const ItemHistory& itemHistory)
{
	NewsHistory* newsHistory = chitBag->GetNewsHistory();
	str = "\t";
	if (prefix && *prefix) {
		str.Format("%s\t", prefix);
	}
	itemHistory.AppendDesc(&str, newsHistory, "\n\t");

	Chit* chit = 0;
	GLASSERT(itemHistory.itemID);
	itemIDToChitMap.Query(itemHistory.itemID, &chit);
	if (chit) {
		Vector2I sector = ToSector(chit->Position());

		WorldMap* map = chitBag->Context()->worldMap;
		const SectorData& sd = map->GetSectorData(sector);
		str.AppendFormat("at %s", sd.name.safe_str());
	}

	label[i].SetText(str.c_str());
	link[i].SetVisible(chit > 0);
	link[i].userData = (const void*)itemHistory.itemID;
}


void CensusScene::DoLayout(bool first)
{
	ReserveBank* reserve = ReserveBank::Instance();
	const Wallet& reserveWallet = reserve->wallet;
//	NewsHistory* history = chitBag->GetNewsHistory();
	GLString str;

	if (first) {
		GLString debug;

		debug.AppendFormat("Allocated:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", ALL_GOLD, ALL_CRYSTAL_GREEN, ALL_CRYSTAL_RED, ALL_CRYSTAL_BLUE, ALL_CRYSTAL_VIOLET);
		debug.AppendFormat("InPlay:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", allWallet.Gold(), allWallet.Crystal(0), allWallet.Crystal(1), allWallet.Crystal(2), allWallet.Crystal(3));
		debug.AppendFormat("InReserve:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", reserveWallet.Gold(), reserveWallet.Crystal(1), reserveWallet.Crystal(1), reserveWallet.Crystal(2), reserveWallet.Crystal(3));
		debug.AppendFormat("Total:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n",
						   allWallet.Gold() + reserveWallet.Gold(),
						   allWallet.Crystal(0) + reserveWallet.Crystal(0),
						   allWallet.Crystal(1) + reserveWallet.Crystal(1),
						   allWallet.Crystal(2) + reserveWallet.Crystal(2),
						   allWallet.Crystal(3) + reserveWallet.Crystal(3));

		debug.AppendFormat("MOBs:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", mobWallet.Gold(), mobWallet.Crystal(0), mobWallet.Crystal(1), mobWallet.Crystal(2), mobWallet.Crystal(3));
		debug.append("\n");
		GLOUTPUT(("%s", debug.safe_str()));

#ifdef DEBUG
		GLString tfield = censusData.GetText();
		tfield.append(debug);
		censusData.SetText(tfield.safe_str());
#endif
	}

	for (int i = 0; i < MAX_ROWS; ++i) {
		//group[i].SetVisible(false);
		link[i].SetVisible(false);
		label[i].SetText("");
	}

//	static const char* SEP = "\n\t";

	if (radio[GROUP_KILLS].Down()) {
		for (int i = 0; i < Min(kills.Size(), (int)MAX_ROWS); ++i) {
			SetItem(i, "", kills[i]);
		}
	}

	if (radio[GROUP_GREATER_KILLS].Down()) {
		for (int i = 0; i < Min(greaterKills.Size(), (int)MAX_ROWS); ++i) {
			SetItem(i, "", greaterKills[i]);
		}
	}

	if (radio[GROUP_ITEMS].Down()) {
		for (int i = 0; i < Min(items.Size(), (int)MAX_ROWS); ++i) {
			SetItem(i, "", items[i]);
		}
	}

	if (radio[GROUP_CRAFTING].Down()) {
		for (int i = 0; i < Min(crafted.Size(), (int)MAX_ROWS); ++i) {
			SetItem(i, "", crafted[i]);
		}
	}

	if (radio[GROUP_DOMAINS].Down()) {
		for (int i = 0; i < Min(domains.Size(), (int)MAX_ROWS); ++i) {
			SetItem(i, "", domains[i]);
		}
	}

	if (radio[GROUP_NOTABLE].Down()) {
		int count = 0;
		for (int i = 0; i < MOB_COUNT; ++i) {
			static const char* NAME[MOB_COUNT] = { "Denizen", "Greater", "Lesser" };
			if (mobActive[i].item) {
				ItemHistory h;
				h.Set(mobActive[i].item);
				itemIDToChitMap.Add(h.itemID, mobActive[i].ic ? mobActive[i].ic->ParentChit() : 0);
				SetItem(count, NAME[i], h);
				++count;
			}
		}

		for (int i = 0; i < ITEM_COUNT; ++i) {
			static const char* NAME[ITEM_COUNT] = { "Pistol", "Blaster", "Pulse", "Beamgun", "Ring", "Shield" };
			if (itemActive[i].item) {
				ItemHistory h;
				h.Set(itemActive[i].item);
				itemIDToChitMap.Add(h.itemID, itemActive[i].ic ? itemActive[i].ic->ParentChit() : 0);
				SetItem(count, NAME[i], h);
				++count;
			}
		}
	}
	censusData.SetVisible(radio[GROUP_DATA].Down());
}


void CensusScene::HandleHotKey(int value)
{
	/*
	if (value == GAME_HK_SPACE) {
		for (int i = 0; i < greaterKills.Size(); ++i) {
			int id = greaterKills[i].tempID;
			Chit* chit = chitBag->GetChit(id);
			if (chit && chit->GetItemComponent()) {
				CharacterSceneData* csd = new CharacterSceneData(chit->GetItemComponent(), 0, CharacterSceneData::CHARACTER_ITEM, 0, 0);
				game->PushScene(LumosGame::SCENE_CHARACTER, csd);
				break;
			}
		}
	}
	else 
	*/
	{
		super::HandleHotKey(value);
	}
}


