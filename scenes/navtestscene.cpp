#include "navtestscene.h"

#include "../grinliz/glgeometry.h"

#include "../engine/engine.h"
#include "../engine/ufoutil.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"

#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../game/pathmovecomponent.h"
#include "../game/debugpathcomponent.h"


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

	showOverlay.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	showOverlay.SetSize( layout.Width(), layout.Height() );
	showOverlay.SetText( "Over" );

	toggleBlock.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	toggleBlock.SetSize( layout.Width(), layout.Height() );
	toggleBlock.SetText( "TogBlock" );

	showAdjacent.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	showAdjacent.SetSize( layout.Width(), layout.Height() );
	showAdjacent.SetText( "Adj" );

	showZonePath.Init( &gamui2D, game->GetButtonLook( LumosGame::BUTTON_LOOK_STD ));
	showZonePath.SetSize( layout.Width(), layout.Height() );
	showZonePath.SetText( "Region" );
	showZonePath.SetText2( "Path" );

	textLabel.Init( &gamui2D );

	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase() );
	
	map = new WorldMap( 32, 32 );
	map->InitCircle();

	engine->SetMap( map );
	engine->CameraLookAt( 10, 10, 40 );
	tapMark.Zero();

	chit = chitBag.CreateTestChit( engine, "humanFemale" );
	PathMoveComponent* pmc = new PathMoveComponent( map );
	chit->Add( pmc );
	chit->Add( new DebugPathComponent( engine, map, game ) );
	chit->GetSpatialComponent()->SetPosition( 10.0f, 0.0f, 10.0f );
	chit->AddListener( this );
}


NavTestScene::~NavTestScene()
{
	chitBag.DeleteAll();
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

	layout.PosAbs( &showOverlay,   0, -2 );
	layout.PosAbs( &showAdjacent, 1, -2 );
	layout.PosAbs( &showZonePath, 2, -2 );
	layout.PosAbs( &toggleBlock, 3, -2 );

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
			SNPrintf( buf, 40, "xz = %.1f,%.1f nSubZ=%d", tapMark.x, tapMark.z, map->NumRegions() );
			textLabel.SetText( buf );

			Vector2F d = { tapMark.x, tapMark.z };
			PathMoveComponent* pmc = static_cast<PathMoveComponent*>( chit->GetComponent( "PathMoveComponent" ) );
			GLASSERT( pmc );
			pmc->SetDest( d );

			if ( showAdjacent.Down() ) {
				map->ShowAdjacentRegions( tapMark.x, tapMark.z );
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
	else if ( item == &showZonePath ) {
		PathMoveComponent* pmc = static_cast<PathMoveComponent*>( chit->GetComponent( "PathMoveComponent" ) );
		if ( pmc ) 
			pmc->SetPathDebugging( showZonePath.Down() );
	}
	else if ( item == &showOverlay ) {
		map->ShowRegionOverlay( showOverlay.Down() );
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


void NavTestScene::DoTick( U32 deltaTime )
{
	chitBag.DoTick( deltaTime );
}


void NavTestScene::OnChitMsg( Chit* chit, const char* componentName, int id )
{
	GLOUTPUT(( "OnChitMsg %s %d\n", componentName, id ));
}


void NavTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );

	FlatShader flat;
	flat.SetColor( 1, 0, 0 );
	Vector3F delta = { 0.2f, 0, 0.2f };
	flat.DrawQuad( tapMark-delta, tapMark+delta, false );
}
