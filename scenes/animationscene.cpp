/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "animationscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../engine/model.h"
#include "../engine/animation.h"

#include <direct.h>

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

	modelLeft.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	modelLeft.SetSize( layout.Width(), layout.Height() );
	modelLeft.SetText( "<-" );

	modelRight.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	modelRight.SetSize( layout.Width(), layout.Height() );
	modelRight.SetText( "->" );	

	modelName.Init( &gamui2D );
	modelName.SetText( "no model" );

	ortho.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	ortho.SetSize( layout.Width(), layout.Height() );
	ortho.SetText( "ortho" );

	instance.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	instance.SetSize( layout.Width(), layout.Height() );
	instance.SetText( "3x" );

	static const char* triggerLabels[NUM_TRIGGERS] = {
		"particle", "gun", "knife", "ax"
	};

	for( int i=0; i<NUM_TRIGGERS; ++i ) {
		triggerToggle[i].Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
		triggerToggle[i].SetSize( layout.Width(), layout.Height() );
		triggerToggle[i].SetText( triggerLabels[i] );
		if ( i > 0 ) {
			triggerToggle[0].AddToToggleGroup( &triggerToggle[i] );
		}
	}

	exportSCML.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	exportSCML.SetSize( layout.Width(), layout.Height() );
	exportSCML.SetText( "export" );

	pixelUnitRatio.Init( &gamui2D );

	engine = new Engine( port, game->GetDatabase(), 0 );
	engine->particleSystem->LoadParticleDefs( "./res/particles.xml" );

	engine->lighting.ambient.Set( 0.5f, 0.5f, 0.5f );
	engine->lighting.direction.Set( -1, 1, 1 );
	engine->lighting.direction.Normalize();
	engine->lighting.diffuse.Set( 0.5f, 0.5f, 0.5f );

	int count=0;
	for( int i=0; i<NUM_MODELS; ++i ) {
		model[i] = 0;
	}

	currentModel = 0;
	const ModelResource* const* resArr = ModelResourceManager::Instance()->GetAllResources( &count );
	for( int i=0; i<count; ++i ) {
		if ( !resArr[i]->header.animation.empty() ) {
			resourceArr.Push( resArr[i] );
			if ( resArr[i]->header.name == "humanFemale" ) {
				currentModel = resourceArr.Size()-1;
			}
		}
	}

	triggerModel = 0;
	LoadModel();

	engine->CameraLookAt( 1, 1, 5 );
	UpdateAnimationInfo();
}


AnimationScene::~AnimationScene()
{
	for( int i=0; i<NUM_MODELS; ++i ) {
		engine->FreeModel( model[i] );
	}
	if ( triggerModel ) {
		engine->FreeModel( triggerModel );
	}
	delete engine;
}


void AnimationScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	//const Screenport& port = game->GetScreenport();
	lumosGame->PositionStd( &okay, 0 );

	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &boneLeft,		0, -2 );
	layout.PosAbs( &boneName,		1, -2 );
	layout.PosAbs( &boneRight,		3, -2 );
	layout.PosAbs( &ortho,			4, -2 );
	layout.PosAbs( &instance,		5, -2 );
	for( int i=0; i<NUM_TRIGGERS; ++i ) {
		layout.PosAbs( &triggerToggle[i], 6+i, -2 );
	}

	layout.PosAbs( &exportSCML,		4, -1 );
	layout.PosAbs( &pixelUnitRatio, 5, -1 );

	layout.PosAbs( &animLeft,	0, 0 );
	layout.PosAbs( &animName,	1, 0 );
	layout.PosAbs( &animRight,	3, 0 );

	layout.PosAbs( &modelLeft,	0, -3 );
	layout.PosAbs( &modelName,	1, -3 );
	layout.PosAbs( &modelRight,	3, -3 );
}


void AnimationScene::LoadModel()
{
	if ( currentModel >= resourceArr.Size() ) currentModel = 0;
	if ( currentModel < 0 ) currentModel = resourceArr.Size()-1;

	const ModelResource* res = resourceArr[currentModel];;
	GLASSERT( res );
	for( int j=0; j<NUM_MODELS; ++j ) {
		if ( model[j] ) {
			engine->FreeModel( model[j] );
		}
		model[j] = engine->AllocModel( res );
		model[j]->SetPos( 1.0f + (float)j*0.6f, 0, 1 );
	}
	SetModelVis( false );
	model[1]->SetAnimationRate( 0.8f );
	model[2]->SetAnimationRate( 1.2f );
}


