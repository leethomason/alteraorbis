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
#include "../engine/animation.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glperformance.h"
#include "../grinliz/glstringutil.h"

#include "../tinyxml2/tinyxml2.h"
#include "../version.h"

#include "ufosound.h"

#include <time.h>
#include <direct.h>	// for _mkdir

using namespace grinliz;
using namespace gamui;
using namespace tinyxml2;

extern long memNewCount;

const Game::Palette* Game::mainPalette = 0;

Game::Game( int width, int height, int rotation, int uiHeight, const char* path ) :
	screenport( width, height, uiHeight ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	debugLevel( 1 ),
	perfLevel( 0 ),
	perfFrameCount( 0 ),
	suppressText( false ),
	renderUI( true ),
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
	
	scenePopQueued = false;
	currentFrame = 0;
	surface.Set( Surface::RGBA16, 256, 256 );

	// Load the database.
	char buffer[260];
	int offset;
	int length;
	PlatformPathToResource( buffer, 260, &offset, &length );
	database0 = new gamedb::Reader();
	database0->Init( 0, buffer, offset );

	GLOUTPUT(( "Game::Init Database initialized.\n" ));

	GLOUTPUT_REL(( "Game::Init stage 10\n" ));
	SoundManager::Create( database0 );
	TextureManager::Create( database0 );
	ImageManager::Create( database0 );
	ModelResourceManager::Create();
	AnimationResourceManager::Create();

	LoadTextures();
	modelLoader = new ModelLoader();
	LoadModels();
	LoadAtoms();
	LoadPalettes();
	AnimationResourceManager::Instance()->Load( database0 );

	delete modelLoader;
	modelLoader = 0;

	Texture* textTexture = TextureManager::Instance()->GetTexture( "font" );
	GLASSERT( textTexture );
	UFOText::Create( database0, textTexture, &screenport );

	_mkdir( "save" );
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

	TextureManager::Instance()->TextureCreatorInvalid( this );
	UFOText::Destroy();
	SoundManager::Destroy();
	AnimationResourceManager::Destroy();
	ModelResourceManager::Destroy();
	ImageManager::Destroy();
	TextureManager::Destroy();
	delete ShaderManager::Instance();
	delete database0;
	delete StringPool::Instance();
	Performance::Free();

	GLOUTPUT_REL(( "Game destructor complete.\n" ));
}


void Game::LoadTextures()
{
	TextureManager* texman = TextureManager::Instance();
	texman->CreateTexture( "white", 2, 2, Surface::RGB16, Texture::PARAM_NONE, this );
}


void Game::CreateTexture( Texture* t )
{
	if ( StrEqual( t->Name(), "white" ) ) {
		U16 pixels[4] = { 0xffff, 0xffff, 0xffff, 0xffff };
		GLASSERT( t->Format() == Surface::RGB16 );
		t->Upload( pixels, 8 );
	}
	else {
		GLASSERT( 0 );
	}
}


void Game::LoadModel( const char* name )
{
	GLASSERT( modelLoader );

	const gamedb::Item* item = database0->Root()->Child( "models" )->Child( name );
	GLASSERT( item );

	ModelResource* res = new ModelResource();
	modelLoader->Load( item, res );
	ModelResourceManager::Instance()->AddModelResource( res );
}


void Game::LoadModels()
{
	// Run through the database, and load all the models.
	const gamedb::Item* parent = database0->Root()->Child( "models" );
	GLASSERT( parent );

	for( int i=0; i<parent->NumChildren(); ++i )
	{
		const gamedb::Item* node = parent->ChildAt( i );
		LoadModel( node->Name() );
	}
}


bool Game::HasFile( const char* path ) const
{
	bool result = false;

	FILE* fp = fopen( path, "rb" );
	if ( fp ) {
		fseek( fp, 0, SEEK_END );
		long d = ftell( fp );
		if ( d > 10 ) {	// has to be something there: sanity check
			result = true;
		}
		fclose( fp );
	}
	return result;
}


void Game::DeleteFile( const char* path )
{
	FILE* fp = fopen( path, "w" );
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
	GLOUTPUT_REL(( "PushScene %d\n", sceneID ));
	GLASSERT( sceneQueued.sceneID == MAX_SCENES );
	GLASSERT( sceneQueued.scene == 0 );

	sceneQueued.sceneID = sceneID;
	sceneQueued.data = data;
}


