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
#include "../script/itemscript.h"

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
	viewButton.Init(&gamui2D, lumosGame->GetButtonLook(0));
	viewButton.SetText("View");

	Texture* mapTexture = TextureManager::Instance()->GetTexture( "miniMap" );
	RenderAtom mapAtom( (const void*)UIRenderer::RENDERSTATE_UI_NORMAL_OPAQUE, (const void*)mapTexture, 0, 1, 1, 0 );
	mapImage.Init( &gamui2D, mapAtom, false );
	mapImage.SetCapturesTap( true );
	
	Rectangle2I sectorBounds = MapBounds2();
	Rectangle2F map2Bounds = ToWorld(sectorBounds);

	mapAtom.tx0 = map2Bounds.min.x / float(worldMap->Width());
	mapAtom.ty0 = map2Bounds.max.y / float(worldMap->Height());
	mapAtom.tx1 = map2Bounds.max.x / float(worldMap->Width());
	mapAtom.ty1 = map2Bounds.min.y / float(worldMap->Height());

	mapAtom.renderState = (void*)UIRenderer::RENDERSTATE_UI_DECO;
	mapImage2.Init( &gamui2D, mapAtom, false );
	mapImage2.SetCapturesTap( true );

	RenderAtom atom = lumosGame->CalcPaletteAtom( PAL_TANGERINE*2, PAL_ZERO );
	playerMark[0].Init( &gamui2D, atom, true );
	playerMark[1].Init( &gamui2D, atom, true );

	atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO_DISABLED;
	homeMark[0].Init( &gamui2D, atom, true );
	homeMark[1].Init( &gamui2D, atom, true );

	for (int i = 0; i < MAX_SQUADS; ++i) {
		static const char* NAME[MAX_SQUADS] = { "alphaMark", "betaMark", "deltaMark", "omegaMark" };
		RenderAtom atom = lumosGame->CalcUIIconAtom(NAME[i], true);
		squadMark[0][i].Init(&gamui2D, atom, true);
		squadMark[1][i].Init(&gamui2D, atom, true);
	}

	RenderAtom travelAtom = lumosGame->CalcPaletteAtom( PAL_GRAY*2, PAL_ZERO );
	travelAtom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO_DISABLED;
	travelMark[0].Init( &gamui2D, travelAtom, true );
	travelMark[1].Init( &gamui2D, travelAtom, true );

	RenderAtom selectionAtom = lumosGame->CalcUIIconAtom("mapSelection", true);
	selectionMark.Init(&gamui2D, selectionAtom, true);

	for( int i=0; i<MAP2_SIZE2; ++i ) {
		map2Text[i].Init( &gamui2D );
	}

	RenderAtom nullAtom;
	for (int i = 0; i < MAX_FACE; ++i) {
		face[i].Init(&gamui2D, nullAtom, true);
	}
}


MapScene::~MapScene()
{
}


Rectangle2F MapScene::GridBounds2(int x, int y, bool useGutter)
{
	float gridSize = mapImage2.Width() / float(MAP2_SIZE);
	float gutter = useGutter ? gamui2D.GetTextHeight() * 0.25f : 0;

	Rectangle2F r;
	r.Set(mapImage2.X() + float(x)*gridSize + gutter,
		  mapImage2.Y() + float(y)*gridSize + gutter,
		  mapImage2.X() + float(x + 1)*gridSize - gutter,
		  mapImage2.Y() + float(y + 1)*gridSize - gutter);
	return r;
}


Vector2F MapScene::ToUI(int select, const grinliz::Vector2F& pos, const grinliz::Rectangle2I& bounds, bool* inBounds)
{
	float dx = (pos.x - float(bounds.min.x)) / (float(bounds.max.x) - float(bounds.min.x));
	float dy = (pos.y - float(bounds.min.y)) / (float(bounds.max.y) - float(bounds.min.y));

	Vector2F v = { 0, 0 };
	const gamui::Image& image = (select == 0) ? mapImage : mapImage2;
	v.x = image.X() + dx * image.Width();
	v.y = image.Y() + dy * image.Height();

	if (inBounds) {
		*inBounds = dx >= 0 && dx <= 1 && dy >= 0 && dy <= 1;
	}
	return v;
}


