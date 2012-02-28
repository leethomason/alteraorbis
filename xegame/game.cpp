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

#include "game.h"
#include "cgame.h"
#include "scene.h"

#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../engine/particle.h"
#include "../engine/gpustatemanager.h"
#include "../engine/renderqueue.h"
#include "../engine/shadermanager.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glperformance.h"
#include "../grinliz/glstringutil.h"

#include "../tinyxml/tinyxml.h"
#include "../version.h"

#include "ufosound.h"

#include <time.h>

using namespace grinliz;
using namespace gamui;

extern long memNewCount;

Game::Game( int width, int height, int rotation, const char* path ) :
	screenport( width, height, rotation ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	debugLevel( 0 ),
	suppressText( false ),
	previousTime( 0 ),
	isDragging( false )
{
	savePath = path;
	char c = savePath[savePath.size()-1];
	if ( c != '\\' && c != '/' ) {	
#ifdef WIN32
		savePath += "\\";
#else
		savePath += "/";
#endif
	}	
	
	Init();
}


void Game::Init()
{
	mainPalette = 0;
	scenePopQueued = false;
	loadSlot = 0;
	currentFrame = 0;
	surface.Set( Surface::RGBA16, 256, 256 );		// All the memory we will ever need (? or that is the intention)

	// Load the database.
	char buffer[260];
	int offset;
	int length;
	PlatformPathToResource( buffer, 260, &offset, &length );
	database0 = new gamedb::Reader();
	database0->Init( 0, buffer, offset );

	GLOUTPUT(( "Game::Init Database initialized.\n" ));

	GLOUTPUT(( "Game::Init stage 10\n" ));
	SoundManager::Create( database0 );
	TextureManager::Create( database0 );
	ImageManager::Create( database0 );
	ModelResourceManager::Create();
	ParticleSystem::Create();

	LoadTextures();
	modelLoader = new ModelLoader();
	LoadModels();
	LoadItemResources();
	LoadAtoms();
	LoadPalettes();

	delete modelLoader;
	modelLoader = 0;

	Texture* textTexture = TextureManager::Instance()->GetTexture( "font" );
	GLASSERT( textTexture );
	UFOText::Create( database0, textTexture, &screenport );

	GLOUTPUT(( "Game::Init complete.\n" ));
}


Game::~Game()
{
	// Roll up to the main scene before saving.
	while( sceneStack.Size() > 1 ) {
		PopScene();
		PushPopScene();
	}

	sceneStack.Top()->scene->DeActivate();
	sceneStack.Top()->Free();
	sceneStack.Pop();

	UFOText::Destroy();
	SoundManager::Destroy();
	ParticleSystem::Destroy();
	ModelResourceManager::Destroy();
	ImageManager::Destroy();
	TextureManager::Destroy();
	delete ShaderManager::Instance();
	delete database0;
}


bool Game::HasSaveFile( int slot ) const
{
	bool result = false;

	FILE* fp = GameSavePath( SAVEPATH_READ, slot );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		long d = ftell( fp );
		if ( d > 100 ) {	// has to be something there: sanity check
			result = true;
		}
		fclose( fp );
	}
	return result;
}


void Game::DeleteSaveFile(int slot )
{
	FILE* fp = GameSavePath( SAVEPATH_WRITE, slot );
	if ( fp ) {
		fclose( fp );
	}
}


void Game::SceneNode::Free()
{
	sceneID = Game::MAX_SCENES;
	delete scene;	scene = 0;
	delete data;	data = 0;
	result = INT_MIN;
}


void Game::PushScene( int sceneID, SceneData* data )
{
	GLOUTPUT(( "PushScene %d\n", sceneID ));
	GLASSERT( sceneQueued.sceneID == MAX_SCENES );
	GLASSERT( sceneQueued.scene == 0 );

	sceneQueued.sceneID = sceneID;
	sceneQueued.data = data;
}


void Game::PopScene( int result )
{
	GLOUTPUT(( "PopScene result=%d\n", result ));
	GLASSERT( scenePopQueued == false );
	scenePopQueued = true;
	if ( result != INT_MAX )
		sceneStack.Top()->result = result;
}


void Game::PopAllAndLoad( int slot )
{
	GLASSERT( scenePopQueued == false );
	GLASSERT( slot > 0 );
	scenePopQueued = true;
	loadSlot = slot;
}


