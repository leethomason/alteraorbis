#include "censusscene.h"
#include "../game/lumosgame.h"
#include "../xegame/chitbag.h"

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


void CensusScene::Scan()
{
	text.SetText( "Hello" );

	int nChits = 0;
	int mobAU = 0;
	int au = 0;
	int mobCrystal[NUM_CRYSTAL_TYPES] = { 0 };
	int crystal[NUM_CRYSTAL_TYPES] = { 0 };
	CDynArray<Chit*> arr;

	for( int block=0; block < chitBag->NumBlocks(); ++block ) {
		chitBag->GetBlockPtrs( block, &arr );

		nChits += arr.Size();
	}

	GLString str;
	str.Format( "Chits: %d", nChits );
	text.SetText( str.c_str() );
}


