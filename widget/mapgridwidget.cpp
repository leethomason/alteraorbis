#include "mapgridwidget.h"
#include "../script/corescript.h"
#include "../xegame/spatialcomponent.h"
#include "../game/worldmap.h"
#include "../game/team.h"
#include "../game/worldinfo.h"
#include "../game/gameitem.h"
#include "../game/lumoschitbag.h"
#include "../game/lumosgame.h"

using namespace gamui;
using namespace grinliz;

MapGridWidget::MapGridWidget()
{
}

void MapGridWidget::Init(Gamui* gamui2D)
{
	textLabel.Init(gamui2D);
	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].Init(gamui2D, RenderAtom(), true);
	}

	image[MOB_COUNT_IMAGE_0].SetLevel(Gamui::LEVEL_FOREGROUND + 0);
	image[MOB_COUNT_IMAGE_1].SetLevel(Gamui::LEVEL_FOREGROUND + 0);
	image[MOB_COUNT_IMAGE_2].SetLevel(Gamui::LEVEL_FOREGROUND + 0);
	image[CIV_TECH_IMAGE].SetLevel(Gamui::LEVEL_FOREGROUND + 0);
	image[POPULATION_IMAGE].SetLevel(Gamui::LEVEL_FOREGROUND + 0);

	image[FACE_IMAGE_0].SetLevel(Gamui::LEVEL_FOREGROUND + 1);
	image[FACE_IMAGE_1].SetLevel(Gamui::LEVEL_FOREGROUND + 1);
	image[FACE_IMAGE_2].SetLevel(Gamui::LEVEL_FOREGROUND + 1);
	image[GOLD_IMAGE].SetLevel(Gamui::LEVEL_FOREGROUND + 1);
	image[GARRISON_IMAGE].SetLevel(Gamui::LEVEL_FOREGROUND + 1);
}


void MapGridWidget::SetPos(float x, float y)
{
	textLabel.SetPos(x, y);
	DoLayout();
}


void MapGridWidget::SetSize(float w, float h)
{
	width = w;
	height = h;
	DoLayout();
}

void MapGridWidget::SetVisible(bool vis)
{
	textLabel.SetVisible(vis);
	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].SetVisible(vis);
	}
}


void MapGridWidget::Clear()
{
	textLabel.SetText("");
	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].SetAtom(RenderAtom());
	}
}


void MapGridWidget::DoLayout()
{
	float x = this->X();
	float y = this->Y();
	float w = this->Width();
	float h = this->Height();

	float dw = w / float(NUM_FACE_IMAGES);
	float dh = h / 3.0f;

	textLabel.SetPos(x, y);

	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].SetSize(dw, dh);
	}
	for (int i = 0; i < NUM_FACE_IMAGES; ++i) {
		image[FACE_IMAGE_0+i].SetPos(x + dw * float(i), y + dh);
		image[MOB_COUNT_IMAGE_0 + i].SetPos(x + dw*float(i), y + dh);
	}
	image[CIV_TECH_IMAGE].SetPos(x, y + dh*2.0f);
	image[GOLD_IMAGE].SetPos(x, y + dh*2.0f);
	image[GARRISON_IMAGE].SetPos(x + dw, y + dh*2.0f);
	image[POPULATION_IMAGE].SetPos(x + dw, y + dh*2.0f);
}


void MapGridWidget::Set(const ChitContext* context, CoreScript* coreScript, CoreScript* home)
{
	Clear();
	if (!coreScript) return;

	Vector2I sector = coreScript->ParentChit()->GetSpatialComponent()->GetSector();
	const SectorData& sd = context->worldMap->GetSectorData( sector );

	// ---- Text at top. ----
	CStr<64> str = "";
	const char* owner = "";
	if ( coreScript->InUse() ) {
		owner = Team::TeamName( coreScript->ParentChit()->Team() ).safe_str();
	}
	str.Format( "%s\n%s", sd.name, owner );
	textLabel.SetText(str.safe_str());

	// ---- Count of MOBs ---
	Rectangle2I inner = sd.InnerBounds();
	Rectangle2F innerF = ToWorld(inner);

	MOBKeyFilter mobFilter;
	CChitArray query;
	context->chitBag->QuerySpatialHash(&query, innerF, 0, &mobFilter);

	struct Count {
		bool operator==(const Count& rhs) const { return name == rhs.name; }
		IString name;
		bool greater;
		int count;
	};
	CArray<Count, 16> countArr;

	for (int i = 0; i < query.Size() && countArr.HasCap(); ++i) {
		const GameItem* item = query[i]->GetItem();
		GLASSERT(item);
		bool greater = item->keyValues.GetIString(ISC::mob) == ISC::greater;

		Count c = { query[i]->GetItem()->IName(), greater, 1 };
		int idx = countArr.Find(c);
		if (idx >= 0)
			countArr[idx].count++;
		else
			countArr.Push(c);
	}

	countArr.Sort([](const Count& a, const Count& b){
		return (a.count * (a.greater ? 20 : 1)) > (b.count * (b.greater ? 20 : 1));
	});

	for (int i = 0; i < NUM_FACE_IMAGES; ++i) {
		RenderAtom atom, countAtom;
		if (countArr.Size() > i) {
			atom = LumosGame::CalcUIIconAtom(countArr[i].name.c_str(), true, 0);
			atom.renderState = (void*)UIRenderer::RENDERSTATE_UI_NORMAL;

			if (countArr[i].count > 12) countAtom = LumosGame::CalcUIIconAtom("num4");
			else if (countArr[i].count > 8) countAtom = LumosGame::CalcUIIconAtom("num3");
			else if (countArr[i].count > 4) countAtom = LumosGame::CalcUIIconAtom("num2");
			else countAtom = LumosGame::CalcUIIconAtom("num1");
		}
		image[FACE_IMAGE_0 + i].SetAtom(atom);
		image[MOB_COUNT_IMAGE_0 + i].SetAtom(countAtom);
	}

	// CivTech
	// FIXME
	// - don't need the garrison
	// - make layout square
	if (coreScript->InUse()) {
		RenderAtom atom = LumosGame::CalcUIIconAtom("civtech4");
		image[CIV_TECH_IMAGE].SetAtom(atom);
		atom = LumosGame::CalcUIIconAtom("au4");
		image[GOLD_IMAGE].SetAtom(atom);
	}
	else {
		image[CIV_TECH_IMAGE].SetAtom(RenderAtom());
		image[GOLD_IMAGE].SetAtom(RenderAtom());
	}

	// Garrison
	CChitArray citizenIDArr;
	int nCitizens = coreScript->Citizens(&citizenIDArr);
	image[GARRISON_IMAGE].SetAtom(RenderAtom());
	image[POPULATION_IMAGE].SetAtom(RenderAtom());

	if (citizenIDArr.Size()) {
		Chit* citizen = citizenIDArr[0];
		RenderAtom atom = LumosGame::CalcUIIconAtom(citizen->GetItem()->Name());
		image[GARRISON_IMAGE].SetAtom(atom);
		if (nCitizens > 12) atom = LumosGame::CalcUIIconAtom("num4");
		else if (nCitizens > 8) atom = LumosGame::CalcUIIconAtom("num3");
		else if (nCitizens > 4) atom = LumosGame::CalcUIIconAtom("num2");
		else atom = LumosGame::CalcUIIconAtom("num1");

		image[POPULATION_IMAGE].SetAtom(atom);
	}
}