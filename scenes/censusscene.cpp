#include "censusscene.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../script/itemscript.h"
#include "../game/lumosgame.h"
#include "../game/reservebank.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../engine/engine.h"

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

	memset( mobActive, 0, sizeof(*mobActive)*MOB_COUNT );
	memset( itemActive, 0, sizeof(*itemActive)*ITEM_COUNT );

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
	text.SetTab( text.Width() * 0.3f );
}


void CensusScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
}


void CensusScene::ScanItem( ItemComponent* ic, const GameItem* item )
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
			if ( !mobActive[slot].item || item->Traits().Level() > mobActive[slot].item->Traits().Level() ) {
				mobActive[slot].item = item;
				mobActive[slot].ic = ic;
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
		if ( !itemActive[slot].item || item->GetValue() > itemActive[slot].item->GetValue() ) {
			itemActive[slot].item = item;
			itemActive[slot].ic = ic;
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
					ScanItem( ic, ic->GetItem(k));
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
	str.AppendFormat( "Allocated:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", ALL_GOLD, ALL_CRYSTAL_GREEN, ALL_CRYSTAL_RED, ALL_CRYSTAL_BLUE, ALL_CRYSTAL_VIOLET );
	str.AppendFormat( "InPlay:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", allWallet.gold, allWallet.crystal[0], allWallet.crystal[1], allWallet.crystal[2], allWallet.crystal[3] );
	str.AppendFormat( "InReserve:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", reserveWallet.gold, reserveWallet.crystal[0], reserveWallet.crystal[1], reserveWallet.crystal[2], reserveWallet.crystal[3] );
	str.AppendFormat( "Total:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", 
		allWallet.gold + reserveWallet.gold, 
		allWallet.crystal[0] + reserveWallet.crystal[0], 
		allWallet.crystal[1] + reserveWallet.crystal[1], 
		allWallet.crystal[2] + reserveWallet.crystal[2], 
		allWallet.crystal[3] + reserveWallet.crystal[3] );

	str.AppendFormat( "MOBs:\tAu=%d Green=%d Red=%d Blue=%d Violet=%d\n", mobWallet.gold, mobWallet.crystal[0], mobWallet.crystal[1], mobWallet.crystal[2], mobWallet.crystal[3] );
	str.append( "Level, alive:\t" );			levelActive.AppendDesc( &str );	str.append( "\n" );
	str.append( "Level, historical:\t" );	levelAny.AppendDesc( &str ); str.append( "\n" );
	str.append( "Value, alive:\t" );			valueActive.AppendDesc( &str ); str.append( "\n" );
	str.append( "Value, historical:\t" );	valueAny.AppendDesc( &str ); str.append( "\n" );

	for( int i=0; i<MOB_COUNT; ++i ) {
		static const char* NAME[MOB_COUNT] = { "Denizen", "Greater", "Lesser" };
		if ( mobActive[i].item ) {
			ItemHistory h;
			h.Set( mobActive[i].item );

			str.AppendFormat( "%s:\t", NAME[i] );
			h.AppendDesc( &str );

			Chit* chit = mobActive[i].ic->ParentChit();
			SpatialComponent* sc = chit->GetSpatialComponent();
			if ( sc ) {
				Vector2I sector = ToSector( sc->GetPosition2DI() );

				WorldMap* map = Engine::Instance()->GetMap()->ToWorldMap();
				const SectorData& sd = map->GetSector( sector );
				str.AppendFormat( " at %s", sd.name.c_str() );
			}

			str.append( "\n" );
		}
	}

	for( int i=0; i<ITEM_COUNT; ++i ) {
		static const char* NAME[ITEM_COUNT] = { "Pistol", "Blaster", "Pulse", "Beamgun", "Ring", "Shield" };
		if ( itemActive[i].item ) {
			ItemHistory h;
			h.Set( itemActive[i].item );

			str.AppendFormat( "%s:\t", NAME[i] );
			h.AppendDesc( &str );

			const GameItem* mainItem = itemActive[i].ic->GetItem(0);
			IString mob = mainItem->keyValues.GetIString( "mob" );
			if ( !mob.empty() ) {
				str.AppendFormat( " wielded by %s", mainItem->BestName() );
			}
			str.append( "\n" );
		}
	}

	text.SetText( str.c_str() );
}


