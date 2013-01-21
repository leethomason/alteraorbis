#include "worldgenscene.h"
#include "../xegame/xegamelimits.h"
#include "../game/lumosgame.h"
#include "../engine/surface.h"
#include "../game/worldinfo.h"
#include "../game/worldmap.h"
#include "../script/rockgen.h"

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


void WorldGenScene::BlendZone( int zone )
{
	int y = genState.zone / NZONE;
	int x = genState.zone - y*NZONE;

	Random random;
	random.SetSeedFromTime();

	const int rockArr[3] = { RockGen::CAVEY_ROUGH, RockGen::CAVEY_SMOOTH, RockGen::BOULDERY };
	const float rockScore[3] = {
		(float)(NZONE-1-y),
		0.5f*(float)(NZONE-1-y+x),
		(float)y
	};
	int rock = rockArr[ random.Select( rockScore, 3 ) ];

	float fractionLand = (x==0 || x==(NZONE-1) || y ==0 || y==(NZONE-1)) ? 0.55f : 0.85f;

	const int heightArr[3] = { RockGen::NOISE_HEIGHT, RockGen::USE_HIGH_HEIGHT, RockGen::KEEP_HEIGHT };
	const float heightScore[3] = { 0.8f, 1.0f, 0.5f };
	const int height = heightArr[ random.Select( heightScore, 4 ) ];
	const bool blendExisting = (random.Rand(3) == 0);

	static const int BORDER = 16;
	static const int S  = WorldGen::SIZE / NZONE;
	static const int SB = S + BORDER*2;
	
	RockGen rockGen( SB );
	rockGen.DoCalc( random.Rand(), rock );
	rockGen.DoThreshold( random.Rand(), fractionLand, height );

	int x0 = x*S - BORDER;
	int y0 = y*S - BORDER;

	Rectangle2I bounds;
	bounds.Set( 0, 0, WorldGen::SIZE-1, WorldGen::SIZE-1 );

	for( int j=0; j<SB; ++j ) {
		for( int i=0; i<SB; ++i ) {
			Vector2I dst = { x0 + i, y0 + j };
			Vector2I src = { i, j };

			if ( bounds.Contains( dst ) ) {
				int delta = Min( i, j, SB-1-i, SB-1-j );
				int blend = 256;
				if ( delta < BORDER ) {
					blend = 256*delta/BORDER;
				}
				if ( blendExisting ) {
					blend /= 2;
				}
				worldGen.ApplyHeight( dst.x, dst.y, 
					                  *(rockGen.Height() + src.y*SB + src.x),
									  blend );
			}
		}
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
	else if ( genState.scanline == WorldGen::SIZE && genState.zone < 0 ) {
		bool okay = worldGen.EndLandAndWater( 0.4f );
		if ( okay ) {
			worldGen.WriteMarker();
			featureArr.Clear();
			worldGen.CalColor( &featureArr );
		}
		if ( okay ) {
			sendTexture = true;
			label.SetText( "Land/Water\nDone" );
			genState.zone = 0;
		}
		else {
			genState.Clear();	// around again.
		}
	}
	else if ( genState.scanline == WorldGen::SIZE && genState.zone < NUM_ZONES ) {
		BlendZone( genState.zone );
		sendTexture = true;
		genState.zone++;
		if ( genState.zone == NUM_ZONES ) {
			label.SetText( "Done" );
		}
	}

	if ( sendTexture ) {
		Texture* t = TextureManager::Instance()->GetTexture( "worldGenPreview" );
		CreateTexture( t );
	}
}

