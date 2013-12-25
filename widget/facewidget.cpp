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

static const float HEIGHT = 10.0f;
static const float SPACE = 5.0f;

void FaceToggleWidget::Init( gamui::Gamui* gamui, const gamui::ButtonLook& look, int f )
{
	BaseInit( gamui, look, f );
	toggle.Init( gamui, look );
	toggle.SetEnabled( true );
}


void FacePushWidget::Init( gamui::Gamui* gamui, const gamui::ButtonLook& look, int f )
{
	BaseInit( gamui, look, f );
	push.Init( gamui, look );
	push.SetEnabled( true );
}


void FaceWidget::BaseInit( gamui::Gamui* gamui, const gamui::ButtonLook& look, int f )
{
	flags = f;
	upper.Init( gamui );

	RenderAtom green = LumosGame::CalcPaletteAtom( 1, 3 );	
	RenderAtom grey  = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue  = LumosGame::CalcPaletteAtom( 8, 0 );	

	bar[BAR_HP].Init(		gamui, 2, green, grey );
	bar[BAR_AMMO].Init(		gamui, 2, blue, grey );
	bar[BAR_SHIELD].Init(	gamui, 2, blue, grey );
	bar[BAR_LEVEL].Init(	gamui, 2, blue, grey );

	bar[BAR_HP].SetText( "HP" );
	bar[BAR_AMMO].SetText( "Weapon" );
	bar[BAR_SHIELD].SetText( "Shield" );

	for( int i=0; i<ai::Needs::NUM_NEEDS; i++ ) {
		GLASSERT( i < MAX_BARS );
		bar[i+BAR_SOCIAL].Init( gamui, 2, green, grey );
		bar[i+BAR_SOCIAL].SetText( ai::Needs::Name( i ) );
	}

	upper.SetVisible( false );
	for( int i=0; i<MAX_BARS; ++i ) {
		bar[i].SetVisible( (flags & (1<<i)) != 0 );
	}
}


void FaceWidget::SetFace( UIRenderer* renderer, const GameItem* item )
{
	if ( item ) {
		ProcRenderInfo info;
		HumanGen faceGen( strstr( item->ResourceName(), "emale" ) != 0, item->ID(), item->primaryTeam, false );
		faceGen.AssignFace( &info );

		RenderAtom procAtom(	(const void*) (UIRenderer::RENDERSTATE_UI_CLIP_XFORM_MAP), 
								info.texture,
								info.te.uv.x, info.te.uv.y, info.te.uv.z, info.te.uv.w );

		button->SetDeco( procAtom, procAtom );

		renderer->uv[0]			= info.te.uv;
		renderer->uvClip[0]		= info.te.clip;
		renderer->colorXForm[0]	= info.color;

		button->SetVisible( true );

		CStr<40> str;
		if ( flags & SHOW_NAME ) {
			upper.SetVisible( true );
			str.AppendFormat( "%s", item->IBestName().c_str() );
		}
		upper.SetText( str.c_str() );
	}
	else {
		button->SetVisible( false );
		upper.SetText( "" );
	}
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
			bar[BAR_HP].SetRange( item->HPFraction() );

			int lev = item->traits.Level();
			int xp = item->traits.Experience();
			int nxp = item->traits.LevelToExperience( item->traits.Level()+1 );
			int pxp = item->traits.LevelToExperience( item->traits.Level() );

			str.Format( "Level %d", lev );
			bar[BAR_LEVEL].SetText( str.c_str() );
			bar[BAR_LEVEL].SetRange( float( xp - pxp ) / float( nxp - pxp ));
		}
		IShield* ishield			= ic->GetShield();
		IRangedWeaponItem* iweapon	= ic->GetRangedWeapon(0);

		if ( iweapon ) {
			float r = 0;
			if ( iweapon->GetItem()->Reloading() ) {
				r = iweapon->GetItem()->ReloadFraction();
				bar[BAR_AMMO].SetLowerAtom( orange );
			}
			else {
				r = iweapon->GetItem()->RoundsFraction();
				bar[BAR_AMMO].SetLowerAtom( blue );
			}
			bar[BAR_AMMO].SetRange( r );
		}
		else {
			bar[BAR_AMMO].SetRange( 0 );
		}

		if ( ishield ) {
			float r = ishield->GetItem()->RoundsFraction();
			bar[BAR_SHIELD].SetRange( r );
		}
		else {
			bar[BAR_SHIELD].SetRange( 0 );
		}
	}


	if ( ai ) {
		const ai::Needs& needs = ai->GetNeeds();
		for( int i=0; i<ai::Needs::NUM_NEEDS; ++i ) {
			bar[i+BAR_SOCIAL].SetRange( (float)needs.Value(i) );
		}
	}
}


void FaceWidget::SetPos( float x, float y )			
{ 
	button->SetPos( x, y );  
	upper.SetPos( x, y ); 

	float cy = button->Y() + button->Height() + SPACE;
	for( int i=0; i < MAX_BARS; ++i ) {
		int on = (1<<i) & flags;

		if ( on ) {
			bar[i].SetPos( x, cy );
			bar[i].SetSize( button->Width(), HEIGHT );
			bar[i].SetVisible( true );
			cy += HEIGHT + SPACE;
		}
		else {
			bar[i].SetVisible( false );
		}
	}
}


void FaceWidget::SetSize( float w, float h )		
{ 
	button->SetSize( w, h ); 
	upper.SetBounds( w, 0 ); 
	for( int i=0; i < MAX_BARS; ++i ) {
		bar[i].SetSize( button->Width(), HEIGHT );
	}
	// SetSize calls SetPos, but NOT vice versa
	SetPos( button->X(), button->Y() );
}


void FaceWidget::SetVisible( bool vis )
{ 
	button->SetVisible( vis ); 
	upper.SetVisible( vis );
	for( int i=0; i < MAX_BARS; ++i ) {
		bar[i].SetVisible( vis && ((1<<i) & flags) );
	}
}
