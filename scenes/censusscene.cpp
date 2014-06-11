#include "censusscene.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"
#include "../script/itemscript.h"
#include "../game/lumosgame.h"
#include "../game/reservebank.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../engine/engine.h"
#include "characterscene.h"

using namespace gamui;
using namespace grinliz;

/*	
	Money in world.
	Money in MOBs.
	Highest [Living, Dead] [MOB, Greater, Lesser, Denizen] by [Level,Age]
	Highest [Living, Dead] [Pistol, Blaster, Pulse, Beamgun, Shield, Ring] by [Level]

*/


CensusScene::CensusScene( LumosGame* game, CensusSceneData* d ) : Scene( game ), lumosGame( game ), chitBag(d->chitBag)
{
	lumosGame->InitStd( &gamui2D, &okay, 0 );

	memset( mobActive, 0, sizeof(*mobActive)*MOB_COUNT );
	memset( itemActive, 0, sizeof(*itemActive)*ITEM_COUNT );

	for (int i = 0; i < MAX_BUTTONS; ++i) {
		link[i].Init(&gamui2D, game->GetButtonLook(0));
		label[i].Init(&gamui2D);
		label[i].SetTab(100);
		group[i].Init(&gamui2D);
	}

	Scan();
}


CensusScene::~CensusScene()
{
}


void CensusScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	const Screenport& port = lumosGame->GetScreenport();

	// --- half size --- //
	layout.SetSize(layout.Width(), 0.5f*layout.Height());

	for (int i = 0; i < MAX_BUTTONS; ++i) {
		layout.PosAbs(&group[i], 1, i + 1);
		layout.PosAbs(&link[i], 2, i + 1);
		layout.PosAbs(&label[i], 3, i + 1);
	}
}


void CensusScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
	for (int i = 0; i < MAX_BUTTONS; ++i) {
		if (item == &link[i]) {
			Chit* chit = chitBag->GetChit((int)link[i].userData);
			if (chit && chit->GetItemComponent()) {
				CharacterSceneData* csd = new CharacterSceneData(chit->GetItemComponent(), 0, CharacterSceneData::CHARACTER_ITEM, 0);
				game->PushScene(LumosGame::SCENE_CHARACTER, csd);
				break;
			}
		}
	}
}



void CensusScene::ScanItem(ItemComponent* ic, const GameItem* item)
{
	if (item->Intrinsic()) return;

	IString mobIStr = item->keyValues.GetIString("mob");
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
	h.tempID = ic->ParentChit()->ID();
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
}