void MapScene::Resize()
{
	lumosGame->PositionStd( &okay, 0 );

	const Screenport& port = lumosGame->GetScreenport();
	LayoutCalculator layout = lumosGame->DefaultLayout();
	
	layout.PosAbs( &gridTravel, 1, -1, 2, 1 );
	layout.PosAbs( &viewButton, 3, -1, 2, 1 );

	float y  = layout.GutterY();
	float dy = okay.Y() - layout.GutterY() - y;
	float x = layout.GutterX();
	float dx = port.UIWidth()*0.5f - 1.5f*layout.GutterX();
	dx = dy = Min( dx, dy );

	mapImage.SetSize( dx, dy );
	mapImage2.SetSize( dx, dy );

	mapImage.SetPos( x, y );
	mapImage2.SetPos( port.UIWidth()*0.5f + 0.5f*layout.GutterX(), y );

	for (int i = 0; i < 2; ++i) {
		const float MARK_SIZE = 5;
		const float SQUAD_MARK_SIZE = 20;
		playerMark[i].SetSize(MARK_SIZE, MARK_SIZE);
		homeMark[i].SetSize(dx / float(NUM_SECTORS), dy / float(NUM_SECTORS));
		travelMark[i].SetSize(dx / float(NUM_SECTORS), dy / float(NUM_SECTORS));
		for (int k = 0; k < MAX_SQUADS; ++k) {
			squadMark[i][k].SetSize(SQUAD_MARK_SIZE, SQUAD_MARK_SIZE);
		}
	}
	selectionMark.SetSize(float(MAP2_SIZE) * dx / float(NUM_SECTORS), float(MAP2_SIZE) *dx / float(NUM_SECTORS));
	DrawMap();
}


Rectangle2I MapScene::MapBounds2()
{
	Vector2I subOrigin = data->destSector;
	if (subOrigin.IsZero()) {
		subOrigin = lumosChitBag->GetHomeSector();
	}
	if (subOrigin.IsZero()) {
		subOrigin.Set(NUM_SECTORS / 2, NUM_SECTORS / 2);
	}

	if ( subOrigin.x < MAP2_RAD )					subOrigin.x = MAP2_RAD;
	if ( subOrigin.y < MAP2_RAD )					subOrigin.y = MAP2_RAD;
	if ( subOrigin.x >= NUM_SECTORS - MAP2_RAD )	subOrigin.x = NUM_SECTORS - MAP2_RAD - 1;
	if ( subOrigin.y >= NUM_SECTORS - MAP2_RAD )	subOrigin.y = NUM_SECTORS - MAP2_RAD - 1;
	Rectangle2I subBounds;
	subBounds.min = subBounds.max = subOrigin;
	subBounds.Outset(MAP2_RAD);

	return subBounds;
}