void Game::PopScene( int result )
{
	GLOUTPUT_REL(( "PopScene result=%d\n", result ));
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
}


void Game::PushPopScene() 
{
	if ( scenePopQueued || sceneQueued.sceneID != MAX_SCENES ) {
		TextureManager::Instance()->ContextShift();
	}

	while ( scenePopQueued && !sceneStack.Empty() )
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

	if (    sceneQueued.sceneID == MAX_SCENES 
		 && sceneStack.Empty() ) 
	{
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


void Game::LoadPalettes()
{
	const gamedb::Item* parent = database0->Root()->Child( "data" )->Child( "palettes" );
	for( int i=0; i<parent->NumChildren(); ++i ) {
		const gamedb::Item* child = parent->ChildAt( i );
		child = database0->ChainItem( child );

		Palette* p = 0;
		if ( palettes.Size() <= i ) 
			p = palettes.PushArr(1);
		else
			p = &palettes[i];
		p->name = child->Name();
		p->dx = child->GetInt( "dx" );
		p->dy = child->GetInt( "dy" );
		p->colors.Clear();
		GLASSERT( (int)p->colors.Capacity() >= p->dx*p->dy );
		p->colors.PushArr( p->dx*p->dy );
		GLASSERT( child->GetDataSize( "colors" ) == (int)(p->dx*p->dy*sizeof(Color4U8)) );
		child->GetData( "colors", (void*)p->colors.Mem(), p->dx*p->dy*sizeof(Color4U8) );
	}
	mainPalette = GetPalette( "mainPalette" );
}


const gamui::RenderAtom& Game::GetRenderAtom( int id )
{
	GLASSERT( id >= 0 && id < ATOM_COUNT );
	GLASSERT( renderAtoms[id].textureHandle );
	return renderAtoms[id];
}


RenderAtom Game::CreateRenderAtom( int uiRendering, const char* assetName, float x0, float y0, float x1, float y1 )
{
	GLASSERT( uiRendering >= 0 && uiRendering < UIRenderer::RENDERSTATE_COUNT );
	TextureManager* tm = TextureManager::Instance();
	return RenderAtom(	(const void*)uiRendering,
						tm->GetTexture( assetName ),
						x0, y0, x1, y1 );
}


const char* Game::GamePath( const char* type, int slot, const char* extension ) const
{	
	CStr<256> str;
	str = savePath.c_str();
	str += "./save/";
	str += type;

	if ( slot > 0 ) {
		str += "-";
		str += '0' + slot;
	}
	str += ".";
	str += extension;
	return StringPool::Intern( str.c_str() ).c_str();
}


/*
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
*/
/*
void Game::SaveGame( int slot )
{
	const char* path = GamePath( "game", 0, "dat" );
	FILE* fp = fopen( path, "w" );
	GLASSERT( fp );
	if ( fp ) {
		XMLPrinter printer( fp );
		printer.OpenElement( "Game" );
		printer.PushAttribute( "version", VERSION );

		// Somewhat scary c code to get the current time.
		char buf[40];
		time_t rawtime;
		struct tm * timeinfo;  
		time ( &rawtime );
		timeinfo = localtime( &rawtime );
		const char* atime = asctime( timeinfo );

		StrNCpy( buf, atime, 40 );
		buf[ strlen(buf)-1 ] = 0;	// remove trailing newline.

		printer.PushAttribute( "timestamp", buf );
		Save( &printer );
		printer.CloseElement();

		fclose( fp );
	}
}


void Game::LoadGame()
{
	const char* path = GamePath( "game", 0, "dat" );
	FILE* fp = fopen( path, "r" );
	GLASSERT( fp );
	if ( fp ) {
		XMLDocument doc;
		doc.LoadFile( fp );
		GLASSERT( !doc.Error() );
		if ( !doc.Error() ) {
			const tinyxml2::XMLElement* gameElement = doc.FirstChildElement( "Game" );
			GLASSERT( gameElement );
			if ( gameElement ) {
				Load( *gameElement );
			}
		}
		fclose( fp );
	}
}
*/

void Game::PrintPerf( int depth, const PerfData& data )
{
	static const int X = 350;
	static const int Y = 100;

	UFOText* ufoText = UFOText::Instance();
	ufoText->Draw( X + 15*depth, Y+perfY, "%s", data.name );
	ufoText->Draw( X+280,		 Y+perfY, "%6.2f  %4d", data.inclusiveMSec, data.callCount );
	perfY += 20;
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

		
		GPUState::ResetState();

		Scene* scene = sceneStack.Top()->scene;

		Color4F cc = scene->ClearColor();
		GPUState::Clear( cc.r, cc.g, cc.b, cc.a );

		scene->DoTick( deltaTime );

		{
			GRINLIZ_PERFTRACK_NAME( "Game::DoTick 3D" );

			screenport.SetPerspective();
			scene->Draw3D( deltaTime );
		}

		{
			GRINLIZ_PERFTRACK_NAME( "Game::DoTick UI" );
			GLASSERT( scene );

			// UI Pass
			screenport.SetUI(); 
			scene->RenderGamui3D();

			if ( renderUI ) {
				screenport.SetUI();
				scene->RenderGamui2D();
			}
		}
	}

	int Y = 0;
	int space = 16;
	#ifndef GRINLIZ_DEBUG_MEM
	const int memNewCount = 0;
	#endif
#if 1
	UFOText* ufoText = UFOText::Instance();
	if ( !suppressText ) {
		if ( debugLevel >= 1 ) {
			ufoText->Draw(	0,  Y, "#%d %5.1ffps vbo=%d %4.1fK/f %3ddc/f", 
							VERSION, 
							framesPerSecond, 
							GPUState::SupportsVBOs() ? 1 : 0,
							(float)GPUState::TrianglesDrawn()/1000.0f,
							GPUState::DrawCalls() );
			sceneStack.Top()->scene->DrawDebugText();
		}
	}
	Y += space;
#endif

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

	if ( GetPerfLevel() ) {
		Performance::EndFrame();
		if ( ++perfFrameCount == 10 ) {
			perfFrameCount = 0;
			Performance::Process();
		}
		perfY = 0;
		Performance::Walk( this );
	}
#endif

	GPUState::ResetTriCount();
	previousTime = currentTime;
	++currentFrame;

	PushPopScene();
}



bool Game::PopSound( int* database, int* offset, int* size )
{
	return SoundManager::Instance()->PopSound( database, offset, size );
}


void Game::MouseMove( int x, int y )
{
	Vector2F window = { (float)x, (float)y };
	Vector2F view;
	screenport.WindowToView( window, &view );

	grinliz::Ray world;
	screenport.ViewToWorld( view, 0, &world );

	sceneStack.Top()->scene->MouseMove( view, world );
}


void Game::Tap( int action, int wx, int wy, int mod )
{
	tapMod = mod;
	if ( action == GAME_TAP_MOVE_UP ) {
		MouseMove( wx, wy );
		return;
	}

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


void Game::HandleHotKey( int key )
{
	if ( key == GAME_HK_TOGGLE_UI ) {
		renderUI = !renderUI;
	}
	if ( key == GAME_HK_TOGGLE_DEBUG_TEXT ) {
		SetDebugLevel( GetDebugLevel() + 1 );
	}
	else if ( key == GAME_HK_TOGGLE_PERF ) {
		SetPerfLevel( GetPerfLevel() + 1 );
	}
	else {
		sceneStack.Top()->scene->HandleHotKey( key );
	}
}


void Game::DeviceLoss()
{
	TextureManager::Instance()->DeviceLoss();
	ModelResourceManager::Instance()->DeviceLoss();
	ShaderManager::Instance()->DeviceLoss();
	GPUState::ResetState();
}


void Game::Resize( int width, int height, int rotation ) 
{
	screenport.Resize( width, height );
	sceneStack.Top()->scene->Resize();
}


const Game::Palette* Game::GetPalette( const char* name ) const
{
	if ( !name || !*name ) {
		name = "mainPalette";
	}
	for( int i=0; i<palettes.Size(); ++i ) {
		if ( StrEqual( name, palettes[i].name ) ) {
			return &palettes[i];
		}
	}
	return 0;
}