void CensusScene::Scan()
{
	int nChits = 0;
	CDynArray<Chit*> arr;

	for( int block=0; block < chitBag->NumBlocks(); ++block ) {
		chitBag->GetBlockPtrs( block, &arr );
		nChits += arr.Size();

		for ( int i=0; i<arr.Size(); ++i ) {
			ItemComponent* ic = arr[i]->GetItemComponent();
			if (!ic) continue;

			for (int j = 0; j < ic->NumItems(); ++j) {
				GameItem* item = ic->GetItem(j);
				if (!item) continue;

				allWallet.Add(item->wallet);
				ScanItem(ic, item);
			}
		}
	}

	ItemDB* itemDB = ItemDB::Instance();
	for( int i=0; i<itemDB->NumHistory(); ++i ) {
		const ItemHistory& h = itemDB->HistoryByIndex(i);

		const GameItem* hItem = itemDB->Find( h.itemID );
		if ( hItem && hItem->hp > 0 ) {
			// still alive.
			continue;
		}
		AddToHistory(h);
	}

	ReserveBank* reserve = ReserveBank::Instance();
	const Wallet& reserveWallet = reserve->bank;
	NewsHistory* history = chitBag->GetNewsHistory();

	GLString debug;
	GLString str;
	debug.Format( "Chits: %d\n", nChits );
	debug.AppendFormat( "Allocated:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", ALL_GOLD, ALL_CRYSTAL_GREEN, ALL_CRYSTAL_RED, ALL_CRYSTAL_BLUE, ALL_CRYSTAL_VIOLET );
	debug.AppendFormat( "InPlay:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", allWallet.gold, allWallet.crystal[0], allWallet.crystal[1], allWallet.crystal[2], allWallet.crystal[3] );
	debug.AppendFormat( "InReserve:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", reserveWallet.gold, reserveWallet.crystal[0], reserveWallet.crystal[1], reserveWallet.crystal[2], reserveWallet.crystal[3] );
	debug.AppendFormat( "Total:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", 
		allWallet.gold + reserveWallet.gold, 
		allWallet.crystal[0] + reserveWallet.crystal[0], 
		allWallet.crystal[1] + reserveWallet.crystal[1], 
		allWallet.crystal[2] + reserveWallet.crystal[2], 
		allWallet.crystal[3] + reserveWallet.crystal[3] );

	debug.AppendFormat( "MOBs:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", mobWallet.gold, mobWallet.crystal[0], mobWallet.crystal[1], mobWallet.crystal[2], mobWallet.crystal[3] );
	debug.append( "\n" );

	int count = 0;

	for (int i = 0; i < MAX_BUTTONS; ++i) {
		group[i].SetVisible(false);
		link[i].SetVisible(false);
	}

	group[count].SetText("Kills:");
	group[count].SetVisible(true);
	for (int i = 0; i < Min(kills.Size(), (int)MAX_ROWS); ++i) {
		str = "";
		kills[i].AppendDesc(&str, history);
		label[count].SetText(str.c_str());
		link[count].SetVisible(true);
		link[count].SetEnabled(kills[i].tempID > 0);
		link[count].userData = (const void*)kills[i].tempID;
		count++;
	}

	++count;
	group[count].SetText( "Greater Kills:" );
	group[count].SetVisible(true);
	for (int i = 0; i < Min(greaterKills.Size(), (int)MAX_ROWS); ++i) {
		str = "";
		greaterKills[i].AppendDesc(&str, history);
		label[count].SetText(str.c_str());
		link[count].SetVisible(true);
		link[count].SetEnabled(greaterKills[i].tempID > 0);
		link[count].userData = (const void*)greaterKills[i].tempID;
		++count;
	}

	++count;
	group[count].SetText( "Crafting:" );
	group[count].SetVisible(true);
	for (int i = 0; i < Min(crafted.Size(), (int)MAX_ROWS); ++i) {
		str = "";
		crafted[i].AppendDesc(&str, history);
		label[count].SetText(str.c_str());
		link[count].SetVisible(true);
		link[count].SetEnabled(crafted[i].tempID > 0);
		link[count].userData = (const void*)crafted[i].tempID;
		++count;
	}

	++count;
	group[count].SetText("Domains:");
	group[count].SetVisible(true);
	for (int i = 0; i < Min(domains.Size(), (int)MAX_ROWS); ++i) {
		str = "";
		domains[i].AppendDesc(&str, history);
		label[count].SetText(str.c_str());
		link[count].SetVisible(true);
		link[count].SetEnabled(domains[i].tempID > 0);
		link[count].userData = (const void*)domains[i].tempID;
		++count;
	}

	++count;
	group[count].SetText("Notable:");
	group[count].SetVisible(true);
	for( int i=0; i<MOB_COUNT; ++i ) {
		static const char* NAME[MOB_COUNT] = { "Denizen", "Greater", "Lesser" };
		if ( mobActive[i].item ) {
			ItemHistory h;
			h.Set( mobActive[i].item );
			h.tempID = mobActive[i].ic ? mobActive[i].ic->ParentChit()->ID() : 0;

			str = "";
			str.AppendFormat( "%s\t", NAME[i] );
			h.AppendDesc( &str, history );

			Chit* chit = mobActive[i].ic->ParentChit();
			SpatialComponent* sc = chit->GetSpatialComponent();
			if ( sc ) {
				Vector2I sector = ToSector( sc->GetPosition2DI() );

				WorldMap* map = Engine::Instance()->GetMap()->ToWorldMap();
				const SectorData& sd = map->GetSector( sector );
				str.AppendFormat( " at %s", sd.name.safe_str() );
			}
			label[count].SetText(str.c_str());
			link[count].SetVisible(true);
			link[count].SetEnabled(h.tempID > 0);
			link[count].userData = (const void*)h.tempID;
			++count;
		}
	}

	for( int i=0; i<ITEM_COUNT; ++i ) {
		static const char* NAME[ITEM_COUNT] = { "Pistol", "Blaster", "Pulse", "Beamgun", "Ring", "Shield" };
		if ( itemActive[i].item ) {
			ItemHistory h;
			h.Set( itemActive[i].item );
			h.tempID = itemActive[i].ic ? itemActive[i].ic->ParentChit()->ID() : 0;

			str = "";
			str.AppendFormat( "%s\t", NAME[i] );
			h.AppendDesc( &str, history );

			const GameItem* mainItem = itemActive[i].ic->GetItem(0);
			IString mob = mainItem->keyValues.GetIString( "mob" );
			if ( !mob.empty() ) {
				str.AppendFormat( " wielded by %s", mainItem->BestName() );
			}
			label[count].SetText(str.c_str());
			link[count].SetVisible(true);
			link[count].SetEnabled(h.tempID > 0);
			link[count].userData = (const void*)h.tempID;
			++count;
		}
	}

//	text.SetText( str.safe_str() );
	GLASSERT(count <= MAX_BUTTONS);
	for (int i = count; i < MAX_BUTTONS; ++i) {
		label[i].SetVisible(false);
	}

	GLOUTPUT(("%s", debug.safe_str()));
}


void CensusScene::HandleHotKey(int value)
{
	if (value == GAME_HK_SPACE) {
		for (int i = 0; i < greaterKills.Size(); ++i) {
			int id = greaterKills[i].tempID;
			Chit* chit = chitBag->GetChit(id);
			if (chit && chit->GetItemComponent()) {
				CharacterSceneData* csd = new CharacterSceneData(chit->GetItemComponent(), 0, CharacterSceneData::CHARACTER_ITEM, 0);
				game->PushScene(LumosGame::SCENE_CHARACTER, csd);
				break;
			}
		}
	}
	else {
		super::HandleHotKey(value);
	}
}


