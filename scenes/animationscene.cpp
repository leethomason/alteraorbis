#include "animationscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../engine/model.h"
#include "../engine/animation.h"

extern void ScreenCapture( const char* baseFilename, bool appendCount, bool trim, bool makeTransparent, grinliz::Rectangle2I* size );

using namespace gamui;
using namespace grinliz;
using namespace tinyxml2;

AnimationScene::AnimationScene( LumosGame* game ) : Scene( game )
{
	currentBone = -1;
	currentAnim = 0;
	doExport = false;

	game->InitStd( &gamui2D, &okay, 0 );
	Screenport* port = game->GetScreenportMutable();
	
	LayoutCalculator layout = game->DefaultLayout();

	boneLeft.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	boneLeft.SetSize( layout.Width(), layout.Height() );
	boneLeft.SetText( "<" );

	boneRight.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	boneRight.SetSize( layout.Width(), layout.Height() );
	boneRight.SetText( ">" );	

	boneName.Init( &gamui2D );
	boneName.SetText( "all" );

	animLeft.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	animLeft.SetSize( layout.Width(), layout.Height() );
	animLeft.SetText( "<" );

	animRight.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	animRight.SetSize( layout.Width(), layout.Height() );
	animRight.SetText( ">" );	

	animName.Init( &gamui2D );
	animName.SetText( "no animation" );

	ortho.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	ortho.SetSize( layout.Width(), layout.Height() );
	ortho.SetText( "ortho" );

	exportSCML.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	exportSCML.SetSize( layout.Width(), layout.Height() );
	exportSCML.SetText( "export" );

	pixelUnitRatio.Init( &gamui2D );

	engine = new Engine( port, game->GetDatabase(), 0 );
	engine->lighting.ambient.Set( 0.5f, 0.5f, 0.5f );
	engine->lighting.direction.Set( -1, 1, 1 );
	engine->lighting.direction.Normalize();
	engine->lighting.diffuse.Set( 0.5f, 0.5f, 0.5f );

	int count=0;
	model = 0;
	const ModelResource* const* resArr = ModelResourceManager::Instance()->GetAllResources( &count );
	for( int i=0; i<count; ++i ) {
		if ( !resArr[i]->header.animation.empty() ) {
			const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "humanFemale" );
			GLASSERT( res );
			model = engine->AllocModel( res );
			model->SetPos( 1, 0, 1 );
		}
	}

	engine->CameraLookAt( 1, 1, 5 );
	UpdateAnimationInfo();
}


AnimationScene::~AnimationScene()
{
	engine->FreeModel( model );
	delete engine;
}


void AnimationScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	//const Screenport& port = game->GetScreenport();
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &boneLeft,	0, -2 );
	layout.PosAbs( &boneName,	1, -2 );
	layout.PosAbs( &boneRight,	3, -2 );
	layout.PosAbs( &ortho,		4, -2 );
	layout.PosAbs( &exportSCML,	5, -2 );
	layout.PosAbs( &pixelUnitRatio, 6, -2 );

	layout.PosAbs( &animLeft,	0, 0 );
	layout.PosAbs( &animName,	1, 0 );
	layout.PosAbs( &animRight,	3, 0 );

}


void AnimationScene::UpdateBoneInfo()
{
	if ( currentBone == -1 ) {
		boneName.SetText( "all" );
		model->ClearParam();
	}
	else {
		GLString str;
		const char* name = model->GetResource()->header.BoneNameFromID( currentBone );
		str.Format( "%d:%s", currentBone, name ? name : "none" );
		boneName.SetText( str.c_str() );
		model->SetBoneFilter( currentBone );
	}
}


void AnimationScene::UpdateAnimationInfo()
{
	const AnimationResource* res = model->GetAnimationResource();
	int nAnim = res->NumAnimations();
	GLASSERT( currentAnim >=0 && currentAnim < nAnim );

	const char* name = "no animation";
	if ( nAnim > 0 ) {
		name = res->AnimationName( currentAnim );
		if ( !StrEqual( model->GetAnimation(), name )) {
			model->SetAnimation( name );
		}
	}
	animName.SetText( name );


	char buf[256];
	const char* fname = model->GetResource()->header.name.c_str();
	SNPrintf( buf, 256, "./resin/%s/%s.scml", fname, fname );
	FILE* fp = fopen( buf, "r" );
	if ( fp ) {
		exportSCML.SetEnabled( false );
		fclose( fp );
	}
	else {
		exportSCML.SetEnabled( true );
	}
}



void AnimationScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &boneRight ) {
		++currentBone;
		if ( currentBone == EL_MAX_BONES ) currentBone = -1;
	}
	else if ( item == &boneLeft ) {
		--currentBone;
		if ( currentBone == -2 ) currentBone = EL_MAX_BONES-1;
	}
	else if ( item == &animRight ) {
		++currentAnim;
		const AnimationResource* res = model->GetAnimationResource();
		if ( currentAnim >= res->NumAnimations() ) {
			currentAnim = 0;
		}
	}
	else if ( item == &animLeft ) {
		-- currentAnim;
		if ( currentAnim < 0 ) {
			const AnimationResource* res = model->GetAnimationResource();
			currentAnim = res->NumAnimations()-1;
		}
	}
	else if ( item == &ortho ) {
		Screenport* port = engine->GetScreenportMutable();
		if ( ortho.Down() ) {
			port->SetOrthoCamera( true, 0, 3.0f );
			static const Vector3F DIR = { -1, 0, 0 };	// FIXME: bug in camera code. (EEK.) Why is the direction negative???
			static const Vector3F UP  = { 0, 1, 0 };
			engine->camera.SetPosWC( -5, 0.4f, 1 );
			engine->camera.SetDir( DIR, UP ); 
		}
		else {
			port->SetOrthoCamera( false, 0, 0 );
			engine->CameraLookAt( 1, 1, 5 );
		}
	}
	else if ( item == &exportSCML ) {
		doExport = true;
		exportCount = -1;
	}

	UpdateBoneInfo();
	UpdateAnimationInfo();
}