void AnimationScene::UpdateBoneInfo()
{
	if ( currentBone == -1 ) {
		boneName.SetText( "all" );
		for( int i=0; i<NUM_MODELS; ++i )
			model[i]->ClearParam();
	}
	else {
		GLString str;
		IString name = model[0]->GetBoneName( currentBone );
		str.Format( "%d:%s", currentBone, !name.empty() ? name.c_str() : "none" );
		boneName.SetText( str.c_str() );
	
		int id[4] = { currentBone, -1, -1, -1 };
		for( int i=0; i<NUM_MODELS; ++i )
			model[i]->SetBoneFilter( id );
	}
}


void AnimationScene::UpdateModelInfo()
{
	const ModelResource* res = resourceArr[currentModel];
	modelName.SetText( res->header.name.c_str() );
	if ( model[0]->GetResource() != res ) {
		LoadModel();
	}
	modelName.SetText( model[0]->GetResource()->header.name.c_str() );
}


void AnimationScene::UpdateAnimationInfo()
{
	const AnimationResource* res = model[0]->GetAnimationResource();
	if ( res ) {
		int nAnim = res->NumAnimations();
		GLASSERT( currentAnim >=0 && currentAnim < nAnim );

		const char* name = "none";
		if ( nAnim > 0 ) {
			name = res->AnimationName( currentAnim );
			AnimationType type = AnimationResource::NameToType( name );
			for( int i=0; i<NUM_MODELS; ++i ) {
				model[i]->SetAnimation( type, 500, false );
			}
		}
		animName.SetText( name );
		exportSCML.SetEnabled( false );
	}
	else {
		animName.SetText( "none" );
		exportSCML.SetEnabled( true );
	}
}


void AnimationScene::SetModelVis( bool onlyShowOneUnrotated )
{
	for( int i=1; i<NUM_MODELS; ++i ) {
		if ( onlyShowOneUnrotated || !instance.Down() ) {
			model[i]->SetFlag( Model::MODEL_INVISIBLE );
		}
		else {
			model[i]->ClearFlag( Model::MODEL_INVISIBLE );
		}
	}
	for( int i=0; i<NUM_MODELS; ++i ) {
		if ( onlyShowOneUnrotated )
			model[i]->SetYRotation( 0 );
		else
			model[i]->SetYRotation( 20 );
	}
}


void AnimationScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
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
		const AnimationResource* res = model[0]->GetAnimationResource();
		GLASSERT( res );
		if ( currentAnim >= res->NumAnimations() ) {
			currentAnim = 0;
		}
	}
	else if ( item == &animLeft ) {
		-- currentAnim;
		if ( currentAnim < 0 ) {
			const AnimationResource* res = model[0]->GetAnimationResource();
			currentAnim = res->NumAnimations()-1;
		}
	}
	else if ( item == &ortho ) {
		Screenport* port = engine->GetScreenportMutable();
		if ( ortho.Down() ) {
			port->SetOrthoCamera( true, 0, 3.0f );
			static const Vector3F DIR = { -1, 0, 0 };	// FIXME: bug in camera code. (EEK.) Why is the direction negative???
			static const Vector3F UP  = { 0, 1, 0 };
			engine->camera.SetPosWC( -5, 0.55f, 1 );
			engine->camera.SetDir( DIR, UP ); 
			SetModelVis( true );
		}
		else {
			port->SetOrthoCamera( false, 0, 0 );
			engine->CameraLookAt( 1, 1, 5 );
			SetModelVis( false );
		}
	}
	else if ( item == &instance ) {
		SetModelVis( false );
	}
	else if ( item == &exportSCML ) {
		doExport = true;
		exportCount = -1;
	}
	else if ( item == &modelRight ) {
		++currentModel;
		if ( currentModel == resourceArr.Size() ) currentModel = 0;
		currentAnim = 0;
	}
	else if ( item == &modelLeft ) {
		--currentModel;
		if ( currentModel < 0 ) currentModel = resourceArr.Size()-1;
		currentAnim = 0;
	}

	UpdateModelInfo();
	UpdateAnimationInfo();
	UpdateBoneInfo();
}


