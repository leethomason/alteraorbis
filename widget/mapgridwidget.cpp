#include "mapgridwidget.h"
#include "../script/corescript.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitcontext.h"
#include "../game/worldmap.h"
#include "../game/team.h"
#include "../game/worldinfo.h"
#include "../game/gameitem.h"
#include "../game/lumoschitbag.h"
#include "../game/lumosgame.h"
#include "../game/reservebank.h"

using namespace gamui;
using namespace grinliz;

MapGridWidget::MapGridWidget()
{
}

void MapGridWidget::Init(Gamui* gamui2D)
{
	textLabel.Init(gamui2D);
	dScore.Init(gamui2D);

	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].Init(gamui2D, RenderAtom(), true);
	}

	int layer = Gamui::LEVEL_FOREGROUND + 0;
	image[FACE_IMAGE_0].SetLevel(layer);
	image[FACE_IMAGE_1].SetLevel(layer);
	image[FACE_IMAGE_2].SetLevel(layer);
	image[CIV_TECH_IMAGE].SetLevel(layer);

	layer = Gamui::LEVEL_FOREGROUND + 1;
	image[MOB_COUNT_IMAGE_0].SetLevel(layer);
	image[MOB_COUNT_IMAGE_1].SetLevel(layer);
	image[MOB_COUNT_IMAGE_2].SetLevel(layer);
	image[GOLD_IMAGE].SetLevel(layer);
}


void MapGridWidget::SetCompactMode(bool value) 
{
	if (value != compact) {
		compact = value;
		DoLayout();
	}
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

	if (compact) {
		dw = w / 2.0f;
		dh = h / 2.0f;
	}

	textLabel.SetPos(x, y);
	static const float MULT = 0.8f;

	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].SetSize(dw, dh);
	}
	for (int i = 0; i < NUM_FACE_IMAGES; ++i) {
		float fy = compact ? y : y + dh;
		image[FACE_IMAGE_0 + i].SetSize(dw * MULT, dh * MULT);
		image[FACE_IMAGE_0 + i].SetCenterPos(x + dw * float(i) + dw * 0.5f, fy + dh * 0.5f);
		image[MOB_COUNT_IMAGE_0 + i].SetPos(x + dw*float(i), fy + dh * 0.2f);
	}

	float fy = compact ? y + dh : y + dh * 2.0f;
	image[CIV_TECH_IMAGE].SetPos(x, fy);

	image[GOLD_IMAGE].SetCenterPos(x + dw * 0.5f, fy + dh * 0.5f);

	float fx = compact ? x + dw : x + dw*2.0f;
	image[DIPLOMACY_IMAGE].SetPos(fx, fy);

	if (!compact) {
		dScore.SetPos(x + dw, y + dh * 2);
	}
}


void MapGridWidget::Set(const ChitContext* context, CoreScript* coreScript, CoreScript* home, const Web* web)
{
	Clear();
	if (!coreScript) return;

	Vector2I sector = ToSector(coreScript->ParentChit()->Position());
	const SectorData& sd = context->worldMap->GetSectorData( sector );

	// ---- Text at top. ----
	CStr<64> str = "";
	if (!compact) {
		const char* owner = "";
		if (coreScript->InUse()) {
			owner = Team::TeamName(coreScript->ParentChit()->Team()).safe_str();
		}
		str.Format("%s\n%s", sd.name.c_str(), owner);
		textLabel.SetText(str.safe_str());
	}
	for (int i = 0; i < NUM_IMAGES; ++i) {
		image[i].SetAtom(RenderAtom());
	}

	// ---- Count of MOBs ---
	Rectangle2I inner = sd.InnerBounds();
	Rectangle2F innerF = ToWorld2F(inner);

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

	const int N = compact ? 2 : 3;
	for (int i = 0; i < N; ++i) {
		RenderAtom atom, countAtom;
		if (countArr.Size() > i) {
			CStr<64> str;
			str.Format("%s_sil", countArr[i].name.safe_str());
			atom = LumosGame::CalcUIIconAtom(str.safe_str(), true, 0);
			GLASSERT(atom.textureHandle);
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
	if (coreScript->InUse()) {
		float civTech = coreScript->CivTech();
		RenderAtom atom;
		if (civTech > 0.75f)		atom = LumosGame::CalcUIIconAtom("civtech4");
		else if (civTech > 0.50f)	atom = LumosGame::CalcUIIconAtom("civtech3");
		else if (civTech > 0.25f)	atom = LumosGame::CalcUIIconAtom("civtech2");
		else						atom = LumosGame::CalcUIIconAtom("civtech1");

		image[CIV_TECH_IMAGE].SetAtom(atom);
	}

	// Gold
	// Sources: core & value of buildings CoreWealth()
	//			denizens & mobs
	//			stuff on the ground
	int gold = 0;
	GoldLootFilter goldLootFilter;
	CChitArray loot;
	context->chitBag->QuerySpatialHash(&loot, innerF, 0, &goldLootFilter);

	if (coreScript) {
		gold += coreScript->CoreWealth();
	}

	for (int i = 0; i < loot.Size(); ++i) {
		// For gold/crystal the value will be zero and the wallet set.
		const GameItem* item = loot[i]->GetItem();
		if (item->GetValue()) {
			gold += item->GetValue();
		}
		else {
			gold += ReserveBank::Instance()->ValueOfWallet(item->wallet);
		}
	}

	CChitArray mobs;
	context->chitBag->QuerySpatialHash(&mobs, innerF, 0, &mobFilter);
	for (int i = 0; i<mobs.Size(); ++i) {
		gold += ReserveBank::Instance()->ValueOfWallet(mobs[i]->GetItem()->wallet);
	}

	RenderAtom goldAtom;
	if (gold > 2000)		goldAtom = LumosGame::CalcUIIconAtom("au4");
	else if (gold > 1000)	goldAtom = LumosGame::CalcUIIconAtom("au3");
	else if (gold> 500)		goldAtom = LumosGame::CalcUIIconAtom("au2");
	else if (gold> 100)		goldAtom = LumosGame::CalcUIIconAtom("au1");

	image[GOLD_IMAGE].SetAtom(goldAtom);

	if (coreScript && home && coreScript->InUse() && home->InUse() && !Team::IsDeityCore(coreScript->ParentChit()->Team())) {
		RenderAtom atom;
		ERelate relate = Team::Instance()->GetRelationship(coreScript->ParentChit(), home->ParentChit());

		if (relate == ERelate::FRIEND) atom       = LumosGame::CalcUIIconAtom("friend");
		else if (relate == ERelate::NEUTRAL) atom = LumosGame::CalcUIIconAtom("neutral");
		else if (relate == ERelate::ENEMY) atom   = LumosGame::CalcUIIconAtom("enemy");

		image[DIPLOMACY_IMAGE].SetAtom(atom);

		if (web && (home != coreScript)) {
			//int diplomacy = Team::Instance()->CalcDiplomacy(home, coreScript, web);
			int attitude = Team::Instance()->Attitude(home, coreScript);
			CStr<32> str;
			str.Format("%+d", attitude);
			dScore.SetText(str.safe_str());
		}
	}
	else {
		dScore.SetText("");
	}
}
