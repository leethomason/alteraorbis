#include "worldgenscene.h"
#include "../xegame/xegamelimits.h"
#include "../game/lumosgame.h"
#include "../engine/surface.h"

using namespace grinliz;
using namespace gamui;

WorldGenScene::WorldGenScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, &cancel );

	pixels = new U16[MAX_MAP_SIZE*MAX_MAP_SIZE];

	TextureManager* texman = TextureManager::Instance();
	if ( !texman->IsTexture( "worldGenPreview" ) ) {
		texman->CreateTexture( "worldGenPreview", MAX_MAP_SIZE, MAX_MAP_SIZE, Surface::RGB16, Texture::PARAM_NONE, this );
	}

	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, texman->GetTexture( "worldGenPreview" ), 0, 0, 1, 1 );
	worldImage.Init( &gamui2D, atom, false );
	worldImage.SetSize( 400, 400 );

	label.Init( &gamui2D );
	tryAgain.Init( &gamui2D, game->GetButtonLook(0) );
	tryAgain.SetText( "Re-try" );

	scanline = -1;
}


WorldGenScene::~WorldGenScene()
{
	delete [] pixels;
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
		worldGen.Save( "./save/world.png" );
		game->PopScene();
	}
	else if ( item == &cancel ) {
		game->PopScene();
	}
	else if ( item == &tryAgain ) {
		scanline = -1;
	}
}


void WorldGenScene::CreateTexture( Texture* t )
{
	if ( StrEqual( t->Name(), "worldGenPreview" ) ) {

		const U8* land = worldGen.Land();

		for( int j=0; j<WorldGen::SIZE; ++j ) {
			for( int i=0; i<WorldGen::SIZE; ++i ) {
				Color4U8 rgb8;
				if ( land[j*WorldGen::SIZE+i] ) {
					rgb8.Set( 0, 255, 0, 255 );
				}
				else {
					rgb8.Set( 0, 0, 255, 255 );
				}
				U16 rgb16 = Surface::CalcRGB16( rgb8 );
				pixels[j*WorldGen::SIZE+i] = rgb16;
			}
		}
		t->Upload( pixels, MAX_MAP_SIZE*MAX_MAP_SIZE*sizeof(U16) );
	}
	else {
		GLASSERT( 0 );
	}
}


void WorldGenScene::DoTick( U32 delta )
{
	bool sendTexture = false;

	if ( scanline < WorldGen::SIZE ) {
		okay.SetEnabled( false );
		tryAgain.SetEnabled( false );
	}
	else {
		okay.SetEnabled( true );
		tryAgain.SetEnabled( true );
	}

	if ( scanline == -1 ) {
		Random random;
		random.SetSeedFromTime();
		U32 seed0 = random.Rand();
		U32 seed1 = delta ^ random.Rand();

		worldGen.StartLandAndWater( seed0, seed1 );
		scanline = 0;
	}
	else if ( scanline < WorldGen::SIZE ) {
		clock_t start = clock();
		while( (scanline < WorldGen::SIZE) && (clock() - start < 30) ) {
			for( int i=0; i<16; ++i ) {
				worldGen.DoLandAndWater( scanline );
				++scanline;
			}
		}
		CStr<16> str;
		str.Format( "%d%%", (int)(100.0f*(float)scanline/(float)WorldGen::SIZE) );
		label.SetText( str.c_str() );
	}
	else if ( scanline == WorldGen::SIZE ) {
		bool okay = worldGen.EndLandAndWater( 0.4f );
		if ( okay ) {
			sendTexture = true;
			label.SetText( "Done" );
			++scanline;
		}
		else {
			scanline = -1;	// around again.
		}
	}

	if ( sendTexture ) {
		Texture* t = TextureManager::Instance()->GetTexture( "worldGenPreview" );
		CreateTexture( t );
	}
}