void Game::PushPopScene() 
{
	if ( scenePopQueued || sceneQueued.sceneID != MAX_SCENES ) {
		TextureManager::Instance()->ContextShift();
	}

	while ( ( scenePopQueued || loadSlot ) && !sceneStack.Empty() )
	{
		sceneStack.Top()->scene->DeActivate();
		scenePopQueued = false;
		int result = sceneStack.Top()->result;
		int id     = sceneStack.Top()->sceneID;

		sceneStack.Top()->Free();
		sceneStack.Pop();

		if ( !sceneStack.Empty() ) {
			sceneStack.Top()->scene->Activate();
			sceneStack.Top()->scene->Resize();
			if ( result != INT_MIN ) {
				sceneStack.Top()->scene->SceneResult( id, result );
			}
		}
	}

	if ( loadSlot ) {
		if( HasSaveFile( loadSlot ) ) {
			sceneQueued.sceneID = LoadSceneID();
		}
	}

	if (    sceneQueued.sceneID == MAX_SCENES 
		 && sceneStack.Empty() ) 
	{
		// Unwind and full reset.
		DeleteSaveFile( 0 );
		DeleteSaveFile( 0 );

		PushScene( 0, 0 );
		PushPopScene();
	}
	else if ( sceneQueued.sceneID != MAX_SCENES ) 
	{
		GLASSERT( sceneQueued.sceneID < MAX_SCENES );

		if ( sceneStack.Size() ) {
			sceneStack.Top()->scene->DeActivate();
		}

		SceneNode* oldTop = 0;
		if ( !sceneStack.Empty() ) {
			oldTop = sceneStack.Top();
		}

		SceneNode* node = sceneStack.Push();
		CreateSceneLower( sceneQueued, node );
		sceneQueued.data = 0;
		sceneQueued.Free();

		node->scene->Activate();
		node->scene->Resize();

		if ( oldTop ) 
			oldTop->scene->ChildActivated( node->sceneID, node->scene, node->data );

		if (    node->scene->CanSave() 
			 && sceneStack.Size() == 1 ) 
		{
			FILE* fp = GameSavePath( SAVEPATH_READ, loadSlot );
			if ( fp ) {
				TiXmlDocument doc;
				doc.LoadFile( fp );
				//GLASSERT( !doc.Error() );
				if ( !doc.Error() ) {
					Load( doc );
				}
				fclose( fp );
			}
			loadSlot = 0;	// queried during the load; don't clear until after load.
		}
	}
}


void Game::CreateSceneLower( const SceneNode& in, SceneNode* node )
{
	Scene* scene = CreateScene( in.sceneID, in.data );
	node->scene = scene;
	node->sceneID = in.sceneID;
	node->data = in.data;
	node->result = INT_MIN;
}


void Game::LoadAtoms()
{
	TextureManager* tm = TextureManager::Instance();

	renderAtoms[ATOM_TEXT].Init(   (const void*)UIRenderer::RENDERSTATE_UI_TEXT, 
		                           (const void*)tm->GetTexture( "font" ), 0, 0, 1, 1 );
	renderAtoms[ATOM_TEXT_D].Init( (const void*)UIRenderer::RENDERSTATE_UI_TEXT_DISABLED, 
		                           (const void*)tm->GetTexture( "font" ), 0, 0, 1, 1 );
}	


const gamui::RenderAtom& Game::GetRenderAtom( int id )
{
	GLASSERT( id >= 0 && id < ATOM_COUNT );
	GLASSERT( renderAtoms[id].textureHandle );
	return renderAtoms[id];
}


/*
const gamui::ButtonLook& Game::GetButtonLook( int id )
{
	GLASSERT( id >= 0 && id < LOOK_COUNT );
	return buttonLooks[id];
}
*/


void Game::Load( const TiXmlDocument& doc )
{
	ParticleSystem::Instance()->Clear();

	// Already pushed the BattleScene. Note that the
	// BOTTOM of the stack loads. (BattleScene or GeoScene).
	// A GeoScene will in turn load a BattleScene.
	const TiXmlElement* game = doc.RootElement();
	GLASSERT( StrEqual( game->Value(), "Game" ) );
	const TiXmlElement* scene = game->FirstChildElement();
	sceneStack.Top()->scene->Load( scene );
}


