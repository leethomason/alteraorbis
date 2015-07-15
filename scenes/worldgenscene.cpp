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
#include "../script/procedural.h"
#include "../script/corescript.h"
#include "../audio/xenoaudio.h"
#include <time.h>

using namespace grinliz;
using namespace gamui;

// FIXME clock() assumes milliseconds

WorldGenScene::WorldGenScene(LumosGame* game) : Scene(game)
{
	InitStd(&gamui2D, &okay, &cancel);
	sim = 0;

	worldMap = new WorldMap(MAX_MAP_SIZE, MAX_MAP_SIZE);
	pix16 = 0;

	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture("worldGenPreview", MAX_MAP_SIZE, MAX_MAP_SIZE, TEX_RGB16, Texture::PARAM_NONE, this);

	worldGen = new WorldGen();
	worldGen->LoadFeatures("./res/features.png");

	rockGen = new RockGen(MAX_MAP_SIZE);

	RenderAtom atom((const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, texman->GetTexture("worldGenPreview"),
					0, 1, 1, 0);	// y-flip: image to texture coordinate conversion
	worldImage.Init(&gamui2D, atom, false);
	worldImage.SetSize(400, 400);

	headerText.Init(&gamui2D);
	headerText.SetBounds(400, 0);
	statText.Init(&gamui2D);
	statText.SetBounds(400, 0);
	footerText.Init(&gamui2D);
	footerText.SetBounds(400, 0);

	newsConsole.Init(&gamui2D, 0);
	newsConsole.consoleWidget.SetSize(400, 100);
	newsConsole.consoleWidget.SetTime(VERY_LONG_TICK);

	for (int i = 0; i < NUM_SECTORS*NUM_SECTORS; ++i) {
		gridWidget[i].Init(&gamui2D);
		gridWidget[i].SetCompactMode(true);
		gridWidget[i].SetVisible(false);
	}

	genState.Clear();
}


WorldGenScene::~WorldGenScene()
{
	TextureManager::Instance()->TextureCreatorInvalid(this);
	delete worldMap;
	delete[] pix16;
	delete worldGen;
	delete rockGen;
	delete sim;
}


void WorldGenScene::Resize()
{
	PositionStd(&okay, &cancel);

	float size = gamui2D.Height() * 0.75f;
	worldImage.SetSize(size, size);
	worldImage.SetPos(gamui2D.Width()*0.5f - size*0.5f, 10.0f);

	LayoutCalculator layout = DefaultLayout();

	const float DY = 16.0f;

	headerText.SetPos(worldImage.X() + layout.GutterX(), worldImage.Y() + layout.GutterY());
	newsConsole.consoleWidget.SetPos(worldImage.X(), worldImage.Y() + worldImage.Height() + DY);
	newsConsole.consoleWidget.SetSize(400, okay.Y() + okay.Height() - newsConsole.consoleWidget.Y());

	statText.SetPos(worldImage.X() + worldImage.Width() + layout.GutterX(),
					worldImage.Y());
	footerText.SetPos(headerText.X(), headerText.Y() + gamui2D.TextHeightVirtual());

	headerText.SetTab(worldImage.Width() / 5.0f);
	statText.SetTab(worldImage.Width() / 5.0f);
	footerText.SetTab(worldImage.Width() / 5.0f);

	debugFPS = SettingsManager::Instance()->DebugFPS();

	for (int j = 0; j < NUM_SECTORS; ++j) {
		for (int i = 0; i < NUM_SECTORS; ++i) {
			const float dx = worldImage.Width() / float(NUM_SECTORS);
			const float dy = worldImage.Height() / float(NUM_SECTORS);
			gridWidget[j*NUM_SECTORS + i].SetSize(dx, dy);
			gridWidget[j*NUM_SECTORS + i].SetPos(worldImage.X() + dx * float(i), worldImage.Y() + dy * float(j));
		}
	}
}


void WorldGenScene::DeActivate()
{
	XenoAudio::Instance()->SetAudio(SettingsManager::Instance()->AudioOn());
}


void WorldGenScene::ItemTapped(const gamui::UIItem* item)
{
	if (item == &okay) {
		// SIM_START saves the map. Need to get past that.
		if (genState.mode > GenState::SIM_START) {
			const char* datPath = game->GamePath("map", 0, "dat");
			const char* gamePath = game->GamePath("game", 0, "dat");

			sim->Save(datPath, gamePath);
		}
		if (genState.mode == GenState::GEN_NOTES) {
			genState.mode = GenState::WORLDGEN;
			okay.SetEnabled(false);
			cancel.SetEnabled(false);
		}
		else {
			game->PopScene();
		}
	}
	else if (item == &cancel) {
		game->PopScene();
	}
}


void WorldGenScene::CreateTexture(Texture* t)
{
	if (StrEqual(t->Name(), "worldGenPreview")) {

		if (!pix16) {
			pix16 = new U16[MAX_MAP_SIZE_2];
		}
		// Must also set SectorData, which is done elsewhere.
		worldMap->MapInit(worldGen->Land(), worldGen->Path());

		int i = 0;
		for (int y = 0; y < MAX_MAP_SIZE; ++y) {
			for (int x = 0; x < MAX_MAP_SIZE; ++x) {
				pix16[i++] = Surface::CalcRGB16(worldMap->Pixel(x, y));
			}
		}
		t->Upload(pix16, MAX_MAP_SIZE_2*sizeof(U16));
	}
	else {
		GLASSERT(0);
	}
}


void WorldGenScene::BlendLine(int y)
{
	for (int x = 0; x < MAX_MAP_SIZE; ++x) {
		int h = *(worldGen->Land() + y*MAX_MAP_SIZE + x);
		int r = *(rockGen->Height() + y*MAX_MAP_SIZE + x);

		if (h >= WorldGen::LAND0 && h <= WorldGen::LAND3) {
			if (r) {
				// It is land. The World land is the minimum;
				// apply the rockgen value.

				h = Max(h, WorldGen::LAND1 + r / 102);
				//h = WorldGen::LAND1 + r / 102;
				GLASSERT(h >= WorldGen::LAND0 && h <= WorldGen::LAND3);
			}
			else {
				h = WorldGen::LAND0;
			}
		}
		worldGen->SetHeight(x, y, h);
	}
}


void WorldGenScene::SetMapBright(bool b)
{
	RenderAtom atom((const void*)(b ? UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE : UIRenderer::RENDERSTATE_UI_DISABLED),
					TextureManager::Instance()->GetTexture("worldGenPreview"),
					0, 1, 1, 0);	// y-flip: image to texture coordinate conversion
	worldImage.SetAtom(atom);
}

void WorldGenScene::DoTick(U32 delta)
{
	bool sendTexture = false;		// HACK: sending the texture initializes the worldmap
	bool generateEmitters = false;	// HACK: after initializing worldmap, generate emitters

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

			headerText.SetText("Welcome to world generation!\n\n"
							   "This will delete any game in progress.\n\n"
							   "World Generation creates the terrain and simulates the 1st Age, the Age of Fire. You can press "
							   "'okay' any time after 20% into the 1st Age to save and start. The world will be new and "
							   "largely empty. Or grab some water or coffee while the 1st Age passes. The world will fill with events, "
							   "monsters, and domain cores.\n\n"
							   
							   "Once generated, the world persists and you can play a game, with time passing "
							   "and domains rising and falling, for as long as you wish.\n\n"
							   "Altera's world is shaped by 2 opposing forces: volcanos creating land, and rampaging monsters "
							   "destroying land. Over time, the forces of motion and rock will balance. Domain creation and destruction "
							   "will create silica, pave, and buildings that will dot the landscape.\n\n"
							   "Tap 'okay' to generate or world or 'cancel' to return to title.");
		}
		break;

		case GenState::WORLDGEN:
		{
			// Don't want to hear crazy sound during world-gen:
			XenoAudio::Instance()->SetAudio(false);	// turned back on/off on scene de-activate. 
			statText.SetText("");
			headerText.SetText("");

			clock_t start = clock();
			if (clock() - start < CLOCK_MSEC(30)) {
				while (genState.y < MAX_MAP_SIZE) {
					for (int i = 0; i < 16; ++i) {
						worldGen->DoLandAndWater(genState.y++);
					}
				}
			}
			CStr<32> str;
			str.Format("Stage 1/3 Land: %d%%", (int)(100.0f*(float)genState.y / (float)MAX_MAP_SIZE));
			footerText.SetText(str.c_str());
			GLString name;

			if (genState.y == MAX_MAP_SIZE) {
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

//					const gamedb::Reader* database = game->GetDatabase();
					for (int j = 0; j < NUM_SECTORS; ++j) {
						for (int i = 0; i < NUM_SECTORS; ++i) {
							name = "sector";
							// Keep the names a little short, so that they don't overflow UI.
							/*
							IString n = LumosChitBag::StaticNameGen(database, "sector", random.Rand());
							GLASSERT(!n.empty());
							if (!n.empty()) {
								name = n.safe_str();
							}
							postfix = "";
							postfix.Format(":%c%d", 'A' + i, j + 1);
							name += postfix;
							*/
							name.Format("%c%d", 'A' + i, j + 1);
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
			while ((genState.y < MAX_MAP_SIZE) && (clock() - start < CLOCK_MSEC(30))) {
				for (int i = 0; i < 16; ++i) {
					rockGen->DoCalc(genState.y);
					genState.y++;
				}
			}
			CStr<32> str;
			str.Format("Stage 2/3 Rock: %d%%", (int)(100.0f*(float)genState.y / (float)MAX_MAP_SIZE));
			footerText.SetText(str.c_str());

			if (genState.y == MAX_MAP_SIZE) {
				rockGen->EndCalc();

				Random random;
				random.SetSeedFromTime();

				rockGen->DoThreshold(random.Rand(), 0.35f, RockGen::NOISE_HEIGHT);
				for (int y = 0; y < MAX_MAP_SIZE; ++y) {
					BlendLine(y);
				}
				sendTexture = true;
				generateEmitters = true;
				genState.Clear();
				genState.mode = GenState::SIM_START;
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

			sim->EnableSpawn(0);
			genState.mode = GenState::SIM_TICK;
			newsConsole.AttachChitBag(sim->GetChitBag());
		}
		break;

		case GenState::SIM_TICK:
		{
			clock_t start = clock();
			while (clock() - start < CLOCK_MSEC(100)) {
				for (int i = 0; i < 10; ++i) {
					sim->DoTick(100, false);
					newsConsole.DoTick(100, 0);
				}
			}
			float age = sim->AgeF();

			// Give the volcanos a head start,
			// but then seed plants.
			if (age >= 0.1f && !plantsSeeded) {
				sim->SeedPlants();
				plantsSeeded = true;
			}

			int typeCount[NUM_PLANT_TYPES];
			for (int i = 0; i < NUM_PLANT_TYPES; ++i) {
				typeCount[i] = 0;
				for (int j = 0; j < MAX_PLANT_STAGES; ++j) {
					typeCount[i] += sim->GetWorldMap()->plantCount[i][j];
				}
			}
			WorldMap* swm = sim->GetWorldMap();
			int pools = 0, waterfalls = 0;
			swm->FluidStats(sim->Context()->physicsSims, &pools, &waterfalls);

			simStr.Format("Age=%.2f\n", age);
			headerText.SetText(simStr.safe_str());
			const Census& census = sim->GetChitBag()->census;

			simStr = "";
			if (debugFPS) {
				simStr.Format("Plants=%d Pools=%d Waterfalls=%d wildFruit=%d\n\n"
							  "Orbstalk=%d\t\t[%d, %d, %d, %d]\n"
							  "Tree=%d\t\t[%d, %d, %d, %d]\n"
							  "Fern=%d\t\t[%d, %d, %d, %d]\n"
							  "CrystalGrass=%d\t\t[%d, %d, %d, %d]\n"
							  "Bamboo=%d\t\t[%d, %d, %d, %d]\n"
							  "Shroom=%d\t\t[%d, %d, %d, %d]\n"
							  "SunBloom=%d\t\t[%d, %d]\n"
							  "MoonBloom=%d\t\t[%d, %d]\n\n",
							  swm->CountPlants(),
							  pools, waterfalls, census.wildFruit,
							  typeCount[0], swm->plantCount[0][0], swm->plantCount[0][1], swm->plantCount[0][2], swm->plantCount[0][3],
							  typeCount[1], swm->plantCount[1][0], swm->plantCount[1][1], swm->plantCount[1][2], swm->plantCount[1][3],
							  typeCount[2], swm->plantCount[2][0], swm->plantCount[2][1], swm->plantCount[2][2], swm->plantCount[2][3],
							  typeCount[3], swm->plantCount[3][0], swm->plantCount[3][1], swm->plantCount[3][2], swm->plantCount[3][3],
							  typeCount[4], swm->plantCount[4][0], swm->plantCount[4][1], swm->plantCount[4][2], swm->plantCount[4][3],
							  typeCount[5], swm->plantCount[5][0], swm->plantCount[5][1], swm->plantCount[5][2], swm->plantCount[5][3],
							  typeCount[6], swm->plantCount[6][0], swm->plantCount[6][1],
							  typeCount[7], swm->plantCount[7][0], swm->plantCount[7][1]);
			}
			simStr.AppendFormat("MOBs:\n");
			for (int i = 0; i < census.MOBItems().Size(); ++i) {
				const Census::MOBItem& mobItem = census.MOBItems()[i];
				simStr.AppendFormat("%s\t%d\n", mobItem.name.safe_str(), mobItem.count);
			}
			simStr.AppendFormat("\n\nDomains:\n");
			for (int i = 0; i < census.CoreItems().Size(); ++i) {
				const Census::MOBItem& coreItem = census.CoreItems()[i];
				simStr.AppendFormat("%s\t%d\n", coreItem.name.safe_str(), coreItem.count);
			}

			statText.SetText(simStr.c_str());

			CStr<32> str;
			str.Format("Stage 3/3 Simulation: %1.f%%", 100.0f * age);
			footerText.SetText(str.c_str());

			for (int j = 0; j < NUM_SECTORS; ++j) {
				for (int i = 0; i < NUM_SECTORS; ++i) {
					Vector2I sector = { i, j };
					CoreScript* cs = CoreScript::GetCore(sector);
					// FIXME: don't need the sim->GetWeb() call. Here to force
					// web calc to see performance cost.
					gridWidget[j*NUM_SECTORS + i].Set(sim->Context(), cs, 0, &sim->CalcWeb());
					gridWidget[j*NUM_SECTORS + i].SetVisible(true);
				}
			}

			int spawn = 0;
			if (age > SettingsManager::Instance()->DenizenDate()) {
				spawn |= Sim::SPAWN_DENIZENS;
			}
			if (age > SettingsManager::Instance()->SpawnDate()) {
				spawn |= Sim::SPAWN_LESSER;
				spawn |= Sim::SPAWN_GREATER;
			}
			sim->EnableSpawn(spawn);
			okay.SetEnabled(spawn != 0);

			// Have to wait until the first age so that
			// volcanoes don't eat your domain.
			if (age > SettingsManager::Instance()->WorldGenDone()) {
				genState.mode = GenState::SIM_DONE;
			}
		}
		break;

		case GenState::SIM_DONE:
		{
			genState.mode = GenState::DONE;
			simStr.AppendFormat("\n\nDONE!");
			footerText.SetText("DONE");
			statText.SetText(simStr.c_str());
		}
		break;

		case GenState::DONE:
		{
			XenoAudio::Instance()->SetAudio(SettingsManager::Instance()->AudioOn());
		}
		break;

		default:
		GLASSERT(0);
		break;
	}

	if (sendTexture) {
		Texture* t = TextureManager::Instance()->GetTexture("worldGenPreview");
		CreateTexture(t);
	}
}

