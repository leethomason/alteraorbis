#include "mapscene.h"
#include "../game/lumosgame.h"
#include "../game/worldmap.h"
#include "../xegame/spatialcomponent.h"
#include "../script/procedural.h"

using namespace gamui;
using namespace grinliz;

const int MAP2_RAD = 2;

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

	playerMark.SetSize( dx*0.02f, dy*0.02f );
	Vector2F pos = player->GetSpatialComponent()->GetPosition2D();
	playerMark.SetPos( x + dx*pos.x/float(worldMap->Width()), y + dy*pos.y/float(worldMap->Height()) );
}


void MapScene::ItemTapped( const gamui::UIItem* item )
{
	if ( item == &okay ) {
		lumosGame->PopScene();
	}
}