FILE* Game::GameSavePath( SavePathMode mode, int slot ) const
{	
	grinliz::GLString str( savePath );
	str += "game";

	if ( slot > 0 ) {
		str += "-";
		str += '0' + slot;
	}
	str += ".xml";

	FILE* fp = fopen( str.c_str(), (mode == SAVEPATH_WRITE) ? "wb" : "rb" );
	return fp;
}


void Game::SavePathTimeStamp( int slot, GLString* stamp )
{
	*stamp = "";
	FILE* fp = GameSavePath( SAVEPATH_READ, slot );
	if ( fp ) {
		char buf[400];
		for( int i=0; i<10; ++i ) {
			fgets( buf, 400, fp );
			buf[399] = 0;
			const char* p = strstr( buf, "timestamp" );
			if ( p ) {
				const char* q0 = strstr( p+1, "\"" );
				if ( q0 ) {
					const char* q1 = strstr( q0+1, "\"" );
					if ( q1 && (q1-q0)>2 ) {
						stamp->append( q0+1, q1-q0-1 );
					}
				}
				break;
			}
		}
		fclose( fp );
	}
}


void Game::Save( int slot, bool saveGeo, bool saveTac )
{
	// For loading, the BOTTOM loads and then loads higher scenes.
	// For saving, the GeoScene saves itself before pushing the tactical
	// scene, so save from the top back. (But still need to save if
	// we are in a character scene for example.)
	for( SceneNode* node=sceneStack.BeginTop(); node; node=sceneStack.Next() ) {
		if ( node->scene->CanSave() )
		{
			FILE* fp = GameSavePath( SAVEPATH_WRITE, slot );
			GLASSERT( fp );
			if ( fp ) {
				XMLUtil::OpenElement( fp, 0, "Game" );
				XMLUtil::Attribute( fp, "version", VERSION );
				XMLUtil::Attribute( fp, "sceneID", node->sceneID );

				// Somewhat scary c code to get the current time.
				char buf[40];
			    time_t rawtime;
				struct tm * timeinfo;  
				time ( &rawtime );
				timeinfo = localtime ( &rawtime );
				const char* atime = asctime( timeinfo );

				StrNCpy( buf, atime, 40 );
				buf[ strlen(buf)-1 ] = 0;	// remove trailing newline.

				XMLUtil::Attribute( fp, "timestamp", buf );
				XMLUtil::SealElement( fp );

				node->scene->Save( fp, 1 );
	
				XMLUtil::CloseElement( fp, 0, "Game" );

				fclose( fp );
				break;
			}
		}
	}
}


