#include "facewidget.h"
#include "../script/procedural.h"
#include "../game/gameitem.h"
#include "../game/lumosgame.h"
#include "../grinliz/glstringutil.h"
#include "../ai/aineeds.h"
#include "../game/aicomponent.h"
#include "../xegame/itemcomponent.h"	

using namespace gamui;
using namespace grinliz;


void FaceToggleWidget::Init( gamui::Gamui* gamui, const gamui::ButtonLook& look, int f, int id )
{
	toggle.Init( gamui, look );
	toggle.SetEnabled( true );
	BaseInit( gamui, look, f, id );
}


void FacePushWidget::Init( gamui::Gamui* gamui, const gamui::ButtonLook& look, int f, int id )
{
	push.Init( gamui, look );
	push.SetEnabled( true );
	BaseInit( gamui, look, f, id );
}


void FaceWidget::BaseInit( gamui::Gamui* gamui, const gamui::ButtonLook& look, int f, int id )
{
	GLASSERT(id >= 0 && id < UIRenderer::NUM_DATA);
	this->id = id;
	flags = f;
	upper.Init( gamui );

	barStack.Init(gamui, 1, MAX_BARS-1);

	// Must keep the Needs and Bars in sync.
	GLASSERT( BAR_FOOD + ai::Needs::NUM_NEEDS == MAX_BARS );

	barStack.barArr[BAR_AMMO]->SetText("Weapon");
	barStack.barArr[BAR_MORALE]->SetText("morale");

	for( int i=0; i<ai::Needs::NUM_NEEDS; i++ ) {
		GLASSERT( i < MAX_BARS );
		barStack.barArr[i+BAR_FOOD]->SetText(ai::Needs::Name(i));
	}

	upper.SetVisible( false );
	SetFlags(f);
}

	
void FaceWidget::SetFlags(int f)
{
	flags = f;
	barStack.SetVisible(this->Visible());
	for (int i = 0; i < MAX_BARS; ++i) {
		barStack.barArr[i]->SetVisible((flags & (1 << i))&& this->Visible());
	}
}



void FaceWidget::SetFace( UIRenderer* renderer, const GameItem* item )
{
	if (item) {
		IString icon = item->keyValues.GetIString("uiIcon");
		if (icon.empty()) {
			icon = item->IName();
		}

		if (icon == "humanMale" || icon == "humanFemale") {
			ProcRenderInfo info;
			HumanGen faceGen(item->IsFemale(), item->ID(), item->Team(), false);
			faceGen.AssignFace(&info);

			RenderAtom procAtom((const void*)(UIRenderer::RENDERSTATE_UI_CLIP_XFORM_MAP_0 + id),
								info.texture,
								info.te.uv.x, info.te.uv.y, info.te.uv.z, info.te.uv.w);

			GetButton()->SetDeco(procAtom, procAtom);

			renderer->uv[id] = info.te.uv;
			renderer->uvClip[id] = info.te.clip;
			renderer->colorXForm[id] = info.color;
		}
		else {
			RenderAtom atom = LumosGame::CalcUIIconAtom(icon.safe_str());
			atom.renderState = (void*)UIRenderer::RENDERSTATE_UI_NORMAL;
			GetButton()->SetDeco(atom, atom);
		}

		GetButton()->SetVisible(true);

		CStr<40> str;
		if (flags & SHOW_NAME) {
			upper.SetVisible(true);
			str.AppendFormat("%s", item->IBestName().c_str());
		}
		upper.SetText(str.c_str());
	}
	else {
		GetButton()->SetVisible( false );
		upper.SetText( "" );
	}
 	//barStack.SetVisible(GetButton()->Visible());
	SetFlags(flags);	// sets visibility
}


void FaceWidget::SetMeta( ItemComponent* ic, AIComponent* ai ) 
{
	RenderAtom orange = LumosGame::CalcPaletteAtom( 4, 0 );
	RenderAtom grey   = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue   = LumosGame::CalcPaletteAtom( 8, 0 );	
	//RenderAtom red	  = LumosGame::CalcPaletteAtom( 0, 1 );	
	//RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	

	CStr<30> str;

	if ( ic ) {
		const GameItem* item = ic->GetItem(0);

		if ( flags & LEVEL_BAR ) {
			HPBar* hpbar = static_cast<HPBar*>(barStack.barArr[0]);
			hpbar->Set(item, ic->GetShield(), "HP");

			int lev = item->Traits().Level();
			int xp  = item->Traits().Experience();
			int nxp = item->Traits().LevelToExperience( item->Traits().Level()+1 );
			int pxp = item->Traits().LevelToExperience( item->Traits().Level() );

			str.Format( "Level %d", lev );
			barStack.barArr[BAR_LEVEL]->SetText(str.safe_str());
			barStack.barArr[BAR_LEVEL]->SetRange(float(xp - pxp) / float(nxp - pxp));
		}
		Shield* ishield			= ic->GetShield();
		RangedWeapon* iweapon	= ic->GetRangedWeapon(0);

		if ( iweapon ) {
			float r = 0;
			if ( iweapon->Reloading() ) {
				r = iweapon->ReloadFraction();
				barStack.barArr[BAR_AMMO]->SetAtom(0, orange);
			}
			else {
				r = iweapon->RoundsFraction();
				barStack.barArr[BAR_AMMO]->SetAtom(0, blue);
			}
			barStack.barArr[BAR_AMMO]->SetRange(r);
		}
		else {
			barStack.barArr[BAR_AMMO]->SetRange(0);
		}
	}

	if ( ai ) {
		const ai::Needs& needs = ai->GetNeeds();
		for( int i=0; i<ai::Needs::NUM_NEEDS; ++i ) {
			barStack.barArr[i + BAR_FOOD]->SetRange( (float)needs.Value(i));
		}
		barStack.barArr[BAR_MORALE]->SetRange((float)needs.Morale());
	}
}


void FaceWidget::SetPos( float x, float y )			
{ 
	GetButton()->SetPos( x, y );  
	upper.SetPos( x, y ); 

	float cy = GetButton()->Y() + GetButton()->Height() + 5.0f;
	barStack.SetPos(x, cy );
}


void FaceWidget::SetSize( float w, float h )		
{ 
	Button* button = GetButton();
	button->SetSize( w, h ); 
	upper.SetBounds( w, 0 ); 
	barStack.SetSize(w, h);
	// SetSize calls SetPos, but NOT vice versa
	SetPos( GetButton()->X(), GetButton()->Y() );
}


void FaceWidget::SetVisible( bool vis )
{ 
	GetButton()->SetVisible( vis ); 
	upper.SetVisible( vis );
	//barStack.SetVisible(vis);
	SetFlags(flags);
}
