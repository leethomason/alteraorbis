#include "hpbar.h"
#include "../grinliz/glstringutil.h"
#include "../game/lumosgame.h"
#include "../game/gameitem.h"
#include "../game/layout.h"

using namespace gamui;

void HPBar::Init(Gamui* gamui)
{
	RenderAtom nullAtom;
	RenderAtom orange = LumosGame::CalcPaletteAtom( 4, 0 );
	RenderAtom red	  = LumosGame::CalcPaletteAtom( 0, 1 );	
	RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	

	orange.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	red.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;
	green.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO;

	DigitalBar::Init(gamui, nullAtom, nullAtom);
	this->EnableDouble(true);
	this->SetAtom(0, orange, 0);
	this->SetAtom(0, green, 1);
	this->SetAtom(1, red, 1);
}


void HPBar::Set(const GameItem* item, const Shield* shield, const char* optionalName)
{
	this->SetRange(shield ? shield->ChargeFraction() : 0, 0);
	this->SetRange(item ? (float)item->HPFraction() : 1, 1);
	if (optionalName) {
		this->SetText(optionalName);
	}
	else if (item) {
		grinliz::CStr<32> str;
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
