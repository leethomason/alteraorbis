#include "animationscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../engine/model.h"
//#include "../shared/lodepng.h"
//#include "../win32/glew.h"

extern void ScreenCapture( const char* baseFilename, bool appendCount, bool trim, bool makeTransparent, grinliz::Rectangle2I* size );

using namespace gamui;
using namespace grinliz;

AnimationScene::AnimationScene( LumosGame* game ) : Scene( game )
{
	currentBone = -1;
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

	ortho.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	ortho.SetSize( layout.Width(), layout.Height() );
	ortho.SetText( "ortho" );

	exportSCML.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	exportSCML.SetSize( layout.Width()*2.0f, layout.Height() );
	exportSCML.SetText( "export" );

	engine = new Engine( port, game->GetDatabase(), 0 );

	const ModelResource* res = ModelResourceManager::Instance()->GetModelResource( "humanFemale" );
	GLASSERT( res );
	model = engine->AllocModel( res );
	model->SetPos( 1, 0, 1 );
	engine->CameraLookAt( 1, 1, 5 );
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
			SNPrintf( buf, 256, "./resin/%s/assets/%s.png", model->GetResource()->header.name.c_str(), part );
			ScreenCapture( buf, false, true, true, &size );
		}
		++exportCount;
		if ( exportCount == EL_MAX_BONES ) {
			doExport = false;
			model->ClearParam();
		}
	}
	else {
		engine->Draw( deltaTime );
	}
}