void AnimationScene::InitXML( const Rectangle2I& bounds )
{
	char buf[256];
	
	const char* name = model->GetResource()->header.name.c_str();
	SNPrintf( buf, 256, "./resin/%s/%s.scml", name, name );
	scmlFP = fopen( buf, "w" );

	xmlDocument = new XMLDocument();
	xmlPrinter  = new XMLPrinter( scmlFP );

	xmlPrinter->OpenElement( "spriterdata" );
	xmlPrinter->OpenElement(	"char" );
	xmlPrinter->OpenElement(		"name" );
	xmlPrinter->PushText(				"char_000" );
	xmlPrinter->CloseElement();
	xmlPrinter->OpenElement(		"anim" );
	xmlPrinter->OpenElement(			"name" );
	xmlPrinter->PushText(					"reference" );
	xmlPrinter->CloseElement();			// name
	xmlPrinter->OpenElement(			"frame" );
	xmlPrinter->OpenElement(				"name" );
	xmlPrinter->PushText(						"frame_000" );
	xmlPrinter->CloseElement();
	xmlPrinter->OpenElement(				"duration" );
	xmlPrinter->PushText(						"500.0" );
	xmlPrinter->CloseElement();				// duration
	xmlPrinter->CloseElement();			// frame
	xmlPrinter->CloseElement();		// anim
	xmlPrinter->OpenElement(		"box" );
	xmlPrinter->OpenElement(			"bottom" );
	xmlPrinter->PushText(					bounds.max.y );
	xmlPrinter->CloseElement();
	xmlPrinter->OpenElement(			"top" );
	xmlPrinter->PushText(					bounds.min.y );
	xmlPrinter->CloseElement();
	xmlPrinter->OpenElement(			"left" );
	xmlPrinter->PushText(					bounds.min.x );
	xmlPrinter->CloseElement();
	xmlPrinter->OpenElement(			"right" );
	xmlPrinter->PushText(					bounds.max.x );
	xmlPrinter->CloseElement();
	xmlPrinter->CloseElement();		// box
	xmlPrinter->CloseElement();	// char
}


void AnimationScene::InitFrame()
{
	xmlPrinter->OpenElement( "frame" );
	xmlPrinter->OpenElement(	"name" );
	xmlPrinter->PushText(			"frame_000" );
	xmlPrinter->CloseElement();	// name
}


void AnimationScene::FinishFrame()
{
	xmlPrinter->CloseElement();
}


void AnimationScene::PushSprite( const char* name, const grinliz::Rectangle2I& bounds )
{
	char buf[200];

	xmlPrinter->OpenElement( "sprite" );
	xmlPrinter->OpenElement(	"image" );

	SNPrintf( buf, 200, "assets\\%s.png", name );
	xmlPrinter->PushText( buf );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"color" );
	xmlPrinter->PushText( "16777215" );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"opacity" );
	xmlPrinter->PushText( "100.0" );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"angle" );
	xmlPrinter->PushText( "0.0" );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"xflip" );
	xmlPrinter->PushText( "0" );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"yflip" );
	xmlPrinter->PushText( "0" );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"width" );
	xmlPrinter->PushText( bounds.Width()  );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"height" );
	xmlPrinter->PushText( bounds.Height() );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"x" );
	xmlPrinter->PushText( bounds.min.x );
	xmlPrinter->CloseElement();

	xmlPrinter->OpenElement(	"y" );
	xmlPrinter->PushText( bounds.min.y );
	xmlPrinter->CloseElement();

	xmlPrinter->CloseElement();
}


void AnimationScene::FinishXML()
{
	xmlPrinter->CloseElement();

	delete xmlPrinter;
	delete xmlDocument;
	fclose( scmlFP );
}


void AnimationScene::Draw3D( U32 deltaTime )
{
	if ( doExport ) {

		Rectangle2I size;
		char buf[256];
		const char* part = "reference";
		if ( exportCount >= 0 ) {
			part = model->GetResource()->header.BoneNameFromID( exportCount );
			model->SetBoneFilter( exportCount );
		}

		bool glow = engine->GetGlow();
		engine->SetGlow( false );
		engine->Draw( deltaTime );
		engine->SetGlow( glow );

		if ( part && *part ) {
			SNPrintf( buf, 256, "./resin/%s/assets/%s", model->GetResource()->header.name.c_str(), part );
			ScreenCapture( buf, false, true, true, &size );
			if ( exportCount < 0 ) {
				origin.x = Mean( size.min.x, size.max.x );
				origin.y = size.max.y;
				size.min -= origin;
				size.max -= origin;
				InitXML( size );
				InitFrame();
				SNPrintf( buf, 256, "PUR=%.1f", (float)size.Height() / model->GetResource()->AABB().SizeY() );
				pixelUnitRatio.SetText( buf );
			}
			else {
				size.min -= origin;
				size.max -= origin;
				PushSprite( part, size );
			}
		}
		++exportCount;
		if ( exportCount == EL_MAX_BONES ) {
			doExport = false;
			model->ClearParam();
			FinishFrame();
			FinishXML();
		}
	}
	else {
		model->DeltaAnimation( deltaTime );
		engine->Draw( deltaTime );
	}
}
