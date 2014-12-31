#include "startwidget.h"
#include "../game/lumosgame.h"
#include "../game/worldinfo.h"
#include "../game/weather.h"
#include "../game/lumosmath.h"
#include "../game/lumoschitbag.h"
#include "../game/worldmap.h"

#include "../engine/engine.h"

#include "../xegame/cameracomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/scene.h"

#include "../script/plantscript.h"

using namespace gamui;
using namespace grinliz;

static const float GUTTER = 5.0f;

StartGameWidget::StartGameWidget()
{
	currentSector = 0;
	engine = 0;
	chitBag = 0;
	iScene = 0;
}


StartGameWidget::~StartGameWidget()
{
}


void StartGameWidget::SetSectorData(const SectorData** sdArr, int n, Engine* e, ChitBag* cb, Scene* scene, WorldMap* wm)
{
	n = Min(n, sectors.Capacity());
	sectors.Clear();
	for (int i = 0; i < n; ++i) {
		sectors.Push(sdArr[i]);
	}
	engine = e;
	chitBag = cb;
	iScene = scene;
	worldMap = wm;

	currentSector = 0;
	SetBodyText();
	const SectorData* sd = sectors[currentSector];
	Vector2I pos2i = { sd->x + SECTOR_SIZE / 2, sd->y + SECTOR_SIZE / 2 };
	Vector3F pos = ToWorld3F(pos2i);
	engine->CameraLookAt(pos.x, pos.z);
}


void StartGameWidget::Init(Gamui* gamui, const ButtonLook& look, const LayoutCalculator& calc)
{
	calculator = calc;

	gamui->StartDialog("StartGameWidget");
	background.Init(gamui, LumosGame::CalcUIIconAtom("background0"), false);
	background.SetSize(200, 200);
	background.SetSlice(true);

	topLabel.Init(gamui);
	bodyLabel.Init(gamui);
	countLabel.Init(gamui);
	topLabel.SetText("MotherCore has granted you access to a neutral domain core. Choose wisely.");
	countLabel.SetText("1/1");

	/*
	primaryColor.Init(gamui, LumosGame::CalcPaletteAtom(1, 1), true);
	secondaryColor.Init(gamui, LumosGame::CalcPaletteAtom(1, 1), true);

	prevColor.Init(gamui, look);
	nextColor.Init(gamui, look);
	*/
	prevDomain.Init(gamui, look);
	nextDomain.Init(gamui, look);
	/*
	prevColor.SetText("<");
	nextColor.SetText(">");
	*/
	prevDomain.SetText("<");
	nextDomain.SetText(">");

	okay.Init(gamui, look);
	okay.SetDeco(LumosGame::CalcUIIconAtom("okay", true), LumosGame::CalcUIIconAtom("okay", false));

	textHeight = gamui->TextHeightVirtual();
	gamui->EndDialog();
}


void StartGameWidget::SetBodyText()
{
	if (sectors.Empty()) return;

	const SectorData* sd = sectors[currentSector];
	Weather* weather = Weather::Instance();
	Vector2I pos = { sd->x + SECTOR_SIZE / 2, sd->y + SECTOR_SIZE / 2 };
	Vector2F pos2 = ToWorld2F(pos);
	GLASSERT(weather);
	const float AREA = float(SECTOR_SIZE*SECTOR_SIZE);
	int nPorts = 0;
	for (int i = 0; i < 4; ++i) {
		if ((1 << i) & sd->ports) {
			nPorts++;
		}
	}

	int bioFlora = 0;
	int flowers = 0;
	Rectangle2I bi = sd->InnerBounds();
	for (Rectangle2IIterator it(bi); !it.Done(); it.Next()) {
		const WorldGrid& wg = worldMap->GetWorldGrid(it.Pos().x, it.Pos().y);
		if (wg.Plant() >= 7) {
			flowers++;
		}
		else if (wg.Plant()) {
			bioFlora++;
		}
	}

	CStr<400> str;
	str.Format("%s\nTemperature=%d%%  Rain=%d%%  Land=%d%%  Water=%d%%  nPorts=%d\n"
		"bioFlora=%d%%  flowers=%d%%",
		sd->name.c_str(),
		LRint(weather->Temperature(pos2.x, pos2.y) * 100.0f),
		LRint(weather->RainFraction(pos2.x, pos2.y) * 100.0f),
		LRint(100.0f*float(sd->area) / AREA),
		LRint(100.0f*float(AREA - sd->area) / AREA),
		nPorts,
		LRint(100.0f*float(bioFlora) / AREA),
		LRint(100.0f*float(flowers) / AREA));

	bodyLabel.SetText(str.c_str());

	str.Format("%d/%d", currentSector+1, sectors.Size());
	countLabel.SetText(str.c_str());
}


void StartGameWidget::SetPos(float x, float y)
{
	background.SetPos(x, y);
	calculator.SetOffset(x, y);

	calculator.PosAbs(&topLabel, 0, 0);

	/*
	calculator.PosAbs(&prevColor, 0, 1);
	calculator.PosAbs(&primaryColor, 1, 1);
	calculator.PosAbs(&secondaryColor, 2, 1);
	calculator.PosAbs(&nextColor, 3, 1);
	*/
	calculator.PosAbs(&bodyLabel, 0, 1);
	
	// hack to drop the bottom a bit.
	calculator.SetOffset(x, y + textHeight);

	calculator.PosAbs(&okay, 0, 3);
	calculator.PosAbs(&prevDomain, 1, 3);
	calculator.PosAbs(&countLabel, 2, 3);
	calculator.PosAbs(&nextDomain, 3, 3);

	topLabel.SetBounds(background.Width() - GUTTER*2.0f, 0);
	bodyLabel.SetBounds(background.Width() - GUTTER*2.0f, 0);
}


void StartGameWidget::SetSize(float width, float h)
{
	background.SetSize(width, h);
	// Calls SetPos(), but not vice-versa
	SetPos(background.X(), background.Y());
}


void StartGameWidget::SetVisible(bool vis)
{
	background.SetVisible(vis);
	topLabel.SetVisible(vis);
	bodyLabel.SetVisible(vis);
	countLabel.SetVisible(vis);
	prevDomain.SetVisible(vis);
	nextDomain.SetVisible(vis);
	okay.SetVisible(vis);
}


void StartGameWidget::ItemTapped(const gamui::UIItem* item)
{
	bool changeDomain = false;
	if (item == &prevDomain) {
		currentSector--;
		changeDomain = true;
	}
	else if (item == &nextDomain) {
		currentSector++;
		changeDomain = true;
	}
	else if (item == &okay) {
		if (iScene) {
			iScene->DialogResult(Name(), (void*)sectors[currentSector]);
		}
	}

	if (changeDomain) {
		currentSector += sectors.Capacity();
		currentSector = currentSector % sectors.Capacity();
		SetBodyText();

		const SectorData* sd = sectors[currentSector];
		Vector2I pos2i = { sd->x + SECTOR_SIZE / 2, sd->y + SECTOR_SIZE / 2 };
		Vector3F pos = ToWorld3F(pos2i);
		//pos.y = 10.0f;	// FIXME...and fix camera handling
	
		engine->CameraLookAt(pos.x, pos.z);
		//CameraComponent* cc = chitBag->GetCamera(engine);
		//cc->SetPanTo(pos);
	}
}
