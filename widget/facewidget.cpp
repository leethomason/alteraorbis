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

	barStack.Init(gamui, MAX_BARS);

	// Must keep the Needs and Bars in sync.
	GLASSERT( BAR_FOOD + ai::Needs::NUM_NEEDS == MAX_BARS );

	barStack.SetBarText(BAR_HP, "HP");
	barStack.SetBarText(BAR_AMMO, "Weapon");
	barStack.SetBarText(BAR_SHIELD, "Shield");
	barStack.SetBarText(BAR_MORALE, "morale");

	for( int i=0; i<ai::Needs::NUM_NEEDS; i++ ) {
		GLASSERT( i < MAX_BARS );
		barStack.SetBarText(i + BAR_FOOD, ai::Needs::Name(i));
	}

	upper.SetVisible( false );
	for( int i=0; i<MAX_BARS; ++i ) {
		barStack.SetBarVisible(i, (flags & (1 << i)) != 0);
	}
}


void FaceWidget::SetFlags(int f)
{
	flags = f;
	for( int i=0; i<MAX_BARS; ++i ) {
		barStack.SetBarVisible(i, (flags & (1 << i)) != 0);
	}
	barStack.SetVisible(this->Visible());
}



void FaceWidget::SetFace( UIRenderer* renderer, const GameItem* item )
{
	if (item) {
		IString icon = item->keyValues.GetIString("uiIcon");
		if (icon.empty()) {
			icon = item->IName();
		}

		if (icon == "human") {
			ProcRenderInfo info;
			HumanGen faceGen(strstr(item->ResourceName(), "emale") != 0, item->ID(), item->team, false);
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
 	barStack.SetVisible(GetButton()->Visible());
}


void FaceWidget::SetMeta( ItemComponent* ic, AIComponent* ai ) 
{
	RenderAtom orange = LumosGame::CalcPaletteAtom( 4, 0 );
	RenderAtom grey   = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue   = LumosGame::CalcPaletteAtom( 8, 0 );	

	CStr<30> str;

	if ( ic ) {
		const GameItem* item = ic->GetItem(0);

		if ( flags & LEVEL_BAR ) {
			barStack.SetBarRatio(BAR_HP, (float)item->HPFraction());

			int lev = item->Traits().Level();
			int xp  = item->Traits().Experience();
			int nxp = item->Traits().LevelToExperience( item->Traits().Level()+1 );
			int pxp = item->Traits().LevelToExperience( item->Traits().Level() );

			str.Format( "Level %d", lev );
			barStack.SetBarText(BAR_LEVEL, str.safe_str());
			barStack.SetBarRatio(BAR_LEVEL, float(xp - pxp) / float(nxp - pxp));
		}
		Shield* ishield			= ic->GetShield();
		RangedWeapon* iweapon	= ic->GetRangedWeapon(0);

		if ( iweapon ) {
			float r = 0;
			if ( iweapon->Reloading() ) {
				r = iweapon->ReloadFraction();
				barStack.SetBarColor(BAR_AMMO, orange);
			}
			else {
				r = iweapon->RoundsFraction();
				barStack.SetBarColor(BAR_AMMO, blue);
			}
			barStack.SetBarRatio(BAR_AMMO, r);
		}
		else {
			barStack.SetBarRatio(BAR_AMMO, 0);
		}

		if ( ishield ) {
			float r = ishield->ChargeFraction();
			barStack.SetBarRatio(BAR_SHIELD, r);
		}
		else {
			barStack.SetBarRatio(BAR_SHIELD, 0);
		}
	}

	if ( ai ) {
		const ai::Needs& needs = ai->GetNeeds();
		for( int i=0; i<ai::Needs::NUM_NEEDS; ++i ) {
			barStack.SetBarRatio(i + BAR_FOOD, (float)needs.Value(i));
		}
		barStack.SetBarRatio(BAR_MORALE, (float)needs.Morale());
	}
}


void FaceWidget::SetPos( float x, float y )			
{ 
	GetButton()->SetPos( x, y );  
	upper.SetPos( x, y ); 

	float cy = GetButton()->Y() + GetButton()->Height() + 5.0f;
	barStack.SetPos(x, cy);
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
	barStack.SetVisible(vis);
}
