#include "facewidget.h"
#include "../script/procedural.h"
#include "../game/gameitem.h"
#include "../game/lumosgame.h"

using namespace gamui;

void FaceToggleWidget::Init( gamui::Gamui* gamui, const gamui::ButtonLook& look )
{
	toggle.Init( gamui, look );
	toggle.SetEnabled( true );
}


void FacePushWidget::Init( gamui::Gamui* gamui, const gamui::ButtonLook& look )
{
	push.Init( gamui, look );
	push.SetEnabled( true );
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
	}
	else {
		button->SetVisible( false );
	}
}