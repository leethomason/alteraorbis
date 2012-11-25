#pragma warning (disable:4530)
#include "../shared/lodepng.h"

#include "livepreviewscene.h"

#include "../engine/engine.h"
#include "../engine/model.h"
#include "../xegame/testmap.h"

#include "../game/lumosgame.h"
#include "../script/procedural.h"

#include <sys/stat.h>

using namespace gamui;
using namespace grinliz;

static const float CENTER_X = 4.0f;
static const float CENTER_Z = 4.0f;
static const float START_X = 2.0f;
static const float START_Z = 2.5f;
static const int SIZE = 256;
static const int ROWS = 4;
static const int COLS = 4;

LivePreviewScene::LivePreviewScene( LumosGame* game ) : Scene( game )
{
	TestMap* map = 0;
	live = false;
	
	//map = new TestMap( 8, 8 );
	//Color3F c = { 0.5f, 0.5f, 0.5f };
	//map->SetColor( c );
	

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );
	engine->SetGlow( true );
	engine->lighting.direction.Set( 0, 1, 0 );

	LayoutCalculator layout = game->DefaultLayout();
	const ButtonLook& look = game->GetButtonLook(0);
	const float width  = layout.Width();
	const float height = layout.Height();

	for( int i=0; i<ROWS; ++i ) {
		rowButton[i].Init( &gamui2D, look );
		rowButton[i].SetSize( width, height );
		CStr< 16 > t;
		t.Format( "%d", i+1 );
		rowButton[i].SetText( t.c_str() );
		rowButton[0].AddToToggleGroup( &rowButton[i] );
	}

	static const char* typeName[NUM_TYPES] = { "Face", "Ring" };
	for( int i=0; i<NUM_TYPES; ++i ) {
		typeButton[i].Init( &gamui2D, look );
		typeButton[i].SetSize( width, height );
		typeButton[i].SetText( typeName[i] );
		typeButton[0].AddToToggleGroup( &typeButton[i] );
	}

	memset( model, 0, sizeof(Model*) * NUM_MODEL );
	GenerateFaces( 0 );

	engine->camera.SetPosWC( CENTER_X, 11, CENTER_Z );
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

	LayoutCalculator layout = static_cast<LumosGame*>(game)->DefaultLayout();
	for( int i=0; i<ROWS; ++i ) {
		layout.PosAbs( &rowButton[i], 0, i );
	}
	for( int i=0; i<NUM_TYPES; ++i ) {
		layout.PosAbs( &typeButton[i], -1, i );
	}
}



void LivePreviewScene::GenerateFaces( int mainRow )
{
	FaceGen faceGen( game->GetPalette() );

	float tex[4] = { 0, 0, 0, 0 };
	Random random( mainRow );
	random.Rand();

	const ModelResource* modelResource = ModelResourceManager::Instance()->GetModelResource( "unitPlateProcedural" );
	for( int i=0; i<NUM_MODEL; ++i ) {
		if ( model[i] ) {
			engine->FreeModel( model[i] );
			model[i] = 0;
		}

		Color4F skin, highlight, hair, glasses;
		faceGen.GetSkinColor( i, random.Rand(), random.Uniform(), &skin, &highlight ); 
		faceGen.GetHairColor( i, &hair );
		faceGen.GetGlassesColor( i, random.Rand(), random.Uniform(), &glasses );

		model[i] = engine->AllocModel( modelResource );
		int row = i / COLS;
		int col = i - row*COLS;
		float x = START_X + float(col);
		float z = START_Z + float(row);
		float current = 1.0f - 0.25f * (float)(mainRow);

		switch ( row ) {
		case 0:
			model[i]->SetScale( Lerp( 1.0f, 0.25f, (float)(col)/(float)COLS ));
			faceGen.GetSkinColor( col, col, 0, &skin, &highlight ); 
			faceGen.GetHairColor( col, &hair );
			tex[0] = tex[1] = tex[2] = tex[3] = current;
			break;

		default:
			tex[0] = 0.25f * (float)random.Rand(4);
			tex[1] = 0.25f * (float)random.Rand(4);
			tex[2] = 0.25f * (float)random.Rand(4);
			tex[3] = 0.25f * (float)random.Rand(4);
			tex[col] = current;
			break;
		}

		const Color4F color[4] = {
			skin,						// (r) skin
			highlight,					// (b0) highlight
			glasses,					// (b1) glasses / tattoo
			hair						// (g) hair
		};

		model[i]->SetPos( x, 0.1f, z );
		model[i]->SetProcedural( true, color, tex );
	}
}


void LivePreviewScene::GenerateRing( int mainRow )
{

}


void LivePreviewScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	for( int i=0; i<ROWS; ++i ) {
		if ( item == &rowButton[i] ) {
			GenerateFaces( i );
			break;
		}
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
	static const char* filename = "./res/humanMaleFace.png";
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
	GLASSERT( t->Width() == SIZE*COLS );
	GLASSERT( t->Height() == SIZE*ROWS );

	unsigned w=0, h=0;
	U8* pixels = 0;
	int error = lodepng_decode32_file( &pixels, &w, &h, filename );
	GLASSERT( error == 0 );
	GLASSERT( w == SIZE*COLS );
	GLASSERT( h == SIZE*ROWS );
	static const int BUFFER_SIZE = SIZE*SIZE*COLS*ROWS;
	U16* buffer = new U16[BUFFER_SIZE];

	if ( error == 0 ) {
		int scanline = SIZE*COLS*4;

		static const int RAD=8;
		for( int j=0; j<SIZE*ROWS; ++j ) {
			for( int i=0; i<SIZE*COLS; ++i ) {
				const U8* p = pixels + scanline*j + i*4;
				Color4U8 color = { p[0], p[1], p[2], p[3] };

				if ( color.a == 0 ) {
					color.Set( 0, 0, 0, 0 );
				}				
				U16 c = Surface::CalcRGBA16( color );
				int offset = SIZE*COLS*(SIZE*ROWS-1-j)+i;
				GLASSERT( offset >= 0 && offset < BUFFER_SIZE );
				buffer[offset] = c;
			}
		}
		t->Upload( buffer, BUFFER_SIZE*sizeof(buffer[0]) );
		free( pixels );
	}
	else {
		GLASSERT( 0 );
	}	
	delete [] buffer;
}