void Game::DoTick( U32 _currentTime )
{
	{
		GRINLIZ_PERFTRACK

		currentTime = _currentTime;
		if ( previousTime == 0 ) {
			previousTime = currentTime-1;
		}
		U32 deltaTime = currentTime - previousTime;

		if ( markFrameTime == 0 ) {
			markFrameTime			= currentTime;
			frameCountsSinceMark	= 0;
			framesPerSecond			= 0.0f;
		}
		else {
			++frameCountsSinceMark;
			if ( currentTime - markFrameTime > 500 ) {
				framesPerSecond		= 1000.0f*(float)(frameCountsSinceMark) / ((float)(currentTime - markFrameTime));
				// actually K-tris/second
				markFrameTime		= currentTime;
				frameCountsSinceMark = 0;
			}
		}

		// Limit so we don't ever get big jumps:
		if ( deltaTime > 100 )
			deltaTime = 100;

		GPUShader::ResetState();
		GPUShader::Clear();

		Scene* scene = sceneStack.Top()->scene;
		scene->DoTick( currentTime, deltaTime );

		Rectangle2I clip2D, clip3D;
		clip2D.SetInvalid();
		clip3D.SetInvalid();
		int renderPass = scene->RenderPass( &clip3D, &clip2D );
		GLASSERT( renderPass );
	
		if ( renderPass & Scene::RENDER_3D ) {
			GRINLIZ_PERFTRACK_NAME( "Game::DoTick 3D" );
			screenport.SetPerspective( clip3D.Width() > 0 ? &clip3D : 0 );

			scene->Draw3D();
/*		
			const grinliz::Vector3F* eyeDir = engine->camera.EyeDir3();
			ParticleSystem* particleSystem = ParticleSystem::Instance();
			particleSystem->Update( deltaTime, currentTime );
			particleSystem->Draw( eyeDir, engine->GetMap() ? &engine->GetMap()->GetFogOfWar() : 0 );
*/
		}

		{
			GRINLIZ_PERFTRACK_NAME( "Game::DoTick UI" );

			// UI Pass
			screenport.SetUI( clip2D.IsValid() ? &clip2D : 0 ); 
			if ( renderPass & Scene::RENDER_3D ) {
				scene->RenderGamui3D();
			}
			if ( renderPass & Scene::RENDER_2D ) {
				screenport.SetUI( clip2D.IsValid() ? &clip2D : 0 );
				scene->DrawHUD();
				scene->RenderGamui2D();
			}
		}
//		SoundManager::Instance()->PlayQueuedSounds();
	}

	const int Y = 305;
	#ifndef GRINLIZ_DEBUG_MEM
	const int memNewCount = 0;
	#endif
#if 1
	if ( !suppressText ) {
		UFOText* ufoText = UFOText::Instance();
		if ( debugLevel >= 1 ) {
			ufoText->Draw(	0,  Y, "#%d %5.1ffps vbo=%d ps=%d", 
							VERSION, 
							framesPerSecond, 
							GPUShader::SupportsVBOs() ? 1 : 0,
							PointParticleShader::IsSupported() ? 1 : 0 );
		}
		if ( debugLevel >= 2 ) {
			ufoText->Draw(	0,  Y-15, "%4.1fK/f %3ddc/f", 
							(float)GPUShader::TrianglesDrawn()/1000.0f,
							GPUShader::DrawCalls() );
		}
	}
#endif
	GPUShader::ResetTriCount();

#ifdef EL_SHOW_MODELS
	int k=0;
	while ( k < nModelResource ) {
		int total = 0;
		for( unsigned i=0; i<modelResource[k].nGroups; ++i ) {
			total += modelResource[k].atom[i].trisRendered;
		}
		UFODrawText( 0, 12+12*k, "%16s %5d K", modelResource[k].name, total );
		++k;
	}
#endif

#ifdef GRINLIZ_PROFILE
	const int SAMPLE = 8;
	if ( (currentFrame & (SAMPLE-1)) == 0 ) {
		Performance::SampleData();
	}
	for( int i=0; i<Performance::NumData(); ++i ) {
		const PerformanceData& data = Performance::GetData( i );

		UFOText::Draw( 60,  20+i*12, "%s", data.name );
		UFOText::Draw( 300, 20+i*12, "%.3f", data.normalTime );
		UFOText::Draw( 380, 20+i*12, "%d", data.functionCalls/SAMPLE );
	}
#endif

	previousTime = currentTime;
	++currentFrame;

	PushPopScene();
}



bool Game::PopSound( int* database, int* offset, int* size )
{
	return SoundManager::Instance()->PopSound( database, offset, size );
}


void Game::Tap( int action, int wx, int wy )
{
	// The tap is in window coordinate - need to convert to view.
	Vector2F window = { (float)wx, (float)wy };
	Vector2F view;
	screenport.WindowToView( window, &view );

	grinliz::Ray world;
	screenport.ViewToWorld( view, 0, &world );

#if 0
	{
		Vector2F ui;
		screenport.ViewToUI( view, &ui );
		if ( action != GAME_TAP_MOVE )
			GLOUTPUT(( "Tap: action=%d window(%.1f,%.1f) view(%.1f,%.1f) ui(%.1f,%.1f)\n", action, window.x, window.y, view.x, view.y, ui.x, ui.y ));
	}
#endif
	sceneStack.Top()->scene->Tap( action, view, world );
}


void Game::Zoom( int style, float distance )
{
	sceneStack.Top()->scene->Zoom( style, distance );
}


void Game::Rotate( float degrees )
{
	sceneStack.Top()->scene->Rotate( degrees );
}


void Game::CancelInput()
{
	isDragging = false;
}


void Game::HandleHotKeyMask( int mask )
{
	sceneStack.Top()->scene->HandleHotKeyMask( mask );
	if ( mask & GAME_HK_TOGGLE_DEBUG_TEXT ) {
		SetDebugLevel( GetDebugLevel() + 1 );
	}
}


void Game::DeviceLoss()
{
	TextureManager::Instance()->DeviceLoss();
	ModelResourceManager::Instance()->DeviceLoss();
	ShaderManager::Instance()->DeviceLoss();
	GPUShader::ResetState();
}


void Game::Resize( int width, int height, int rotation ) 
{
	screenport.Resize( width, height, rotation );
	sceneStack.Top()->scene->Resize();
}




