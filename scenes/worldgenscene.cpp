#include "worldgenscene.h"
#include "../xegame/xegamelimits.h"
#include "../game/lumosgame.h"
#include "../engine/surface.h"
#include "../game/worldinfo.h"
#include "../game/worldmap.h"

using namespace grinliz;
using namespace gamui;

WorldGenScene::WorldGenScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, &cancel );

	worldMap = new WorldMap( WorldGen::SIZE, WorldGen::SIZE );
	pix16 = 0;

	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture( "worldGenPreview", MAX_MAP_SIZE, MAX_MAP_SIZE, Surface::RGB16, Texture::PARAM_NONE, this );

	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, texman->GetTexture( "worldGenPreview" ), 
					 0, 1, 1, 0 );	// y-flip: image to texture coordinate conversion
	worldImage.Init( &gamui2D, atom, false );
	worldImage.SetSize( 400, 400 );

	label.Init( &gamui2D );
	tryAgain.Init( &gamui2D, game->GetButtonLook(0) );
	tryAgain.SetText( "Re-try" );

	genState.Clear();
}


WorldGenScene::~WorldGenScene()
{
	TextureManager::Instance()->TextureCreatorInvalid( this );
	delete [] worldMap;
	delete [] pix16;
}


void WorldGenScene::Resize()
{
	const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, &cancel );
	
	float size = port.UIHeight() * 0.75f;
	worldImage.SetSize( size, size );
	worldImage.SetPos( port.UIWidth()*0.5f - size*0.5f, 10.0f );

	label.SetPos( worldImage.X(), worldImage.Y() + worldImage.Height() + 16.f );

	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	layout.PosAbs( &tryAgain, 1, -1 );
}


void WorldGenScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		const char* gameXML = game->GamePath( "game", 0, "xml" );
		const char* mapPNG  = game->GamePath( "map", 0, "png" );
		const char* mapDAT  = game->GamePath( "map", 0, "dat" );
		const char* mapXML  = game->GamePath( "map", 0, "xml" );

		game->DeleteFile( gameXML );
		game->DeleteFile( mapDAT );
		worldMap->Save( mapDAT, mapXML );
		worldMap->SavePNG( mapPNG );
		game->PopScene();
	}
	else if ( item == &cancel ) {
		game->PopScene();
	}
	else if ( item == &tryAgain ) {
		genState.Clear();
	}
}


void WorldGenScene::CreateTexture( Texture* t )
{
	if ( StrEqual( t->Name(), "worldGenPreview" ) ) {

		static const int SIZE2 = WorldGen::SIZE*WorldGen::SIZE; 

		if ( !pix16 ) {
			pix16 = new U16[SIZE2];
		}
		worldMap->Init( worldGen.Land(), worldGen.Color(), featureArr );

		int i=0;
		for( int y=0; y<WorldGen::SIZE; ++y ) {
			for( int x=0; x<WorldGen::SIZE; ++x ) {
				pix16[i++] = Surface::CalcRGB16( worldMap->Pixel( x, y ));
			}
		}
		t->Upload( pix16, SIZE2*sizeof(U16) );
	}
	else {
		GLASSERT( 0 );
	}
}


void WorldGenScene::DoTick( U32 delta )
{
	bool sendTexture = false;

	if ( genState.scanline < WorldGen::SIZE ) {
		okay.SetEnabled( false );
		tryAgain.SetEnabled( false );
	}
	else {
		okay.SetEnabled( true );
		tryAgain.SetEnabled( true );
	}

	if ( genState.scanline == -1 ) {
		Random random;
		random.SetSeedFromTime();
		U32 seed0 = random.Rand();
		U32 seed1 = delta ^ random.Rand();

		worldGen.StartLandAndWater( seed0, seed1 );
		genState.scanline = 0;
	}
	else if ( genState.scanline < WorldGen::SIZE ) {
		clock_t start = clock();
		while( ( genState.scanline < WorldGen::SIZE) && (clock() - start < 30) ) {
			for( int i=0; i<16; ++i ) {
				worldGen.DoLandAndWater( genState.scanline );
				++genState.scanline;
			}
		}
		CStr<16> str;
		str.Format( "%d%%", (int)(100.0f*(float)genState.scanline/(float)WorldGen::SIZE) );
		label.SetText( str.c_str() );
	}
	else if ( genState.scanline == WorldGen::SIZE ) {
		bool okay = worldGen.EndLandAndWater( 0.4f );
		if ( okay ) {
			worldGen.WriteMarker();
			featureArr.Clear();
			worldGen.CalColor( &featureArr );
		}
		if ( okay ) {
			sendTexture = true;
			label.SetText( "Done" );
			++genState.scanline;
		}
		else {
			genState.scanline = -1;	// around again.
		}
	}

	if ( sendTexture ) {
		Texture* t = TextureManager::Instance()->GetTexture( "worldGenPreview" );
		CreateTexture( t );
	}
}

