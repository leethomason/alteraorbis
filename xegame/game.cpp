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
#include "platformpath.h"

#include "../engine/text.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../engine/particle.h"
#include "../engine/gpustatemanager.h"
#include "../engine/renderqueue.h"
#include "../engine/shadermanager.h"
#include "../engine/animation.h"
#include "../engine/settings.h"
#include "../engine/platformgl.h"

#include "../grinliz/glmatrix.h"
#include "../grinliz/glutil.h"
#include "../Shiny/include/Shiny.h"
#include "../grinliz/glstringutil.h"

#include "../audio/xenoaudio.h"
#include "../tinyxml2/tinyxml2.h"
#include "../version.h"

#include "istringconst.h"

#include "../script/itemscript.h"

#include "../xegame/chitbag.h"

#include <time.h>
#include "../game/layout.h"

using namespace grinliz;
using namespace gamui;
using namespace tinyxml2;

extern long memNewCount;

const Game::Palette* Game::mainPalette = 0;

Game::Game( int width, int height, int rotation, int uiHeight ) :
	screenport( width, height ),
	aiDebugLog(false),
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
	CHECK_GL_ERROR;
	IStringConst::Init();

#ifdef DEBUG
	Matrix4::Test();
	ChitBag::Test();
	NewsEvent::Test();
#endif

	scenePopQueued = false;
	surface.Set( TEX_RGBA16, 256, 256 );

	// Load the database.
	char buffer[260];
	int offset;
	int length;
	PathToDatabase(buffer, 260, &offset, &length);
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
	GetSystemPath(GAME_SAVE_DIR, "settings2.xml", &settingsPath );
	SettingsManager::Create(settingsPath.c_str());
	SettingsManager* settings = SettingsManager::Instance();
	debugUI = settings->DebugUI();

	LoadTextures();
	modelLoader = new ModelLoader();
	LoadModels();
	LoadPalettes();
	AnimationResourceManager::Instance()->Load( database0 );

	delete modelLoader;
	modelLoader = 0;

	// Load font:
	GLString fontPath = "./res/";
	FontSingleton* bridge = FontSingleton::Instance();
	{
		XMLDocument doc;
		doc.LoadFile("./res/font.xml");
		XMLElement* ele = doc.FirstChildElement("Font");
		GLASSERT(ele && ele->Attribute("name"));
		if (ele && ele->Attribute("name")) {
			fontPath += ele->Attribute("name");
		}
		bridge->Init(fontPath.c_str());

		int spacing = 0, height = 0, ascent = 0, descent = 0;
		ele->QueryAttribute("spacing", &spacing);
		ele->QueryAttribute("height", &height);
		ele->QueryAttribute("ascent", &ascent);
		ele->QueryAttribute("descent", &descent);
		
		bridge->SetAscentDelta(ascent);
		bridge->SetDescentDelta(descent);
		bridge->SetHeightDelta(height);
		bridge->SetLineSpacingDelta(spacing);
	}
	
	Texture* textTexture = TextureManager::Instance()->GetTexture( "fixedfont" );
	GLASSERT( textTexture );
	UFOText::Create(textTexture);

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
	delete FontSingleton::Instance();
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
	texman->CreateTexture( "white", 2, 2, TEX_RGB16, Texture::PARAM_NONE, this );
}


void Game::CreateTexture( Texture* t )
{
	if ( StrEqual( t->Name(), "white" ) ) {
		U16 pixels[4] = { 0xffff, 0xffff, 0xffff, 0xffff };
		GLASSERT( t->Format() == TEX_RGB16 );
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
			Scene* scene = sceneStack.Top()->scene;
			GLASSERT(scene);

			scene->Activate();
			scene->ResizeGamui(screenport.PhysicalWidth(), screenport.PhysicalHeight());
			scene->Resize();
			scene->SceneResult( id, result, data );
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
		node->scene->ResizeGamui(screenport.PhysicalWidth(), screenport.PhysicalHeight());
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


void Game::LoadPalettes()
{
	const gamedb::Item* parent = database0->Root()->Child( "data" )->Child( "palettes" );
	for( int i=0; i<parent->NumChildren(); ++i ) {
		const gamedb::Item* child = parent->ChildAt( i );

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
}


void Game::DoTick( U32 _currentTime )
{
	GPUDevice* device = GPUDevice::Instance();
	screenport.Resize(0, 0, device);
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
		device->Clear( cc.x, cc.y, cc.z, cc.w );

		scene->DoTick( deltaTime );

		{
			//GRINLIZ_PERFTRACK_NAME( "Game::DoTick 3D" );
			PROFILE_BLOCK( DoTick3D );

			screenport.SetPerspective(GPUDevice::Instance());
			scene->Draw3D( deltaTime );
		}

		{
			//GRINLIZ_PERFTRACK_NAME( "Game::DoTick UI" );
			PROFILE_BLOCK( DoTickUI );
			GLASSERT( scene );

			// UI Pass
			screenport.SetUI(GPUDevice::Instance()); 

			if ( renderUI ) {
				screenport.SetUI(GPUDevice::Instance());
				scene->RenderGamui2D();
			}
		}
	}

	int Y = 0;
	int space = 16;
	#ifndef GRINLIZ_DEBUG_MEM
//	const int memNewCount = 0;
	#endif
#if 1
	UFOText* ufoText = UFOText::Instance();
	if (SettingsManager::Instance()->DebugFPS()) {
		ufoText->Draw(0, Y, "%s %5.1ffps %4.1fK/f %3ddc/f fver=%d",
					  VERSION,
					  framesPerSecond,
					  (float)device->TrianglesDrawn() / 1000.0f,
					  device->DrawCalls(),
					  CURRENT_FILE_VERSION);
	}
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
	PROFILE_UPDATE();

	if ( perfText ) {
		PrintPerf();
	}
#endif
	screenport.SetUI(GPUDevice::Instance());
	UFOText::Instance()->FinalDraw();

	device->ResetTriCount();
	previousTime = currentTime;

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
	bool handled = sceneStack.Top()->scene->Tap(action, view, world);

//	if (action == GAME_TAP_DOWN) {
//		alsoPan = !handled && Game::TabletMode();
//	}
//	if (alsoPan) {
//		Pan(GAME_PAN_START + (action - GAME_TAP_DOWN), float(wx), float(wy));
//	}
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


void Game::MoveCamera(float dx, float dy)
{
	sceneStack.Top()->scene->MoveCamera(dx, dy);
}


void Game::FPSMove(float forward, float right, float rotate)
{
	sceneStack.Top()->scene->FPSMove(forward, right, rotate);
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
	else if (key == GAME_HK_TOGGLE_AI_DEBUG) {
		aiDebugLog = !aiDebugLog;
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
	screenport.Resize( width, height, GPUDevice::Instance() );
	sceneStack.Top()->scene->ResizeGamui(width, height);
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


