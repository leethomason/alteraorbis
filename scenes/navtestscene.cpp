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

#include "navtestscene.h"

#include "../grinliz/glgeometry.h"

#include "../engine/engine.h"
#include "../engine/ufoutil.h"
#include "../engine/text.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

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
	debugRay.direction.Zero();
	debugRay.origin.Zero();

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
	showOverlay.SetDown();

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

	map = new WorldMap( 32, 32 );
	map->InitCircle();
	engine = new Engine( game->GetScreenportMutable(), game->GetDatabase(), map );
	engine->SetGlow( false );

	map->ShowRegionOverlay( true );

	engine->CameraLookAt( 10, 10, 40 );
	tapMark.Zero();

	for( int i=0; i<NUM_CHITS; ++i ) {
		chit[i] = chitBag.NewChit();
		chit[i]->Add( new SpatialComponent() );
		chit[i]->Add( new RenderComponent( engine, "humanFemale" ) );
		chit[i]->Add( new PathMoveComponent( map ) );
		chit[i]->Add( new DebugPathComponent( engine, map, game ) );
		chit[i]->GetSpatialComponent()->SetPosition( 10.0f + (float)i*2.f, 0.0f, 10.0f );
	}
	chit[1]->AddListener( this );
	// Comment in to get a unit pacing.
	//GET_COMPONENT( chit[1], PathMoveComponent )->SetDest( 13.f, 10.f );

}


NavTestScene::~NavTestScene()
{
	chitBag.DeleteAll();
	delete engine;
	delete map;
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


void NavTestScene::MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world )
{
	debugRay = world;
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
			SNPrintf( buf, 40, "xz = %.1f,%.1f nSubZ=%d", tapMark.x, tapMark.z, -1 ); //map->NumRegions() );
			textLabel.SetText( buf );

			if ( toggleBlock.Down() ) {
				Vector2I d = { (int)tapMark.x, (int)tapMark.z };
				if ( map->IsBlockSet( d.x, d.y ) ) 
					map->ClearBlock( d.x, d.y );
				else
					map->SetBlock( d.x, d.y );
			}
			else {
				// Move to the marked location.
				Vector2F d = { tapMark.x, tapMark.z };
				PathMoveComponent* pmc = static_cast<PathMoveComponent*>( chit[0]->GetComponent( "PathMoveComponent" ) );
				GLASSERT( pmc );
				pmc->QueueDest( d );
			}

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
		PathMoveComponent* pmc = static_cast<PathMoveComponent*>( chit[0]->GetComponent( "PathMoveComponent" ) );
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
	chitBag.DoTick( deltaTime, 0 );
}


void NavTestScene::DrawDebugText()
{
	UFOText* ufoText = UFOText::Instance();
	if ( debugRay.direction.x ) {
		Model* root = engine->IntersectModel( debugRay.direction, debugRay.origin, FLT_MAX, TEST_TRI, 0, 0, 0, 0 );
		int y = 16;
		for ( ; root; root=root->next ) {
			Chit* chit = root->userData;
			if ( chit ) {
				GLString str;
				chit->DebugStr( &str );
				ufoText->Draw( 0, y, "%s", str.c_str() );
				y += 16;
			}
		}
	}
}


void NavTestScene::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	/*
//	GLOUTPUT(( "OnChitMsg %s %d\n", componentName, id ));
	const Vector2F& pos = chit->GetSpatialComponent()->GetPosition2D();
	static const Vector2F t0 = { 11.f, 10.f };
	static const Vector2F t1 = { 13.f, 10.f };
	if ( (pos-t0).LengthSquared() < (pos-t1).LengthSquared() ) {
		GET_COMPONENT( chit, PathMoveComponent )->QueueDest( t1 );
	}
	else {
		GET_COMPONENT( chit, PathMoveComponent )->QueueDest( t0 );
	}
	*/
}


void NavTestScene::Draw3D( U32 deltaTime )
{
	engine->Draw( deltaTime );

	FlatShader flat;
	flat.SetColor( 1, 0, 0 );
	Vector3F delta = { 0.2f, 0, 0.2f };
	flat.DrawQuad( tapMark-delta, tapMark+delta, false );
}
