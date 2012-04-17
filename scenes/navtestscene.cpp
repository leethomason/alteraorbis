#include "navtestscene.h"
#include "../game/lumosgame.h"
#include "../engine/engine.h"
#include "../game/worldmap.h"
#include "../grinliz/glgeometry.h"
#include "../engine/ufoutil.h"

using namespace grinliz;
using namespace gamui;

// Draw calls as a proxy for world subdivision:
// 20+20 blocks:
// quad division: 249 -> 407 -> 535
// block growing:  79 -> 135 -> 173
//  32x32          41 ->  97 -> 137
// Wow. Sticking with the growing block algorithm.
// Need to tune size on real map. (Note that the 32vs16 is approaching the same value.)
// 
NavTestScene::NavTestScene( LumosGame* game ) : Scene( game )
{
	game->InitStd( &gamui2D, &okay, 0 );

	LayoutCalculator layout = game->DefaultLayout();
	block.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	block.SetSize( layout.Width(), layout.Height() );
	block.SetText( "block" );

	block20.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	block20.SetSize( layout.Width(), layout.Height() );
	block20.SetText( "block20" );

	showAdjacent.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	showAdjacent.SetSize( layout.Width(), layout.Height() );
	showAdjacent.SetText( "Adj" );

	showZonePath.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	showZonePath.SetSize( layout.Width(), layout.Height() );
	showZonePath.SetText( "Region" );
	showZonePath.SetText2( "Path" );

	showAdjacent.AddToToggleGroup( &showZonePath );

	textLabel.Init( &gamui2D );

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	map = new WorldMap( 32, 32 );
	map->InitCircle();

	engine->SetMap( map );
	engine->CameraLookAt( 10, 10, 40 );
	tapMark.Zero();
}


NavTestScene::~NavTestScene()
{
	engine->SetMap( 0 );
	delete map;
	delete engine;
}


void NavTestScene::Resize()
{
	LumosGame* lumosGame = static_cast<LumosGame*>( game );
	lumosGame->PositionStd( &okay, 0 );
	LayoutCalculator layout = lumosGame->DefaultLayout();
	layout.PosAbs( &block, 1, -1 );
	layout.PosAbs( &block20, 2, -1 );

	layout.PosAbs( &showAdjacent, 0, -2 );
	layout.PosAbs( &showZonePath, 1, -2 );

	layout.PosAbs( &textLabel, 0, -3 );
}


void NavTestScene::Zoom( int style, float delta )
{
	if ( style == GAME_ZOOM_PINCH )
		engine->SetZoom( engine->GetZoom() *( 1.0f+delta) );
	else
		engine->SetZoom( engine->GetZoom() + delta );
}


void NavTestScene::Rotate( float degrees ) 
{
	engine->camera.Orbit( degrees );
}


void NavTestScene::Tap( int action, const grinliz::Vector2F& view, const grinliz::Ray& world )				
{
	bool uiHasTap = ProcessTap( action, view, world );
	if ( !uiHasTap ) {
		int tap = Process3DTap( action, view, world, engine );
		if ( tap ) {
			Vector3F oldMark = tapMark;

			Matrix4 mvpi;
			Ray ray;
			game->GetScreenport().ViewProjectionInverse3D( &mvpi );
			engine->RayFromViewToYPlane( view, mvpi, &ray, &tapMark );
			tapMark.y += 0.1f;

			char buf[40];
			SNPrintf( buf, 40, "xz = %.1f,%.1f nSubZ=%d", tapMark.x, tapMark.z, map->NumSubZones() );
			textLabel.SetText( buf );

			if ( showAdjacent.Down() ) {
				map->ShowAdjacentRegions( tapMark.x, tapMark.z );
			}
			else if ( showZonePath.Down() ) {
				map->ShowRegionPath( oldMark.x, oldMark.z, tapMark.x, tapMark.z );
				Vector2F start = { oldMark.x, oldMark.z };
				Vector2F end   = { tapMark.x, tapMark.z };
				::CDynArray< Vector2F > path;
				map->CalcPath( start, end, &path, true );
			}
		}
	}
}


void NavTestScene::ItemTapped( const gamui::UIItem* item )
{
	int makeBlocks = 0;

	if ( item == &okay ) {
		game->PopScene();
	}
	else if ( item == &block ) {
		makeBlocks = 1;
	}
	else if ( item == &block20 ) {
		makeBlocks = 20;
	}

	while ( makeBlocks ) {
		Rectangle2I b = map->Bounds();
		int x = random.Rand( b.Width() );
		int y = random.Rand( b.Height() );
		if ( map->IsLand( x, y ) && !map->IsBlockSet( x, y ) ) {
			map->SetBlock( x, y );
			--makeBlocks;
		}
	}
}


void NavTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );

	FlatShader flat;
	flat.SetColor( 1, 0, 0 );
	Vector3F delta = { 0.2f, 0, 0.2f };
	flat.DrawQuad( tapMark-delta, tapMark+delta, false );
}
