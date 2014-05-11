#include "worldgenscene.h"

#include "../xegame/xegamelimits.h"

#include "../engine/surface.h"
#include "../engine/settings.h"

#include "../game/lumosgame.h"
#include "../game/lumoschitbag.h"
#include "../game/worldinfo.h"
#include "../game/worldmap.h"
#include "../game/sim.h"

#include "../script/rockgen.h"
#include <time.h>

using namespace grinliz;
using namespace gamui;

// FIXME clock() assumes milliseconds

WorldGenScene::WorldGenScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, &cancel );
	sim = 0;

	worldMap = new WorldMap( WorldGen::SIZE, WorldGen::SIZE );
	pix16 = 0;

	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture( "worldGenPreview", MAX_MAP_SIZE, MAX_MAP_SIZE, TEX_RGB16, Texture::PARAM_NONE, this );

	worldGen = new WorldGen();
	worldGen->LoadFeatures( "./res/features.png" );

	rockGen = new RockGen( WorldGen::SIZE );

	RenderAtom atom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, texman->GetTexture( "worldGenPreview" ), 
					 0, 1, 1, 0 );	// y-flip: image to texture coordinate conversion
	worldImage.Init( &gamui2D, atom, false );
	worldImage.SetSize( 400, 400 );

	worldText.Init(&gamui2D);
	worldText.SetBounds(400, 400);

	label.Init( &gamui2D );
	genState.Clear();
}


WorldGenScene::~WorldGenScene()
{
	TextureManager::Instance()->TextureCreatorInvalid( this );
	delete [] worldMap;
	delete [] pix16;
	delete worldGen;
	delete rockGen;
	delete sim;
}


void WorldGenScene::Resize()
{
	const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, &cancel );
	
	float size = port.UIHeight() * 0.75f;
	worldImage.SetSize( size, size );
	worldImage.SetPos( port.UIWidth()*0.5f - size*0.5f, 10.0f );

	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	label.SetPos(worldImage.X(), worldImage.Y() + worldImage.Height() + 16.f);

	worldText.SetPos(worldImage.X() + layout.GutterX(), worldImage.Y() + layout.GutterY());
	worldText.SetBounds(size - layout.GutterX()*2.0f, size - layout.GutterY()*2.0f);
}


void WorldGenScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		if (genState.mode == GenState::GEN_NOTES) {
			genState.mode = GenState::WORLDGEN;
			okay.SetEnabled(false);
			cancel.SetEnabled(false);
		}
		else {
			game->PopScene();
		}
	}
	else if ( item == &cancel ) {
		game->PopScene();
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


void WorldGenScene::SetMapBright(bool b)
{
	RenderAtom atom((const void*)(b ? UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE : UIRenderer::RENDERSTATE_UI_DISABLED), 
		TextureManager::Instance()->GetTexture("worldGenPreview"),
		0, 1, 1, 0);	// y-flip: image to texture coordinate conversion
	worldImage.SetAtom(atom);
}

void WorldGenScene::DoTick( U32 delta )
{
	bool sendTexture = false;

	switch (genState.mode) {
	case GenState::NOT_STARTED:
	{
		Random random;
		random.SetSeedFromTime();
		U32 seed0 = random.Rand();
		U32 seed1 = delta ^ random.Rand();

		worldGen->StartLandAndWater(seed0, seed1);
		genState.y = 0;
		genState.mode = GenState::GEN_NOTES;
	}
		break;

	case GenState::GEN_NOTES:
	{
		SetMapBright(false);
		okay.SetEnabled(true);
		cancel.SetEnabled(true);

		worldText.SetText("Welcome to world generation!\n\n"
			"This process will take a few minutes (grab some water "
			"or coffee) while the world is generated. This will delete any game in progress. Once generated, "
			"the world persists and you can play a game, with time passing and domains rising and falling, for "
			"as long as you wish.\n\n"
			"World generation plays the 1st Age: The Age of Fire, from date 0.00 to 1.00.\n\n"
			"Altera's world is shaped by 2 opposing forces: volcanos creating land, and rampaging monsters "
			"destroying land. There are few monsters during the Age of Fire, so at the end of world generation "
			"rock will dominate the landscape. Over time, the forces of motion and rock will balance.\n\n"
			"Tap 'okay' to generate or world or 'cancel' to return to title.");
	}
		break;

	case GenState::WORLDGEN:
	{
		worldText.SetText("");
		clock_t start = clock();
		while ((genState.y < WorldGen::SIZE) && (clock() - start < 30)) {
			for (int i = 0; i < 16; ++i) {
				worldGen->DoLandAndWater(genState.y++);
			}
		}
		CStr<16> str;
		str.Format("Land: %d%%", (int)(100.0f*(float)genState.y / (float)WorldGen::SIZE));
		label.SetText(str.c_str());
		GLString name;

		if (genState.y == WorldGen::SIZE) {
			SetMapBright(true);
			bool okay = worldGen->EndLandAndWater(0.4f);
			if (okay) {
				worldGen->WriteMarker();
				SectorData* sectorData = worldMap->GetWorldInfoMutable()->SectorDataMemMutable();
				Random random;
				random.SetSeedFromTime();;

				worldGen->CutRoads(random.Rand(), sectorData);
				worldGen->ProcessSectors(random.Rand(), sectorData);

				GLString name;
				GLString postfix;

				for (int j = 0; j < NUM_SECTORS; ++j) {
					for (int i = 0; i < NUM_SECTORS; ++i) {
						name = "sector";
						// Keep the names a little short, so that they don't overflow UI.
						const char* n = static_cast<LumosGame*>(game)->GenName("sector", random.Rand(), 4, 7);
						GLASSERT(n);
						if (n) {
							name = n;
						}
						GLASSERT(NUM_SECTORS == 16);	// else the printing below won't be correct.
						postfix = "";
						postfix.Format("-%x%x", i, j);
						name += postfix;
						sectorData[j*NUM_SECTORS + i].name = StringPool::Intern(name.c_str());
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
		rockGen->StartCalc(random.Rand());
		genState.y = 0;
		genState.mode = GenState::ROCKGEN;
	}
		break;

	case GenState::ROCKGEN:
	{
		clock_t start = clock();
		while ((genState.y < WorldGen::SIZE) && (clock() - start < 30)) {
			for (int i = 0; i < 16; ++i) {
				rockGen->DoCalc(genState.y);
				genState.y++;
			}
		}
		CStr<16> str;
		str.Format("Rock: %d%%", (int)(100.0f*(float)genState.y / (float)WorldGen::SIZE));
		label.SetText(str.c_str());

		if (genState.y == WorldGen::SIZE) {
			rockGen->EndCalc();

			Random random;
			random.SetSeedFromTime();

			rockGen->DoThreshold(random.Rand(), 0.35f, RockGen::NOISE_HEIGHT);
			for (int y = 0; y < WorldGen::SIZE; ++y) {
				BlendLine(y);
			}
			sendTexture = true;
			genState.Clear();
			genState.mode = GenState::SIM_START;
			label.SetText("100%");
		}
	}
		break;

	case GenState::SIM_START:
	{
		// Nasty bug: the sim will do a load.
		// Be sure to save the map *before* 
		// the sim->Load() else the sim
		// runs on the old map.
		const char* mapPNG = game->GamePath("map", 0, "png");
		const char* mapDAT = game->GamePath("map", 0, "dat");
		worldMap->Save(mapDAT);
		worldMap->SavePNG(mapPNG);

		SetMapBright(false);
		LumosGame* game = this->GetGame()->ToLumosGame();
		sim = new Sim(game);
		const char* datPath = game->GamePath("map", 0, "dat");
		sim->Load(datPath, 0);

		sim->EnableSpawn(false);
		genState.mode = GenState::SIM_TICK;
	}
		break;

	case GenState::SIM_TICK:
	{
		clock_t start = clock();
		while (clock() - start < 100) {
			for (int i = 0; i < 10; ++i) {
				sim->DoTick(100);
			}
		}
		float age = sim->AgeF();
		int typeCount[NUM_PLANT_TYPES];
		for (int i = 0; i<NUM_PLANT_TYPES; ++i) {
			typeCount[i] = 0;
			for (int j = 0; j<MAX_PLANT_STAGES; ++j) {
				typeCount[i] += sim->GetChitBag()->census.plants[i][j];
			}
		}

		simStr.Format("SIM:\nAge=%.2f\n\nOrbstalk=%d\nTree=%d\nFern=%d\nCrystalGrass=%d\nBamboo=%d\nShroom=%d\nSunBloom=%d\nMoonBloom=%d\nMOBs=%d",
					  age, 
					  typeCount[0], typeCount[1], typeCount[2], typeCount[3], typeCount[4], typeCount[5], typeCount[6], typeCount[7],
					  sim->GetChitBag()->census.normalMOBs );
		worldText.SetText(simStr.c_str());

		if (age > SettingsManager::Instance()->SpawnDate()) {
			sim->EnableSpawn(true);
		}

		if (age > 0.2f) {
		//if (age > 1) {
			genState.mode = GenState::SIM_DONE;
		}
	}
		break;

	case GenState::SIM_DONE:
	{
		const char* datPath = game->GamePath("map", 0, "dat");
		const char* gamePath = game->GamePath("game", 0, "dat");

		sim->Save(datPath, gamePath);

		genState.mode = GenState::DONE;
		simStr.AppendFormat("\n\nDONE!");
		worldText.SetText(simStr.c_str());
	}
		break;

	case GenState::DONE:
	{
		okay.SetEnabled(true);
	}
		break;

	default:
		GLASSERT(0);
		break;
	}

	if ( sendTexture ) {
		Texture* t = TextureManager::Instance()->GetTexture( "worldGenPreview" );
		CreateTexture( t );
	}
}