void AnimationScene::WriteXML()
{
	char buf[256];
	const ModelHeader& header = model[0]->GetResource()->header;
	
	const char* name = header.name.c_str();
	SNPrintf( buf, 256, ".\\resin\\%s", name );
	_mkdir( buf );
	SNPrintf( buf, 256, "./resin/%s/%s.scml", name, name );

	FILE* fp = fopen( buf, "w" );
	XMLPrinter* xmlPrinter  = new XMLPrinter( fp );

	int nBones=0;
	for( ; nBones < EL_MAX_BONES; ++nBones ) {
		if ( header.boneName[nBones].empty() )
			break;
	}
	GLASSERT( nBones );

	xmlPrinter->OpenElement( "spriter_data" );
	xmlPrinter->PushAttribute( "scml_version", "1.0" );

	xmlPrinter->OpenElement(	"folder" );
	xmlPrinter->PushAttribute( "id", "0" );
	xmlPrinter->PushAttribute( "name", "assets" );

	for( int i=0; i<nBones; ++i ) {
		xmlPrinter->OpenElement( "file" );
		xmlPrinter->PushAttribute( "id", i );
		GLString fname = MakeFilename( header.boneName[i].c_str() );
		SNPrintf( buf, 256, "assets/%s.png", fname.c_str() );
		xmlPrinter->PushAttribute( "name", buf );
		xmlPrinter->PushAttribute( "width", partSize[i].Width() );
		xmlPrinter->PushAttribute( "height", partSize[i].Height() );

		xmlPrinter->CloseElement();
	}
	xmlPrinter->CloseElement();	// folder

	xmlPrinter->OpenElement( "entity" );
	xmlPrinter->PushAttribute( "id", 0 );
	xmlPrinter->PushAttribute( "name", "char_000" );

	xmlPrinter->OpenElement( "animation" );
	xmlPrinter->PushAttribute( "id", 0 );
	xmlPrinter->PushAttribute( "name", "reference" );
	xmlPrinter->PushAttribute( "length", 5000 );
	xmlPrinter->PushAttribute( "looping", "false" );

	xmlPrinter->OpenElement( "mainline" );
	
	xmlPrinter->OpenElement( "key" );
	xmlPrinter->PushAttribute( "id", 0 );

	for( int i=0; i<nBones; ++i ) {
		xmlPrinter->OpenElement( "object_ref" );
		xmlPrinter->PushAttribute( "id", i );
		xmlPrinter->PushAttribute( "timeline", i );
		xmlPrinter->PushAttribute( "key", 0 );
		xmlPrinter->PushAttribute( "z_index", i );
		xmlPrinter->CloseElement();
	}
	xmlPrinter->CloseElement();	// key
	xmlPrinter->CloseElement(); // mainline

	for( int i=0; i<nBones; ++i ) {
		xmlPrinter->OpenElement( "timeline" );
		xmlPrinter->PushAttribute( "id", i );

		xmlPrinter->OpenElement( "key" );
		xmlPrinter->PushAttribute( "id", 0 );

		xmlPrinter->OpenElement( "object" );
		xmlPrinter->PushAttribute( "folder", 0 );
		xmlPrinter->PushAttribute( "file", i );
		xmlPrinter->PushAttribute( "x", partSize[i].min.x );
		xmlPrinter->PushAttribute( "y", -partSize[i].min.y );
		xmlPrinter->PushAttribute( "a", 0.5f );

		xmlPrinter->CloseElement();	 //object
		xmlPrinter->CloseElement();	 //key
		xmlPrinter->CloseElement();	 //timeline
	}
	xmlPrinter->CloseElement();	 //animation
	xmlPrinter->CloseElement();	 //entity
	xmlPrinter->CloseElement();	 //spriter_data

	delete xmlPrinter;
	fclose( fp );
}


