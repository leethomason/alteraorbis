#include "../shared/lodepng.h"

#include "livepreviewscene.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../xegame/testmap.h"

#include "../game/lumosgame.h"

#include <sys/stat.h>

using namespace gamui;
using namespace grinliz;

static const float CENTER_X = 4.0f;
static const float CENTER_Z = 4.0f;
static const int SIZE = 256;

LivePreviewScene::LivePreviewScene( LumosGame* game ) : Scene( game )
{
	TestMap* map = 0;
	
	map = new TestMap( 8, 8 );
	Color3F c = { 0.5f, 0.5f, 0.5f };
	map->SetColor( c );
	

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );
	engine->SetGlow( true );
	engine->lighting.direction.Set( 0, 1, 0 );


	static const Color4F color[4] = {
		{ 1.0f, 0.6f, 0.4f, 1.0f },	// (r) skin
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// (b0) highlight
		{ 0.0f, 0.6f, 1.0f, 1.0f },	// (b1) glasses / tattoo
		{ 0.5f, 0.2f, 0.0f, 1.0f }	// (g) hair
	};
	static const float tex[4] = { 0, 0, 0, 0 };

	const ModelResource* modelResource = ModelResourceManager::Instance()->GetModelResource( "unitPlateProcedural" );
	for( int i=0; i<NUM_MODEL; ++i ) {
		model[i] = engine->AllocModel( modelResource );
		model[i]->SetPos( CENTER_X + float(i), 0.1f, CENTER_Z );
		model[i]->SetProcedural( true, color, tex );
	}
	model[1]->SetScale( 0.5f );

	engine->camera.SetPosWC( CENTER_X, 8, CENTER_Z );
	static const Vector3F out = { 0, 0, 1 };
	engine->camera.SetDir( V3F_DOWN, out );		// look straight down. This works! cool.
	engine->camera.Orbit( 180.0f );

	CreateTexture();

	game->InitStd( &gamui2D, &okay, 0 );
	fileTimer = 0;
	fileTime = 0;
}


LivePreviewScene::~LivePreviewScene()
{
	Map* map = engine->GetMap();
	for( int i=0; i<NUM_MODEL; ++i ) 
		engine->FreeModel( model[i] );
	delete engine;
	delete map;
}


void LivePreviewScene::Resize()
{
	//const Screenport& port = game->GetScreenport();
	static_cast<LumosGame*>(game)->PositionStd( &okay, 0 );

	//LayoutCalculator layout = lumosGame->DefaultLayout();
}


void LivePreviewScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
}


void LivePreviewScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );
}


void LivePreviewScene::DoTick( U32 deltaTime )
{
	fileTimer += deltaTime;
	if ( fileTimer > 1000 ) {
		fileTimer = 0;
		CreateTexture();
	}
}


void LivePreviewScene::CreateTexture()
{
	static const char* filename = "./res/face.png";
	struct _stat buf;

	_stat( filename, &buf );
	if ( fileTime == buf.st_mtime ) {
		return;
	}
	fileTime = buf.st_mtime;

	TextureManager* texman = TextureManager::Instance();
	Texture* t = texman->GetTexture( "procedural" );
	GLASSERT( t );
	GLASSERT( t->Alpha() );

	unsigned w=0, h=0;
	U8* pixels = 0;
	int error = lodepng_decode32_file( &pixels, &w, &h, filename );
	GLASSERT( error == 0 );
	GLASSERT( w == SIZE*4 );
	GLASSERT( h == SIZE );
	static const int BUFFER_SIZE = SIZE*SIZE*4;
	U16 buffer[BUFFER_SIZE];

	if ( error == 0 ) {
		int scanline = w*4;

		static const int RAD=8;
		for( int j=0; j<SIZE; ++j ) {
			for( int i=0; i<int(w); ++i ) {
				const U8* p = pixels + scanline*j + i*4;
				Color4U8 color = { p[0], p[1], p[2], p[3] };

				if ( color.a == 0 ) {
					color.Set( 0, 0, 0, 0 );
				}				
				/*
					int zone = i / SIZE;
					if ( zone == 0 )      color.Set( 255, 0, 0, 0 );
					else if ( zone == 1 ) color.Set( 255, 0, 0, 0 );
					else if ( zone == 2 ) color.Set( 0, 0, 255, 0 );
					else                  color.Set( 0, 255, 0, 0 );
				}*/
				U16 c = Surface::CalcRGBA16( color );
				buffer[SIZE*4*(SIZE-1-j)+i] = c;
			}
		}
		t->Upload( buffer, BUFFER_SIZE*sizeof(buffer[0]) );

		/*
		static bool result = false;
		if ( !result ) {
			for( unsigned j=0; j<h; ++j ) {
				for( unsigned i=0; i<w; ++i ) {
					U16 c = buffer[SIZE*4*(SIZE-1-j)+i];
					Color4U8 color = Surface::CalcRGBA16( c );
					U8* p = pixels + scanline*j + i*4;
					p[0] = color.r;
					p[1] = color.g;
					p[2] = color.b;
					p[3] = 255;
				}
			}
			lodepng_encode32_file( "./res/facetest.png", pixels, w, h );
			result = true;
		}
		*/
		free( pixels );
	}
	else {
		GLASSERT( 0 );
	}	
}
