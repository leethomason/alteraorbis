#include "censusscene.h"
#include "../game/lumosgame.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"
#include "../script/itemscript.h"
#include "../game/reservebank.h"

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
	text.Init( &gamui2D );
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

	layout.PosAbs( &text, 0, 0 );
	float dx = text.X();
	float dy = text.Y();

	text.SetBounds( port.UIWidth() - dx*2.0f, port.UIHeight() - dy*2.0f );
}


void CensusScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
}


void CensusScene::ScanItem( const GameItem* item )
{
	allWallet.Add( item->wallet );
	if ( item->Intrinsic() ) return;

	IString mobIStr = item->keyValues.GetIString( "mob" );
	if ( !mobIStr.empty() ) {
		mobWallet.Add( item->wallet );

		int slot = -1;
		if      ( mobIStr == "denizen" ) slot = MOB_DENIZEN;
		else if ( mobIStr == "greater" ) slot = MOB_GREATER;
		else if ( mobIStr == "normal" )  slot = MOB_NORMAL;

		if ( slot >= 0 ) {
			if ( item->Traits().Level() > mobLevelActive[slot].level ) {
				mobLevelActive[slot].Set( item );
			}
		}
	}

	IString itemIStr = item->IName();
	int slot = -1;
	if (      itemIStr == "pistol" ) slot = ITEM_PISTOL;
	else if ( itemIStr == "blaster" ) slot = ITEM_BLASTER;
	else if ( itemIStr == "pulse" ) slot = ITEM_PULSE;
	else if ( itemIStr == "beamgun" ) slot = ITEM_BEAMGUN;
	else if ( itemIStr == "ring" ) slot = ITEM_RING;
	else if ( itemIStr == "shield" ) slot = ITEM_SHIELD;

	if ( slot >= 0 ) {
		if ( item->Traits().Level() > itemLevelActive[slot].level ) {
			itemLevelActive[slot].Set( item );
		}
	}

	if ( item->Traits().Level() > levelActive.level ) {
		levelActive.Set( item );
	}
	if ( item->GetValue() > valueActive.value ) {
		valueActive.Set( item );
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
			if ( ic ) {

				for( int k=0; k<ic->NumItems(); ++k ) {
					ScanItem( ic->GetItem(k));
				}
			}
		}
	}
	levelAny = levelActive;
	valueAny = valueActive;

	ItemDB* itemDB = ItemDB::Instance();
	for( int i=0; i<itemDB->NumHistory(); ++i ) {
		const ItemHistory& h = itemDB->HistoryByIndex(i);
		if ( h.level > levelAny.level ) {
			levelAny = h;
		}
		if ( h.value > valueAny.value ) {
			valueAny = h;
		}
	}

	ReserveBank* reserve = ReserveBank::Instance();
	const Wallet& reserveWallet = reserve->GetWallet();

	GLString str;
	str.Format( "Chits: %d\n", nChits );
	str.AppendFormat( "Allocated: Au=%d Green=%d Red=%d Blue=%d Violet=%d\n", ALL_GOLD, ALL_CRYSTAL_GREEN, ALL_CRYSTAL_RED, ALL_CRYSTAL_BLUE, ALL_CRYSTAL_VIOLET );
	str.AppendFormat( "InPlay:    Au=%d Green=%d Red=%d Blue=%d Violet=%d\n", allWallet.gold, allWallet.crystal[0], allWallet.crystal[1], allWallet.crystal[2], allWallet.crystal[3] );
	str.AppendFormat( "InReserve: Au=%d Green=%d Red=%d Blue=%d Violet=%d\n", reserveWallet.gold, reserveWallet.crystal[0], reserveWallet.crystal[1], reserveWallet.crystal[2], reserveWallet.crystal[3] );
	str.AppendFormat( "Total:     Au=%d Green=%d Red=%d Blue=%d Violet=%d\n", 
		allWallet.gold + reserveWallet.gold, 
		allWallet.crystal[0] + reserveWallet.crystal[0], 
		allWallet.crystal[1] + reserveWallet.crystal[1], 
		allWallet.crystal[2] + reserveWallet.crystal[2], 
		allWallet.crystal[3] + reserveWallet.crystal[3] );

	str.AppendFormat( "MOBs: Au=%d Green=%d Red=%d Blue=%d Violet=%d\n", mobWallet.gold, mobWallet.crystal[0], mobWallet.crystal[1], mobWallet.crystal[2], mobWallet.crystal[3] );
	str.append( "Max Level, alive: " );			levelActive.AppendDesc( &str );	str.append( "\n" );
	str.append( "Max Level, historical: " );	levelAny.AppendDesc( &str ); str.append( "\n" );
	str.append( "Max Value, alive: " );			valueActive.AppendDesc( &str ); str.append( "\n" );
	str.append( "Max Value, historical: " );	valueAny.AppendDesc( &str ); str.append( "\n" );

	for( int i=0; i<MOB_COUNT; ++i ) {
		static const char* NAME[MOB_COUNT] = { "Denizen", "Greater", "Lesser" };
		if ( mobLevelActive[i].itemID ) {
			str.AppendFormat( "%s: ", NAME[i] );
			mobLevelActive[i].AppendDesc( &str );
			str.append( "\n" );
		}
	}

	for( int i=0; i<ITEM_COUNT; ++i ) {
		static const char* NAME[ITEM_COUNT] = { "Pistol", "Blaster", "Pulse", "Beamgun", "Ring", "Shield" };
		if ( itemLevelActive[i].itemID ) {
			str.AppendFormat( "%s: ", NAME[i] );
			itemLevelActive[i].AppendDesc( &str );
			str.append( "\n" );
		}
	}

	text.SetText( str.c_str() );
}