void AnimationScene::DoTick( U32 deltaTime )
{
	grinliz::CArray<int, EL_MAX_METADATA> metaDataEvents;
	for( int i=0; i<NUM_MODELS; ++i ) {
		model[i]->DeltaAnimation( deltaTime, (i==0) ? &metaDataEvents : 0, 0 );
		model[i]->EmitParticles( engine->particleSystem, engine->camera.EyeDir3(), deltaTime );
	}

	if ( model[0]->HasAnimation() && model[0]->GetResource()->GetMetaData( "trigger" )) {
		static const Vector3F UP = { 0, 1, 0 };
		static const Vector3F POS = { 0,0,0 };
		Matrix4 xform;
		model[0]->CalcMetaData( "trigger", &xform );
		Vector3F p = xform * POS;

		for( int i=0; i<metaDataEvents.Size(); ++i ) {
			engine->particleSystem->EmitPD( "derez", p, UP, engine->camera.EyeDir3(), 0 );	
		}

		if ( triggerToggle[PARTICLE].Down() ) {
			engine->particleSystem->EmitPD( "spell", p, UP, engine->camera.EyeDir3(), 0 ); 
			if ( triggerModel ) { engine->FreeModel( triggerModel ); triggerModel = 0;	}
		}
		else {
			const ModelResource* res = 0;
			if		( triggerToggle[GUN].Down() )	{ res = ModelResourceManager::Instance()->GetModelResource( "testgun" ); }
			else if ( triggerToggle[KNIFE].Down() )	{ res = ModelResourceManager::Instance()->GetModelResource( "ring" ); }
			else if ( triggerToggle[AX].Down() )	{ res = ModelResourceManager::Instance()->GetModelResource( "ax" ); }

			if ( triggerModel && triggerModel->GetResource() != res ) {
				engine->FreeModel( triggerModel );
				triggerModel = 0;
			}

			if( res && !triggerModel ) {
				triggerModel = engine->AllocModel( res );
			}

			if ( triggerModel ) {
				Matrix4 xform;
				model[0]->CalcMetaData( "trigger", &xform );
				triggerModel->SetTransform( xform );
			}
		}
	}
	else {
		if ( triggerModel ) { engine->FreeModel( triggerModel ); triggerModel = 0;	}
	}
}


GLString AnimationScene::MakeFilename( const char* in )
{
	GLString s = in;
	while( true ) {
		int i = s.find( '.' );
		if ( i == s.size() )
			break;
		s[i] = '_';
	}
	return s;
}


void AnimationScene::Draw3D( U32 deltaTime )
{
	if ( doExport ) {
		SetModelVis( true );

		char buf[256];
		const char* part = "reference";
		if ( exportCount >= 0 ) {
			part = model[0]->GetResource()->header.boneName[exportCount].c_str();
			int id[4] = { exportCount, -1, -1, -1 };
			model[0]->SetBoneFilter( id );
		}

		bool glow = engine->Glow();
		engine->SetGlow( false );
		engine->Draw( deltaTime );
		engine->SetGlow( glow );

		if ( part && *part ) {
			SNPrintf( buf, 256, "./resin/%s/assets", model[0]->GetResource()->header.name.c_str() );
			_mkdir( buf );
			GLString fname = MakeFilename( part );
			SNPrintf( buf, 256, "./resin/%s/assets/%s", model[0]->GetResource()->header.name.c_str(), fname.c_str() );

			if ( exportCount < 0 ) {
				ScreenCapture( buf, false, true, true, &size );
				origin.x = Mean( size.min.x, size.max.x );
				origin.y = size.max.y;
				size.min -= origin;
				size.max -= origin;
				SNPrintf( buf, 256, "PUR=%.1f", (float)size.Height() / model[0]->GetResource()->AABB().SizeY() );
				pixelUnitRatio.SetText( buf );
			}
			else {
				ScreenCapture( buf, false, true, true, &partSize[exportCount] );
				partSize[exportCount].min -= origin;
				partSize[exportCount].max -= origin;
			}
		}
		++exportCount;
		if ( exportCount == EL_MAX_BONES ) {
			doExport = false;
			WriteXML();
			for( int i=0; i<NUM_MODELS; ++i ) 
				model[i]->ClearParam();
			SetModelVis( false );
		}
	}
	else {
		engine->Draw( deltaTime );
	}
}
