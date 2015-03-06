#include "hpbar.h"
#include "../grinliz/glstringutil.h"
#include "../game/lumosgame.h"
#include "../game/gameitem.h"
#include "../game/layout.h"
#include "../xegame/itemcomponent.h"

using namespace gamui;
using namespace grinliz;

void HPBar::Init(Gamui* gamui)
{
	RenderAtom nullAtom;
	RenderAtom orange = LumosGame::CalcPaletteAtom( 4, 0 );
	RenderAtom red	  = LumosGame::CalcPaletteAtom( 0, 1 );	
	RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	

	rangedIcon.Init(gamui, nullAtom, true);
	meleeIcon.Init(gamui, nullAtom, true);
	hitBounds.Init(gamui, nullAtom, false);

	orange.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	red.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	green.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;

	DigitalBar::Init(gamui, nullAtom, nullAtom);
	this->EnableDouble(true);
	this->SetAtom(0, orange, 0);
	this->SetAtom(0, green, 1);
	this->SetAtom(1, red, 1);
}


void HPBar::Set(const GameItem* item, const MeleeWeapon* melee, const RangedWeapon* ranged, const Shield* shield, const char* optionalName)
{
	this->SetRange(shield ? shield->ChargeFraction() : 0, 0);
	this->SetRange(item ? (float)item->HPFraction() : 1, 1);
	rangedIcon.SetAtom(RenderAtom());
	meleeIcon.SetAtom(RenderAtom());

	if (optionalName) {
		this->SetText(optionalName);
	}
	else if (item) {
		if (ranged) {
			RenderAtom atom = LumosGame::CalcUIIconAtom(ranged->Name(), true);
			atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;
			rangedIcon.SetAtom(atom);
		}
		if (melee) {
			RenderAtom atom = LumosGame::CalcUIIconAtom(melee->Name(), true);
			atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_NORMAL;
			meleeIcon.SetAtom(atom);
		}

		CStr<32> str;
		if (item->Traits().Level())
			str.Format("%s %d", item->BestName(), item->Traits().Level());
		else
			str.Format("%s", item->BestName());
		this->SetText(str.safe_str());
	}
	else {
		this->SetText("");
	}
}


void HPBar::Set(ItemComponent* ic)
{
	const Shield* shield = ic->GetShield();
	const RangedWeapon* ranged = ic->QuerySelectRanged();
	const MeleeWeapon* melee = ic->QuerySelectMelee();

	rangedIcon.SetAtom(RenderAtom());
	meleeIcon.SetAtom(RenderAtom());

	this->Set(ic->GetItem(), melee, ranged, shield, 0);
}


bool HPBar::DoLayout()
{
	bool r = DigitalBar::DoLayout();
	float size = this->Height() * 0.8f;
	rangedIcon.SetSize(size, size);
	meleeIcon.SetSize(size, size);

	meleeIcon.SetPos(this->X(), this->Y());
	rangedIcon.SetPos(this->X() + this->Width() - size, this->Y());

	hitBounds.SetSize(this->Width(), this->Height());
	hitBounds.SetPos(this->X(), this->Y());
	return false;
}


