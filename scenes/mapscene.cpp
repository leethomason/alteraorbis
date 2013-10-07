#include "mapscene.h"

#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../game/lumoschitbag.h"
#include "../game/team.h"
#include "../game/gameitem.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/itemcomponent.h"

#include "../script/procedural.h"
#include "../script/corescript.h"

using namespace gamui;
using namespace grinliz;

MapScene::MapScene( LumosGame* game, MapSceneData* data ) : Scene( game ), lumosGame(game)
{
	game->InitStd( &gamui2D, &okay, 0 );

	lumosChitBag = data->lumosChitBag;
	worldMap = data->worldMap;
	player = data->player;

	Texture* mapTexture = TextureManager::Instance()->GetTexture( "miniMap" );
	RenderAtom mapAtom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)mapTexture, 0, 1, 1, 0 );
	mapImage.Init( &gamui2D, mapAtom, false );
	
	Vector2I pos2i = player->GetSpatialComponent()->GetPosition2DI();
	Vector2I sector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
	if ( sector.x < MAP2_RAD )					sector.x = MAP2_RAD;
	if ( sector.y < MAP2_RAD )					sector.y = MAP2_RAD;
	if ( sector.x >= NUM_SECTORS - MAP2_RAD )	sector.x = NUM_SECTORS - MAP2_RAD - 1;
	if ( sector.y >= NUM_SECTORS - MAP2_RAD )	sector.y = NUM_SECTORS - MAP2_RAD - 1;

	sectorBounds.Set( sector.x-MAP2_RAD, sector.y-MAP2_RAD, sector.x+MAP2_RAD, sector.y+MAP2_RAD );

	map2Bounds.Set( float(sector.x-MAP2_RAD)*float(SECTOR_SIZE), 
					float(sector.y-MAP2_RAD)*float(SECTOR_SIZE), 
					float(sector.x+1+MAP2_RAD)*float(SECTOR_SIZE), 
					float(sector.y+1+MAP2_RAD)*float(SECTOR_SIZE) );

	mapAtom.tx0 = map2Bounds.min.x / float(worldMap->Width());
	mapAtom.ty0 = map2Bounds.max.y / float(worldMap->Height());
	mapAtom.tx1 = map2Bounds.max.x / float(worldMap->Width());
	mapAtom.ty1 = map2Bounds.min.y / float(worldMap->Height());

	map2Image.Init( &gamui2D, mapAtom, false );

	RenderAtom atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark2.Init( &gamui2D, atom, true );

	for( int i=0; i<MAP2_SIZE2; ++i ) {
		map2Text[i].Init( &gamui2D );
	}
}


MapScene::~MapScene()
{
}


void MapScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );

	const Screenport& port = lumosGame->GetScreenport();
	LayoutCalculator layout = lumosGame->DefaultLayout();

	float y  = layout.GutterY();
	float dy = okay.Y() - layout.GutterY() - y;
	float x = layout.GutterX();
	float dx = port.UIWidth()*0.5f - 1.5f*layout.GutterX();
	dx = dy = Min( dx, dy );

	mapImage.SetSize( dx, dy );
	map2Image.SetSize( dx, dy );

	mapImage.SetPos( x, y );
	map2Image.SetPos( port.UIWidth()*0.5f + 0.5f*layout.GutterX(), y );

	// --- MAIN ---

	const float MARK_SIZE = dx*0.02f;
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );
	Vector2F pos = player->GetSpatialComponent()->GetPosition2D();
	playerMark.SetCenterPos( x + dx*pos.x/float(worldMap->Width()), 
							 y + dy*pos.y/float(worldMap->Height()));

	// --- AREA --- 

	x = map2Image.X();
	playerMark2.SetSize( MARK_SIZE, MARK_SIZE );
	playerMark2.SetCenterPos( x + dx*(pos.x-map2Bounds.min.x)/map2Bounds.Width(), 
							  y + dy*(pos.y-map2Bounds.min.y)/map2Bounds.Height());

	float gridSize = map2Image.Width() / float(MAP2_SIZE);
	float gutter = gamui2D.GetTextHeight() * 0.25f;

	for( int j=0; j<MAP2_SIZE; ++j ) {
		for( int i=0; i<MAP2_SIZE; ++i ) {
			map2Text[j*MAP2_SIZE+i].SetPos( x + float(i)*gridSize + gutter, y + float(j)*gridSize + gutter );
			map2Text[j*MAP2_SIZE+i].SetSize( gridSize - gutter*2.0f, gridSize - gutter*2.0f );
		}
	}

	SetText();
}


void MapScene::SetText()
{
	CDynArray<Chit*> query;

	for( int j=0; j<MAP2_SIZE; ++j ) {
		for( int i=0; i<MAP2_SIZE; ++i ) {

			Vector2I sector = { sectorBounds.min.x + i, sectorBounds.min.y + j };
			const SectorData& sd = worldMap->GetSector( sector );

			MoBFilter mobFilter;
			Rectangle2I innerI = sd.InnerBounds();
			Rectangle2F inner;
			inner.Set( float(innerI.min.x), float(innerI.min.y), float(innerI.max.x+1), float(innerI.max.y+1) );

			lumosChitBag->QuerySpatialHash( &query, inner, 0, &mobFilter );

			int low=0, med=0, high=0, greater=0;
			float playerPower = player->GetItemComponent()->PowerRating();

			for( int k=0; k<query.Size(); ++k ) {
				const GameItem* item = query[k]->GetItem();
				if ( GetRelationship( player->GetItem()->primaryTeam, item->primaryTeam ) == RELATE_ENEMY ) {

					float power = query[k]->GetItemComponent()->PowerRating();

					if ( item->GetValue( "mob" ) == "greater" )
						++greater;
					else if ( power < playerPower * 0.5f ) 
						++low;
					else if ( power > playerPower * 2.0f )
						++high;
					else
						++med;
				}
			}

			CStr<64> str;
			if ( sd.HasCore() ) {
				const char* owner = "<none>";
				CoreScript* cc = lumosChitBag->GetCore( sector );
				if ( cc ) {
					Chit* chitOwner = cc->GetAttached(0);
					if ( chitOwner ) {
						owner = TeamName( chitOwner->GetItem()->primaryTeam ).c_str();
					}
				}
				str.Format( "%s\n%s\nL%d M%d H%d G%d", sd.name, owner, low, med, high, greater );
			}
			map2Text[j*MAP2_SIZE+i].SetText( str.c_str() );
		}
	}
}


void MapScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
}


void MapScene::HandleHotKey( int value )
{
	if ( value == GAME_HK_MAP ) {
		lumosGame->PopScene();
	}
}