void MapScene::DrawMap()
{
	CDynArray<Chit*> query;

	int primaryTeam = lumosChitBag->GetHomeTeam();
	int nFace = 0;

	Rectangle2I subBounds = MapBounds2();
	float map2X = float(subBounds.min.x) / float(NUM_SECTORS);
	float map2Y = float(subBounds.min.x) / float(NUM_SECTORS);
	RenderAtom subAtom = mapImage.GetRenderAtom();
	subAtom.tx0 = map2X;
	subAtom.ty0 = map2Y;
	subAtom.tx1 = map2X + float(MAP2_SIZE) / float(NUM_SECTORS);
	subAtom.ty1 = map2Y + float(MAP2_SIZE) / float(NUM_SECTORS);
	mapImage2.SetAtom(subAtom);

	for (Rectangle2IIterator it(subBounds); !it.Done(); it.Next()) {
		Vector2I sector = it.Pos();
		const SectorData& sd = worldMap->GetSectorData( sector );

		int i = (sector.x - subBounds.min.x);
		int j = (sector.y - subBounds.min.y);
		int index = j * MAP2_SIZE + i;

		Rectangle2I inner = sd.InnerBounds();
		Rectangle2F innerF = ToWorld(inner);
		CoreScript* coreScript = CoreScript::GetCore(sector);

		CStr<64> str = "";
		const char* owner = "";
		if (coreScript) {
			if ( coreScript->InUse() ) {
				owner = Team::TeamName( coreScript->ParentChit()->Team() ).safe_str();
			}
		}
		str.Format( "%s\n%s", sd.name, owner );

		Rectangle2F r = GridBounds2(i, j, true);
		map2Text[j*MAP2_SIZE + i].SetPos(r.min.x, r.min.y);
		map2Text[j*MAP2_SIZE+i].SetBounds( r.Width(), 0 );
		map2Text[index].SetText( str.c_str() );

		for (int pass = 0; pass < 2; ++pass) {
			MOBKeyFilter mobFilter;
			CChitArray query;
			if (pass == 0) {
				CChitArray arr0;
				mobFilter.value = ISC::lesser;
				lumosChitBag->QuerySpatialHash(&query, ToWorld(inner), 0, &mobFilter);
				mobFilter.value = ISC::denizen;
				lumosChitBag->QuerySpatialHash(&arr0, ToWorld(inner), 0, &mobFilter);
				for (int i = 0; i < arr0.Size(); ++i) {
					if (query.HasCap()) {
						query.Push(arr0[i]);
					}
				}
			}
			else {
				mobFilter.value = ISC::greater;
				lumosChitBag->QuerySpatialHash(&query, ToWorld(inner), 0, &mobFilter);
			}

			CArray<MCount, 16> counter;
			for (int k = 0; k < query.Size(); ++k) {
				const GameItem* item = query[k]->GetItem();
				IString name = item->IName();
				int idx = counter.Find(name);
				if (idx >= 0) {
					counter[idx].count += 1;
				}
				else {
					MCount c(name);
					counter.Push(c);
				}
			}
			Sort<MCount, MCountSorter>(counter.Mem(), counter.Size());

			Rectangle2F r = GridBounds2(i, j, true);
			float h = r.Width() / 3.0f;
			float x = r.min.x;
			float y = r.min.y + h * float(pass + 1);

			for (int k = 0; k < counter.Size() && k < MAX_COL && nFace < MAX_FACE; ++k) {
				float ratio = 1;
				RenderAtom atom = LumosGame::CalcUIIconAtom(counter[k].name.c_str(), true, &ratio);
				atom.renderState = (void*)UIRenderer::RENDERSTATE_UI_NORMAL;
				face[nFace].SetAtom(atom);
				face[nFace].SetSize(ratio * h, h);
				face[nFace].SetPos(x + float(k)*h, y);
				face[nFace].SetVisible(true);
				++nFace;
			}
		}
	}

	for (int i = nFace; i < MAX_FACE; ++i) {
		face[i].SetVisible(false);
	}

	Vector2I homeSector = lumosChitBag->GetHomeSector();
	if ( !data->destSector.IsZero() && data->destSector != homeSector ) {
		const SectorData& sd = worldMap->GetSectorData( data->destSector );
		CStr<64> str;
		str.Format( "Grid Travel\n%s", sd.name.c_str() );
		gridTravel.SetText(  str.c_str() );
		gridTravel.SetEnabled( true );
	}
	else {
		gridTravel.SetText( "Grid Travel" );
		gridTravel.SetEnabled( false );
	}

	// --- MAIN ---
	Rectangle2I mapBounds = data->worldMap->Bounds();
	Rectangle2I map2Bounds;
	map2Bounds.Set(subBounds.min.x*SECTOR_SIZE, subBounds.min.y*SECTOR_SIZE, subBounds.max.x*SECTOR_SIZE, subBounds.max.y*SECTOR_SIZE);

	Vector2F playerPos = { 0, 0 };
	Chit* player = data->player;
	if ( player ) {
		playerPos = player->GetSpatialComponent()->GetPosition2D();
	}

	bool inBounds = true;
	Vector2F v;

	for (int i = 0; i < 2; ++i) {
		const Rectangle2I& b = (i == 0) ? mapBounds : map2Bounds;

		v = ToUI(i, playerPos, b, &inBounds);
		playerMark[i].SetCenterPos(v.x, v.y);
		playerMark[i].SetVisible(inBounds);

		Vector2F pos = { float(homeSector.x * SECTOR_SIZE), float(homeSector.y * SECTOR_SIZE) };
		v = ToUI(i,pos, b, &inBounds);
		homeMark[i].SetPos(v.x, v.y);
		homeMark[i].SetVisible(inBounds && !homeSector.IsZero());

		pos.Set(float(data->destSector.x * SECTOR_SIZE), float(data->destSector.y * SECTOR_SIZE));
		v = ToUI(i,pos, b, &inBounds);
		travelMark[i].SetPos(v.x, v.y);
		travelMark[i].SetVisible(inBounds && !data->destSector.IsZero());

		for (int k = 0; k < MAX_SQUADS; ++k) {
			v = ToUI(i, ToWorld2F(data->squadDest[k]), b, &inBounds);
			squadMark[i][k].SetCenterPos(v.x, v.y);
			squadMark[i][k].SetVisible(!data->squadDest[k].IsZero() && inBounds);
		}
	}
	{
		Vector2F world = { (float)map2Bounds.min.x, (float)map2Bounds.min.y };
		Vector2F pos = ToUI(0, world, mapBounds, 0);
		selectionMark.SetPos(pos.x, pos.y);
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
	else if ( item == &viewButton ) {
		data->view = true;
		lumosGame->PopScene();
	}
	else if ( item == &mapImage ) {
		float x=0, y=0;
		gamui2D.GetRelativeTap( &x, &y );
		sector.x = int( x * float(NUM_SECTORS) );
		sector.y = int( y * float(NUM_SECTORS) );
		data->destSector = sector;
		DrawMap();
	}
	else if ( item == &mapImage2 ) {
		float x=0, y=0;
		gamui2D.GetRelativeTap( &x, &y );
		Rectangle2I b = MapBounds2();
		sector.x = b.min.x + int( x * float(b.Width()) );
		sector.y = b.min.y + int( y * float(b.Height()) );
		data->destSector = sector;
		DrawMap();
	}
}


void MapScene::HandleHotKey( int value )
{
	if ( value == GAME_HK_MAP ) {
		data->destSector.Zero();
		lumosGame->PopScene();
	}
}



