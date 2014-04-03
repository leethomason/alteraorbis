#include "worldgenscene.h"
#include "../xegame/xegamelimits.h"
#include "../game/lumosgame.h"
#include "../engine/surface.h"
#include "../game/worldinfo.h"
#include "../game/worldmap.h"
#include "../script/rockgen.h"
#include <time.h>

using namespace grinliz;
using namespace gamui;

WorldGenScene::WorldGenScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, &cancel );

	worldMap = new WorldMap( WorldGen::SIZE, WorldGen::SIZE );
	pix16 = 0;

	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture( "worldGenPreview", MAX_MAP_SIZE, MAX_MAP_SIZE, Surface::RGB16, Texture::PARAM_NONE, this );

	worldGen = new WorldGen();
	worldGen->LoadFeatures( "./res/features.png" );

	rockGen = new RockGen( WorldGen::SIZE );

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
	delete worldGen;
	delete rockGen;
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
		const char* gameXML = game->GamePath( "game", 0, "dat" );
		const char* mapPNG  = game->GamePath( "map", 0, "png" );
		const char* mapDAT  = game->GamePath( "map", 0, "dat" );

		game->DeleteFile( gameXML );
		game->DeleteFile( mapDAT );
		worldMap->Save( mapDAT );
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
		// Must also set SectorData, which is done elsewhere.
		worldMap->MapInit( worldGen->Land(), worldGen->Path() );

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


void WorldGenScene::BlendLine( int y )
{
	for( int x=0; x<WorldGen::SIZE; ++x ) {
		int h = *(worldGen->Land()  + y*WorldGen::SIZE + x);
		int r = *(rockGen->Height() + y*WorldGen::SIZE + x);

		if ( h >= WorldGen::LAND0 && h <= WorldGen::LAND3 ) {
			if ( r ) {
				// It is land. The World land is the minimum;
				// apply the rockgen value.

				h = Max( h, WorldGen::LAND1 + r / 102 );
				//h = WorldGen::LAND1 + r / 102;
				GLASSERT( h >= WorldGen::LAND0 && h <= WorldGen::LAND3 );
			}
			else {
				h = WorldGen::LAND0;
			}
		}
		worldGen->SetHeight( x, y, h );
	}
}


void WorldGenScene::DoTick( U32 delta )
{
	bool sendTexture = false;

	switch ( genState.mode ) {
	case GenState::NOT_STARTED:
		{
			okay.SetEnabled( false );
			tryAgain.SetEnabled( false );

			Random random;
			random.SetSeedFromTime();
			U32 seed0 = random.Rand();
			U32 seed1 = delta ^ random.Rand();

			worldGen->StartLandAndWater( seed0, seed1 );
			genState.y = 0;
			genState.mode = GenState::WORLDGEN;
		}
		break;
	case GenState::WORLDGEN:
		{
			clock_t start = clock();
			while( ( genState.y < WorldGen::SIZE) && (clock() - start < 30) ) {
				for( int i=0; i<16; ++i ) {
					worldGen->DoLandAndWater( genState.y++ );
				}
			}
			CStr<16> str;
			str.Format( "Land: %d%%", (int)(100.0f*(float)genState.y/(float)WorldGen::SIZE) );
			label.SetText( str.c_str() );
			GLString name;

			if ( genState.y == WorldGen::SIZE ) {
				bool okay = worldGen->EndLandAndWater( 0.4f );
				if ( okay ) {
					worldGen->WriteMarker();
					SectorData* sectorData = worldMap->GetWorldInfoMutable()->SectorDataMemMutable();
					Random random;
					random.SetSeedFromTime();;

					worldGen->CutRoads( random.Rand(), sectorData );
					worldGen->ProcessSectors( random.Rand(), sectorData );

					GLString name;
					GLString postfix;

					for( int j=0; j<NUM_SECTORS; ++j ) {
						for( int i=0; i<NUM_SECTORS; ++i ) {
							name = "sector";
							// Keep the names a little short, so that they don't overflow UI.
							const char* n = static_cast<LumosGame*>(game)->GenName( "sector", random.Rand(), 4, 7 );
							GLASSERT( n );
							if ( n ) {
								name = n;
							}
							GLASSERT( NUM_SECTORS == 16 );	// else the printing below won't be correct.
							postfix = "";
							postfix.Format( "-%x%x", i, j );
							name += postfix;
							sectorData[j*NUM_SECTORS+i].name = StringPool::Intern( name.c_str() );
						}
					}

					sendTexture = true;
					genState.mode = GenState::ROCKGEN_START;
				}
				else {
					genState.y = 0;
					genState.mode = GenState::WORLDGEN;
				}
			}	
		}
		break;

	case GenState::ROCKGEN_START:
		{
			Random random;
			random.SetSeedFromTime();
			rockGen->StartCalc( random.Rand() );
			genState.y = 0;
			genState.mode = GenState::ROCKGEN;
		}
		break;

	case GenState::ROCKGEN:
		{
			clock_t start = clock();
			while( ( genState.y < WorldGen::SIZE) && (clock() - start < 30) ) {
				for( int i=0; i<16; ++i ) {
					rockGen->DoCalc( genState.y );
					genState.y++;
				}
			}
			CStr<16> str;
			str.Format( "Rock: %d%%", (int)(100.0f*(float)genState.y/(float)WorldGen::SIZE) );
			label.SetText( str.c_str() );

			if ( genState.y == WorldGen::SIZE ) {
				rockGen->EndCalc();

				Random random;
				random.SetSeedFromTime();

				rockGen->DoThreshold( random.Rand(), 0.35f, RockGen::NOISE_HEIGHT );
				for( int y=0; y<WorldGen::SIZE; ++y ) {
					BlendLine( y );
				}
				sendTexture = true;
				genState.Clear();
				genState.mode = GenState::DONE;
				label.SetText( "Done" );
			}
		}
		break;

	case GenState::DONE:
		{
			okay.SetEnabled( true );
			tryAgain.SetEnabled( true );
		}
		break;

	default:
		GLASSERT( 0 );
		break;
	}

	if ( sendTexture ) {
		Texture* t = TextureManager::Instance()->GetTexture( "worldGenPreview" );
		CreateTexture( t );
	}
}

