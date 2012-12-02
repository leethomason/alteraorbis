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

LivePreviewScene::LivePreviewScene( LumosGame* game, const LivePreviewSceneData* data ) : Scene( game )
{
	TestMap* map = 0;
	live = data->live;
	timer = 0;
	
	//map = new TestMap( 8, 8 );
	//Color3F c = { 0.5f, 0.5f, 0.5f };
	//map->SetColor( c );
	

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );
	engine->SetGlow( true );
	engine->LoadConfigFiles( "./res/particles.xml", "./res/lighting.xml" );

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

	memset( model, 0, sizeof(model[0])*NUM_MODEL );

	currentType = FACE;
	GenerateFaces( 0 );
	if ( live ) {
		CreateTexture( FACE );
	}
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


grinliz::Color4F LivePreviewScene::ClearColor()
{
	//Color4F cc = game->GetPalette(0)->Get4F( PAL_ZERO, PAL_GRAY );
	Color4F cc = { 0.2f, 0.2f, 0.2f, 0.0f };
	//Color4F cc = { 0,0,0,0 };
	return cc;
}


void LivePreviewScene::GenerateFaces( int mainRow )
{
	static const float CENTER_X = 4.0f;
	static const float CENTER_Z = 4.0f;
	static const float START_X = 2.0f;
	static const float START_Z = 2.5f;

	engine->lighting.direction.Set( 0, 1, 0 );
	engine->camera.SetPosWC( CENTER_X, 11, CENTER_Z );
	static const Vector3F out = { 0, 0, 1 };
	engine->camera.SetDir( V3F_DOWN, out );		// look straight down. This works! cool.
	engine->camera.Orbit( 180.0f );

	FaceGen faceGen( game->GetPalette() );

	float tex[4] = { 0, 0, 0, 0 };
	Random random( mainRow );
	random.Rand();

	const ModelResource* modelResource = 0;
	if ( live ) 
		modelResource = ModelResourceManager::Instance()->GetModelResource( "unitPlateProcedural" );
	else
		modelResource = ModelResourceManager::Instance()->GetModelResource( "unitPlateHumanMaleFace" );

	int srcRows = modelResource->atom[0].texture->Height() / modelResource->atom[0].texture->Width() * 4;
	float rowMult = 1.0f / (float)srcRows;

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
		float current = 1.0f - rowMult * (float)(mainRow);

		switch ( row ) {
		case 0:
			if ( live ) {
				model[i]->SetScale( Lerp( 1.0f, 0.25f, (float)(col)/(float)COLS ));
			}
			faceGen.GetSkinColor( col, col, 0, &skin, &highlight ); 
			faceGen.GetHairColor( col, &hair );
			tex[0] = tex[1] = tex[2] = tex[3] = current;
			break;

		default:
			tex[0] = rowMult * (float)random.Rand(srcRows);
			tex[1] = rowMult * (float)random.Rand(srcRows);
			tex[2] = rowMult * (float)random.Rand(srcRows);
			if ( !live ) {
				// glasses handled special:
				if ( random.Bit() )
					tex[2] = rowMult * (float)(srcRows);
				else
					tex[2] = rowMult * (float)( srcRows - random.Rand( FaceGen::GLASSES_ROWS ));
			}
			tex[3] = rowMult * (float)random.Rand(srcRows);
			if ( live ) {
				tex[col] = current;
			}
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
	static const float DELTA  = 0.3f;
	static const float ORIGIN = DELTA*1.5f;
	static const float DIST   = 3.0f;

	engine->lighting.direction.Set( -1, 1, -1 );
	engine->lighting.direction.Normalize();

	engine->camera.SetPosWC( 0, ORIGIN, ORIGIN );
	static const Vector3F out = { -1, 0, 0 };			// FIXME: weird camera issue illustrated, again.
	engine->camera.SetDir( out, V3F_UP );

	float tex[4] = { 0, 0, 0, 0 };
	Random random( mainRow );
	random.Rand();

	const ModelResource* modelResource = 0;
	modelResource = ModelResourceManager::Instance()->GetModelResource( live ? "ringProc" : "ring" );

	int srcRows = modelResource->atom[0].texture->Height() / modelResource->atom[0].texture->Width() * 4;
	float rowMult = 1.0f / (float)srcRows;

	for( int i=0; i<NUM_MODEL; ++i ) {
		if ( model[i] ) {
			engine->FreeModel( model[i] );
			model[i] = 0;
		}

		model[i] = engine->AllocModel( modelResource );
		int row = i / COLS;
		int col = i - row*COLS;
		float x = float(col) * DELTA;
		float y = float(ROWS-1-row) * DELTA;
		float current = 1.0f - rowMult * (float)(mainRow);

		switch ( row ) {
		case 0:
			tex[0] = tex[1] = tex[2] = tex[3] = current;
			break;
		
		default:
			tex[0] = rowMult * (float)random.Rand(srcRows);
			tex[1] = rowMult * (float)random.Rand(srcRows);
			tex[2] = rowMult * (float)random.Rand(srcRows);
			tex[3] = rowMult * (float)random.Rand(srcRows);
			if ( live ) {
				tex[col] = current;
			}
			break;
		}

		// NOT in order.
		static const char* parts[4] = {
			"main",
			"guard",
			"triad",
			"blade"
		};
		int ids[4] = { -1,-1,-1,-1 };

		ids[0] = model[i]->GetBoneID( StringPool::Intern( "main", true ));
		GLASSERT( ids[0] >= 0 && ids[0] < 4 );
		
		for( int k=1; k<4; ++k ) {
			int id = model[i]->GetBoneID( StringPool::Intern( parts[k], true ));
			GLASSERT( id >= 0 && id < 4 );
			if ( (i==0) || (i & (1<<k))) {
				ids[k] = id;
			}
		}

		WeaponGen weaponGen( game->GetPalette() );
		Color4F color[4] = {	// Test color. Comment out assignment below.
			{ 1, 0, 0, 0 },		// base:		red
			{ 1, 1, 0, 0 },		// contrast:	yellow
			{ 0, 1, 0, 1 },		// effect:		blue
			{ 1, 0.5f, 0, 1 }	// glow:		orange
		};
		// r,    b0,       b1,     g
		// base, contrast, effect, glow
		weaponGen.GetColors( i+mainRow*NUM_MODEL, col==2, col==3, color );

		model[i]->SetPos( 3.0f, y, x );
		model[i]->SetProcedural( true, color, tex );
		model[i]->SetBoneFilter( ids );
	}
}


void LivePreviewScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &typeButton[FACE] ) {
		currentType = FACE;
		CreateTexture( FACE );
		GenerateFaces( 0 );
		rowButton[0].SetDown();
	}
	else if ( item == &typeButton[RING] ) {
		currentType = RING;
		CreateTexture( RING );
		GenerateRing( 0 );
		rowButton[0].SetDown();
	}

	for( int i=0; i<ROWS; ++i ) {
		if ( item == &rowButton[i] ) {
			switch ( currentType ) {
			case FACE:	GenerateFaces( i ); break;
			case RING:	GenerateRing( i );	break;
			default: GLASSERT( 0 );			break;
			}
		}
	}
}


void LivePreviewScene::Draw3D( U32 deltaTime )
{
	timer += deltaTime;
	if ( currentType != FACE ) {
		model[NUM_MODEL-1]->SetYRotation( (float)((timer/20)%360) );
	}
	engine->Draw( deltaTime );
}


void LivePreviewScene::DoTick( U32 deltaTime )
{
	if ( live ) {
		fileTimer += deltaTime;
		if ( fileTimer > 1000 ) {
			fileTimer = 0;
			CreateTexture( currentType );
		}
	}
}


void LivePreviewScene::CreateTexture( int type )
{
	if ( !live ) 
		return;

	const char* filename = 0;
	switch( type ) {
		case FACE:	filename = "./res/humanMaleFace.png";	break;
		case RING:	filename = "./res/ring.png";			break;
		default:	GLASSERT( 0 );							break;
	}

	static const int SIZE = 256;
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


	if ( type == FACE ) 
		t->SetEmissive( false );	// alpha is transparency
	else
		t->SetEmissive( true );		// alpha is emissive
}
