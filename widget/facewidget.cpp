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

	//RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	
	//RenderAtom grey  = LumosGame::CalcPaletteAtom( 0, 6 );
	//RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	
	barStack.Init(gamui, MAX_BARS);

	// Must keep the Needs and Bars in sync.
	GLASSERT( BAR_FOOD + ai::Needs::NUM_NEEDS == MAX_BARS );
	//bar[BAR_HP].Init(		gamui, 2, green, grey );
	//bar[BAR_AMMO].Init(		gamui, 2, blue, grey );
	//bar[BAR_SHIELD].Init(	gamui, 2, blue, grey );
	//bar[BAR_LEVEL].Init(	gamui, 2, blue, grey );
	//bar[BAR_MORALE].Init(	gamui, 2, blue, grey );

	barStack.SetBarText(BAR_HP, "HP"); // bar[BAR_HP].SetText("HP");
	barStack.SetBarText(BAR_AMMO, "Weapon"); // bar[BAR_AMMO].SetText("Weapon");
	barStack.SetBarText(BAR_SHIELD, "Shield"); // bar[BAR_SHIELD].SetText("Shield");
	barStack.SetBarText(BAR_MORALE, "Morale"); // bar[BAR_MORALE].SetText("Morale");

	for( int i=0; i<ai::Needs::NUM_NEEDS; i++ ) {
		GLASSERT( i < MAX_BARS );
		//bar[i+BAR_FOOD].Init( gamui, 2, green, grey );
		barStack.SetBarText(i + BAR_FOOD, ai::Needs::Name(i)); // bar[i + BAR_FOOD].SetText(ai::Needs::Name(i));
	}

	upper.SetVisible( false );
	for( int i=0; i<MAX_BARS; ++i ) {
		barStack.SetBarVisible(i, (flags & (1 << i)) != 0); // bar[i].SetVisible((flags & (1 << i)) != 0);
	}
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
			//str.AppendFormat( "%s", item->INameAndTitle().c_str() );
		}
		upper.SetText(str.c_str());
	}
	else {
		GetButton()->SetVisible( false );
		upper.SetText( "" );
	}
 
	//for( int i=0; i < MAX_BARS; ++i ) {
	//	bool on = ((1<<i) & flags) != 0;
	//	on = on && GetButton()->Visible();
	//	bar[i].SetVisible( on );
	//}
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
			barStack.SetBarRatio(BAR_HP, (float)item->HPFraction()); // bar[BAR_HP].SetRange(float(item->HPFraction()));

			int lev = item->Traits().Level();
			int xp  = item->Traits().Experience();
			int nxp = item->Traits().LevelToExperience( item->Traits().Level()+1 );
			int pxp = item->Traits().LevelToExperience( item->Traits().Level() );

			str.Format( "Level %d", lev );
			barStack.SetBarText(BAR_LEVEL, str.safe_str()); // bar[BAR_LEVEL].SetText(str.c_str());
			barStack.SetBarRatio(BAR_LEVEL, float(xp - pxp) / float(nxp - pxp)); // bar[BAR_LEVEL].SetRange(float(xp - pxp) / float(nxp - pxp));
		}
		IShield* ishield			= ic->GetShield();
		IRangedWeaponItem* iweapon	= ic->GetRangedWeapon(0);

		if ( iweapon ) {
			float r = 0;
			if ( iweapon->GetItem()->Reloading() ) {
				r = iweapon->GetItem()->ReloadFraction();
				//bar[BAR_AMMO].SetLowerAtom( orange );
				barStack.SetBarColor(BAR_AMMO, orange);
			}
			else {
				r = iweapon->GetItem()->RoundsFraction();
				//bar[BAR_AMMO].SetLowerAtom( blue );
				barStack.SetBarColor(BAR_AMMO, blue);
			}
			//bar[BAR_AMMO].SetRange( r );
			barStack.SetBarRatio(BAR_AMMO, r);
		}
		else {
//			bar[BAR_AMMO].SetRange( 0 );
			barStack.SetBarRatio(BAR_AMMO, 0);
		}

		if ( ishield ) {
			float r = ishield->GetItem()->RoundsFraction();
			//bar[BAR_SHIELD].SetRange( r );
			barStack.SetBarRatio(BAR_SHIELD, r);
		}
		else {
			//bar[BAR_SHIELD].SetRange( 0 );
			barStack.SetBarRatio(BAR_SHIELD, 0);
		}
	}

	if ( ai ) {
		const ai::Needs& needs = ai->GetNeeds();
		for( int i=0; i<ai::Needs::NUM_NEEDS; ++i ) {
			//bar[i+BAR_FOOD].SetRange( (float)needs.Value(i) );
			barStack.SetBarRatio(i + BAR_FOOD, (float)needs.Value(i));
		}
//		bar[BAR_MORALE].SetRange( (float)needs.Morale() );
		barStack.SetBarRatio(BAR_MORALE, (float)needs.Morale());
	}
}


void FaceWidget::SetPos( float x, float y )			
{ 
	GetButton()->SetPos( x, y );  
	upper.SetPos( x, y ); 

	float cy = GetButton()->Y() + GetButton()->Height() + 5.0f;
	barStack.SetPos(x, cy);

//	for( int i=0; i < MAX_BARS; ++i ) {
//		int on = (1<<i) & flags;
//
//		if ( on ) {
//			bar[i].SetPos( x, cy );
//			bar[i].SetSize( GetButton()->Width(), HEIGHT );
//			bar[i].SetVisible( true );
//			cy += HEIGHT + SPACE;
//		}
//		else {
//			bar[i].SetVisible( false );
//		}
//	}
}


void FaceWidget::SetSize( float w, float h )		
{ 
	Button* button = GetButton();
	button->SetSize( w, h ); 
	upper.SetBounds( w, 0 ); 
//	for( int i=0; i < MAX_BARS; ++i ) {
//		bar[i].SetSize( GetButton()->Width(), HEIGHT );
//	}
	barStack.SetSize(w, h);
	// SetSize calls SetPos, but NOT vice versa
	SetPos( GetButton()->X(), GetButton()->Y() );
}


void FaceWidget::SetVisible( bool vis )
{ 
	GetButton()->SetVisible( vis ); 
	upper.SetVisible( vis );
	barStack.SetVisible(vis);
//	for( int i=0; i < MAX_BARS; ++i ) {
//		if ( !vis )
//			bar[i].SetVisible( false );
//		else
//			bar[i].SetVisible( ((1<<i) & flags) != 0 );
//	}
}
