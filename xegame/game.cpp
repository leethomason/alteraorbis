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
#include "../engine/settings.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../Shiny/include/Shiny.h"
#include "../grinliz/glstringutil.h"

#include "../audio/xenoaudio.h"
#include "../tinyxml2/tinyxml2.h"
#include "../version.h"

#include "istringconst.h"

#include "../script/itemscript.h"

#include <time.h>
#include <direct.h>	// for _mkdir

using namespace grinliz;
using namespace gamui;
using namespace tinyxml2;

extern long memNewCount;

const Game::Palette* Game::mainPalette = 0;

Game::Game( int width, int height, int rotation, int uiHeight ) :
	screenport( width, height, uiHeight ),
	markFrameTime( 0 ),
	frameCountsSinceMark( 0 ),
	framesPerSecond( 0 ),
	debugUI( false ), 
	debugText( false ),
	perfText( false ),
	perfFrameCount( 0 ),
	renderUI( true ),
	previousTime( 0 ),
	isDragging( false )
{
	IStringConst::Init();

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
	xenoAudio = new XenoAudio(database0, buffer);
	xenoAudio->SetAudio(true);

	GLOUTPUT(( "Game::Init Database initialized.\n" ));

	GLOUTPUT_REL(( "Game::Init stage 10\n" ));
	TextureManager::Create( database0 );
	ImageManager::Create( database0 );
	ModelResourceManager::Create();
	AnimationResourceManager::Create();

	GLString settingsPath;
	GetSystemPath(GAME_SAVE_DIR, "settings.xml", &settingsPath );
	SettingsManager::Create(settingsPath.c_str());

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
	UFOText::Create( database0, textTexture );

	_mkdir( "save" );

	itemDefDB = new ItemDefDB();
	itemDefDB->Load( "./res/itemdef.xml" );
	itemDefDB->DumpWeaponStats();

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
	SettingsManager::Destroy();
	AnimationResourceManager::Destroy();
	ModelResourceManager::Destroy();
	ImageManager::Destroy();
	TextureManager::Destroy();

	delete ShaderManager::Instance();	// handles device loss - should be near the end.
	delete GPUDevice::Instance();
	delete itemDefDB;
	delete xenoAudio;
	delete database0;
	delete StringPool::Instance();
	PROFILE_DESTROY();

	GLOUTPUT_REL(( "Game destructor complete.\n" ));
	GameItem::trackWallet = true;
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

	const gamedb::Item* item = GetDatabase()->Root()->Child( "models" )->Child( name );
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

	GLString fullpath;
	GetSystemPath(GAME_SAVE_DIR, path, &fullpath);

	FILE* fp = fopen( fullpath.c_str(), "rb" );
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
	GLString fullpath;
	GetSystemPath(GAME_SAVE_DIR, path, &fullpath);

	FILE* fp = fopen(fullpath.c_str(), "w");
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

		// Grab the data out.
		SceneData* data = sceneStack.Top()->data;
		sceneStack.Top()->data = 0;	// prevent it from getting deleted.

		sceneStack.Top()->Free();
		sceneStack.Pop();

		if ( !sceneStack.Empty() ) {
			sceneStack.Top()->scene->Activate();
			sceneStack.Top()->scene->Resize();
			sceneStack.Top()->scene->SceneResult( id, result, data );
		}
		delete data;
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
	//str = "save/";
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
void Game::PrintPerf( int depth, const PerfData& data )
{
	static const int X = 350;
	static const int Y = 100;

	UFOText* ufoText = UFOText::Instance();
	ufoText->Draw( X + 15*depth, Y+perfY, "%s", data.name );
	ufoText->Draw( X+280,		 Y+perfY, "%6.2f  %4d", data.inclusiveMSec, data.callCount );
	perfY += 20;
}
*/

void Game::PrintPerf()
{
	++perfFrameCount;
	if ( perfFrameCount == 10 ) {
		std::string str = PROFILE_GET_TREE_STRING();
		profile = str.c_str();
		perfFrameCount = 0;
	}
	char buf[512];

	static const int X = 250;
	static const int Y = 100;

	const char* p = profile.c_str();
	const char* end = p + profile.size();

	UFOText* ufoText = UFOText::Instance();
	int perfY = 0;

	ufoText->SetFixed( true );
	while( p < end ) {
		char* q = buf;
		while( p < end && *p != '\n' ) {
			*q++ = *p++;
		}
		*q = 0;
		ufoText->Draw( X, Y+perfY, "%s", buf );
		perfY += 16;
		++p;
	}
	ufoText->SetFixed( false );
}


void Game::DoTick( U32 _currentTime )
{
	GPUDevice* device = GPUDevice::Instance();
	{
		//GRINLIZ_PERFTRACK
		PROFILE_FUNC();

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

		device->ResetState();

		Scene* scene = sceneStack.Top()->scene;

		Color4F cc = scene->ClearColor();
		device->Clear( cc.r, cc.g, cc.b, cc.a );

		scene->DoTick( deltaTime );

		{
			//GRINLIZ_PERFTRACK_NAME( "Game::DoTick 3D" );
			PROFILE_BLOCK( DoTick3D );

			screenport.SetPerspective();
			scene->Draw3D( deltaTime );
		}

		{
			//GRINLIZ_PERFTRACK_NAME( "Game::DoTick UI" );
			PROFILE_BLOCK( DoTickUI );
			GLASSERT( scene );

			// UI Pass
			screenport.SetUI(); 

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
	ufoText->Draw(	0,  Y, "#%d %5.1ffps %4.1fK/f %3ddc/f quads=%.1fK/f fver=%d", 
					VERSION, 
					framesPerSecond, 
					(float)device->TrianglesDrawn()/1000.0f,
					device->DrawCalls(),
					(float)device->QuadsDrawn()/1000.0f,
					CURRENT_FILE_VERSION );
	if ( debugText ) {
		sceneStack.Top()->scene->DrawDebugText();
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

	/*
	if ( GetPerfLevel() ) {
		Performance::EndFrame();
		if ( ++perfFrameCount == 10 ) {
			perfFrameCount = 0;
			Performance::Process();
		}
		perfY = 0;
		Performance::Walk( this );
	}
	*/
	PROFILE_UPDATE();

	if ( perfText ) {
		PrintPerf();
	}
#endif
	screenport.SetUI();
	UFOText::Instance()->FinalDraw();

	device->ResetTriCount();
	previousTime = currentTime;
	++currentFrame;

	PushPopScene();
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
	if (action == GAME_MOVE_WHILE_UP) {
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
	sceneStack.Top()->scene->Rotate( -degrees );
}


void Game::Pan(int action, float wx, float wy)
{
	//GLOUTPUT(("Pan: action=%d x=%.1f y=%.1f\n", action, x, y));
	Vector2F window = { wx, wy };
	Vector2F view;
	screenport.WindowToView(window, &view);

	grinliz::Ray world;
	screenport.ViewToWorld(view, 0, &world);

	sceneStack.Top()->scene->Pan(action, view, world);
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
	if ( key == GAME_HK_TOGGLE_DEBUG_UI ) {
		debugUI = !debugUI;
		sceneStack.Top()->scene->Resize();
	}
	if ( key == GAME_HK_TOGGLE_DEBUG_TEXT ) {
		debugText = !debugText;
		sceneStack.Top()->scene->Resize();
	}
	else if ( key == GAME_HK_TOGGLE_PERF ) {
		perfText = !perfText;
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
	delete GPUDevice::Instance();
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


