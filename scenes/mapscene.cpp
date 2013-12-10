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
	worldMap     = data->worldMap;
	player       = data->player;
	this->data   = data;

	gridTravel.Init( &gamui2D, lumosGame->GetButtonLook(0) );

	Texture* mapTexture = TextureManager::Instance()->GetTexture( "miniMap" );
	RenderAtom mapAtom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)mapTexture, 0, 1, 1, 0 );
	mapImage.Init( &gamui2D, mapAtom, false );
	mapImage.SetCapturesTap( true );
	
	Vector2I pos2i = { worldMap->Width()/2, worldMap->Height()/2 };
	if ( player ) {
		pos2i = player->GetSpatialComponent()->GetPosition2DI();
	}
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

	mapImage2.Init( &gamui2D, mapAtom, false );
	mapImage2.SetCapturesTap( true );

	RenderAtom atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark.Init( &gamui2D, atom, true );
	playerMark2.Init( &gamui2D, atom, true );

	atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO_DISABLED;
	homeMark.Init( &gamui2D, atom, true );
	homeMark2.Init( &gamui2D, atom, true );

	RenderAtom travelAtom = lumosGame->CalcPaletteAtom( PAL_GRAY*2, PAL_ZERO );
	travelAtom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO_DISABLED;
	travelMark.Init( &gamui2D, travelAtom, true );
	travelMark2.Init( &gamui2D, travelAtom, true );

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
	
	layout.PosAbs( &gridTravel, 1, -1 );
	gridTravel.SetSize( gridTravel.Width()*2.f, gridTravel.Height() );	// double the button size

	float y  = layout.GutterY();
	float dy = okay.Y() - layout.GutterY() - y;
	float x = layout.GutterX();
	float dx = port.UIWidth()*0.5f - 1.5f*layout.GutterX();
	dx = dy = Min( dx, dy );

	mapImage.SetSize( dx, dy );
	mapImage2.SetSize( dx, dy );

	mapImage.SetPos( x, y );
	mapImage2.SetPos( port.UIWidth()*0.5f + 0.5f*layout.GutterX(), y );

	Vector2I homeSector = lumosChitBag->PlayerHomeSector();

	// --- MAIN ---

	const float MARK_SIZE = dx*0.02f;
	playerMark.SetSize( MARK_SIZE, MARK_SIZE );
	Vector2F pos = { 0, 0 };
	if ( player ) {
		pos = player->GetSpatialComponent()->GetPosition2D();
	}
	playerMark.SetCenterPos( x + dx*pos.x/float(worldMap->Width()), 
							 y + dy*pos.y/float(worldMap->Height()));

	homeMark.SetVisible( !homeSector.IsZero() );
	if ( !homeSector.IsZero() ) {
		homeMark.SetSize( dx/float(NUM_SECTORS), dy/float(NUM_SECTORS) );
		homeMark.SetPos( x + dx*(float)homeSector.x/float(NUM_SECTORS),
						 y + dy*(float)homeSector.y/float(NUM_SECTORS));
	}

	travelMark.SetVisible( !data->destSector.IsZero() );
	if ( !data->destSector.IsZero() ) {
		travelMark.SetSize( dx/float(NUM_SECTORS), dy/float(NUM_SECTORS) );
		travelMark.SetPos(	x + dx*(float)data->destSector.x/float(NUM_SECTORS),
							y + dy*(float)data->destSector.y/float(NUM_SECTORS));
	}

	// --- AREA --- 

	x = mapImage2.X();
	playerMark2.SetSize( MARK_SIZE, MARK_SIZE );
	playerMark2.SetCenterPos( x + dx*(pos.x-map2Bounds.min.x)/map2Bounds.Width(), 
							  y + dy*(pos.y-map2Bounds.min.y)/map2Bounds.Height());

	float gridSize = mapImage2.Width() / float(MAP2_SIZE);
	float gutter = gamui2D.GetTextHeight() * 0.25f;

	for( int j=0; j<MAP2_SIZE; ++j ) {
		for( int i=0; i<MAP2_SIZE; ++i ) {
			map2Text[j*MAP2_SIZE+i].SetPos( x + float(i)*gridSize + gutter, y + float(j)*gridSize + gutter );
			map2Text[j*MAP2_SIZE+i].SetBounds( gridSize - gutter*2.0f, 0 );
		}
	}

	homeMark2.SetVisible( false );
	if ( !homeSector.IsZero() && sectorBounds.Contains( homeSector) ) {
		homeMark2.SetVisible( true );
		homeMark2.SetSize( gridSize, gridSize );
		homeMark2.SetPos( x + dx*(float)(homeSector.x-sectorBounds.min.x)/float(MAP2_SIZE),
						  y + dy*(float)(homeSector.y-sectorBounds.min.y)/float(MAP2_SIZE));
	}

	travelMark2.SetVisible( false );
	if ( !data->destSector.IsZero() && sectorBounds.Contains( data->destSector ) ) {
		travelMark2.SetVisible( true );
		travelMark2.SetSize( gridSize, gridSize );
		travelMark2.SetPos( x + dx*(float)(data->destSector.x-sectorBounds.min.x)/float(MAP2_SIZE),
						    y + dy*(float)(data->destSector.y-sectorBounds.min.y)/float(MAP2_SIZE));
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
			float playerPower = 0;
			int primaryTeam = 0;
			if ( player ) {
				playerPower = player->GetItemComponent()->PowerRating();
				primaryTeam = player->GetItem()->primaryTeam;
			}

			for( int k=0; k<query.Size(); ++k ) {
				const GameItem* item = query[k]->GetItem();
				if ( GetRelationship( primaryTeam, item->primaryTeam ) == RELATE_ENEMY ) {

					float power = query[k]->GetItemComponent()->PowerRating();

					if ( item->keyValues.GetIString( "mob" ) == "greater" )
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
				if ( cc->InUse() ) {
					owner = TeamName( cc->PrimaryTeam() ).c_str();
				}
				str.Format( "%s\n%s\n%d-%d-%d G%d", sd.name, owner, low, med, high, greater );
			}
			map2Text[j*MAP2_SIZE+i].SetText( str.c_str() );
		}
	}

	if ( !data->destSector.IsZero() ) {
		const SectorData& sd = worldMap->GetSector( data->destSector );
		CStr<64> str;
		str.Format( "Grid Travel\n%s", sd.name.c_str() );
		gridTravel.SetText(  str.c_str() );
		gridTravel.SetEnabled( true );
	}
	else {
		gridTravel.SetText( "GridTravel" );
		gridTravel.SetEnabled( false );
	}
}


void MapScene::ItemTapped( const gamui::UIItem* item )
{
	Vector2I sector = { 0, 0 };
	
	if ( item == &okay ) {
		data->destSector.Zero();
		lumosGame->PopScene();
	}
	else if ( item == &gridTravel ) {
		lumosGame->PopScene();
	}
	else if ( item == &mapImage ) {
		float x=0, y=0;
		gamui2D.GetRelativeTap( &x, &y );
		sector.x = int( x * float(NUM_SECTORS) );
		sector.y = int( y * float(NUM_SECTORS) );
		data->destSector = sector;
		Resize();
	}
	else if ( item == &mapImage2 ) {
		float x=0, y=0;
		gamui2D.GetRelativeTap( &x, &y );

		sector.x = sectorBounds.min.x + int( x * float(sectorBounds.Width()));
		sector.y = sectorBounds.min.y + int( y * float(sectorBounds.Height()));
		data->destSector = sector;
		Resize();
	}
}


void MapScene::HandleHotKey( int value )
{
	if ( value == GAME_HK_MAP ) {
		data->destSector.Zero();
		lumosGame->PopScene();
	}
}



