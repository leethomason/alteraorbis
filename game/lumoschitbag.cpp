#include "lumoschitbag.h"
#include "gameitem.h"
#include "gamelimits.h"
#include "census.h"
#include "pathmovecomponent.h"
#include "aicomponent.h"
#include "debugstatecomponent.h"
#include "healthcomponent.h"
#include "mapspatialcomponent.h"
#include "reservebank.h"
#include "worldmap.h"
#include "worldgrid.h"
#include "worldinfo.h"
#include "gridmovecomponent.h"
#include "team.h"
#include "visitorstatecomponent.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"

#include "../script/battlemechanics.h"
#include "../script/itemscript.h"
#include "../script/scriptcomponent.h"
#include "../script/corescript.h"
#include "../script/plantscript.h"
#include "../script/procedural.h"


//#define DEBUG_EXPLOSION

using namespace grinliz;

LumosChitBag::LumosChitBag() : engine(0), worldMap(0), lumosGame(0)
{
}


LumosChitBag::~LumosChitBag()
{
	// Call the parent function so that chits aren't 
	// aren't using deleted memory.
	DeleteAll();
}



Chit* LumosChitBag::NewBuilding( const Vector2I& pos, const char* name, int team )
{
	Chit* chit = NewChit();

	const GameItem& rootItem = ItemDefDB::Instance()->Get( name );

	int cx=1;
	rootItem.GetValue( "size", &cx );

	MapSpatialComponent* msc = new MapSpatialComponent( worldMap );
	msc->SetMapPosition( pos.x, pos.y, cx, cx );
	msc->SetMode( GRID_BLOCKED );
	msc->SetBuilding( true );
	
	chit->Add( msc );
	chit->Add( new RenderComponent( engine, rootItem.ResourceName() ));
	chit->Add( new HealthComponent( engine ));
	AddItem( name, chit, engine, team, 0 );

	IString proc = rootItem.GetValue( "procedural" );
	if ( !proc.empty() ) {
		TeamGen gen;
		ProcRenderInfo info;
		gen.Assign( team, &info );
		chit->GetRenderComponent()->SetProcedural( 0, info );
	}

#if 0	// debugging
	SectorPort sp;
	sp.sector.Set( pos.x/SECTOR_SIZE, pos.y/SECTOR_SIZE );
	sp.port = 1;
	worldMap->SetRandomPort( sp );
#endif

	return chit;
}


Chit* LumosChitBag::NewMonsterChit( const Vector3F& pos, const char* name, int team )
{
	Chit* chit = NewChit();

	chit->Add( new SpatialComponent());
	AddItem( name, chit, engine, team, 0 );
	chit->GetItemComponent()->AddGold( ReserveBank::Instance()->WithdrawMonster() );

	chit->Add( new RenderComponent( engine, chit->GetItem()->ResourceName() ));
	chit->Add( new PathMoveComponent( worldMap ));
	chit->Add( new AIComponent( engine, worldMap ));

	chit->GetSpatialComponent()->SetPosition( pos );

	chit->Add( new HealthComponent( engine ));
	return chit;
}


Chit* LumosChitBag::NewWorkerChit( const Vector3F& pos, int team )
{
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "worker" );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, rootItem.ResourceName() ));
	chit->Add( new PathMoveComponent( worldMap ));
	chit->Add( new AIComponent( engine, worldMap ));
	chit->GetSpatialComponent()->SetPosition( pos );
	AddItem( rootItem.Name(), chit, engine, team, 0 );
	chit->Add( new HealthComponent( engine ));
	chit->Add( new DebugStateComponent( worldMap ));
	return chit;
}


Chit* LumosChitBag::NewVisitor( int visitorIndex )
{
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "visitor" );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, rootItem.ResourceName() ));

	AIComponent* ai = new AIComponent( engine, worldMap );
	chit->Add( ai );
	GLASSERT( visitorIndex >= 0 && visitorIndex < Visitors::NUM_VISITORS );
	ai->SetVisitorIndex( visitorIndex );
	Visitors::Instance()->visitorData[visitorIndex].Connect();	// initialize.

	// Visitors start at world center, with gridMove, and go from there.
	Vector3F pos = { (float)worldMap->Width()*0.5f, 0.0f, (float)worldMap->Height()*0.5f };
	chit->GetSpatialComponent()->SetPosition( pos );

	GridMoveComponent* gmc = new GridMoveComponent( worldMap );
	chit->Add( gmc );

	SectorPort sp = worldMap->RandomPort( &random );
	gmc->SetDest( sp );

	AddItem( rootItem.Name(), chit, engine, TEAM_VISITOR, 0 );
	chit->Add( new HealthComponent( engine ));
	chit->Add( new VisitorStateComponent( worldMap ));
	return chit;
}


Chit* LumosChitBag::QueryRemovable( const grinliz::Vector2I& pos2i )
{
	Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

	CChitArray array;
	RemovableFilter removableFilter;
	QuerySpatialHash( &array, pos2, 0.1f, 0, &removableFilter );

	Chit* found = 0;
	// First pass: plants & 1x1 buildings.
	for( int i=0; i<array.Size(); ++i ) {
		found = array[i];
		break;
	}
	// Check for 2x2 buildings.
	if ( !found ) {
		found = QueryBuilding( pos2i );
	}
	return found;
}


Chit* LumosChitBag::QueryBuilding( const grinliz::Rectangle2I& bounds )
{
	GLASSERT( MAX_BUILDING_SIZE == 2 );	// else adjust logic
	static const float EPS   = 0.1f;

	Rectangle2F r;
	r.min.x = (float)bounds.min.x - EPS;
	r.min.y = (float)bounds.min.y - EPS;
	r.max.x = (float)(bounds.max.x+1) + EPS;
	r.max.y = (float)(bounds.max.y+1) + EPS;

	BuildingFilter buildingFilter;
	CChitArray arr;
	this->QuerySpatialHash( &arr, r, 0, &buildingFilter );

	Chit* building = 0;
	if ( arr.Size() > 0 ) {
		building = arr[0];

#ifdef DEBUG
		MapSpatialComponent* msc = GET_SUB_COMPONENT( building, SpatialComponent, MapSpatialComponent );
		GLASSERT( msc->Bounds().Intersect( bounds ));		
#endif
	}
	return building;
}


Chit* LumosChitBag::QueryPorch( const grinliz::Vector2I& pos )
{
	GLASSERT( MAX_BUILDING_SIZE == 2 );	// else adjust logic
	static const float EPS   = 0.1f;
	static const float DELTA = 1.5f;
	static const float OFFSET = DELTA + EPS;

	Rectangle2F r;
	r.min.x = (float)pos.x + 0.5f - OFFSET;
	r.min.y = (float)pos.y + 0.5f - OFFSET;
	r.max.x = (float)pos.x + 0.5f + OFFSET;
	r.max.y = (float)pos.y + 0.5f + OFFSET;

	BuildingFilter buildingFilter;
	CChitArray arr;
	this->QuerySpatialHash( &arr, r, 0, &buildingFilter );

	for( int i=0; i<arr.Size(); ++i ) {
		Chit* chit = arr[i];
		MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
		GLASSERT( msc );	// all buildings should be msc's
		GameItem* item = chit->GetItem();
		if ( item ) {
			int porch = 0;
			item->GetValue( "porch", &porch );
			if ( msc && porch ) {
				if ( msc->PorchPos() == pos ) {
					return chit;
				}
			}
		}
	}
	return 0;
}


Chit* LumosChitBag::NewGoldChit( const grinliz::Vector3F& pos, int amount )
{
	if ( !amount )
		return 0;

	Vector2F v2 = { pos.x, pos.z };

	ItemNameFilter goldFilter( IStringConst::gold );
	this->QuerySpatialHash( &chitList, v2, 1.0f, 0, &goldFilter );
	Chit* chit = 0;
	if ( chitList.Size() ) {
		chit = chitList[0];
	}
	if ( !chit ) {
		chit = this->NewChit();
		chit->Add( new SpatialComponent());
		AddItem( "gold", chit, engine, 0, 0 );
		chit->Add( new RenderComponent( engine, chit->GetItem()->ResourceName() ));
		chit->GetSpatialComponent()->SetPosition( pos );

	}
	chit->GetItemComponent()->AddGold( amount );
	return chit;
}


Chit* LumosChitBag::NewCrystalChit( const grinliz::Vector3F& pos, int crystal, bool fuzz )
{
	Vector2F v2 = { pos.x, pos.z };
	if ( fuzz ) {
		v2.x = floorf(pos.x) + random.Uniform();
		v2.y = floorf(pos.y) + random.Uniform();
	}

	const char* name = 0;
	switch ( crystal ) {
	case CRYSTAL_GREEN:		name="crystal_green";	break;
	case CRYSTAL_RED:		name="crystal_red";		break;
	case CRYSTAL_BLUE:		name="crystal_blue";	break;
	case CRYSTAL_VIOLET:	name="crystal_violet";	break;
	}

	Chit* chit = this->NewChit();
	chit->Add( new SpatialComponent());
	AddItem( name, chit, engine, 0, 0 );
	chit->Add( new RenderComponent( engine, chit->GetItem()->ResourceName() ));
	chit->GetSpatialComponent()->SetPosition( pos );
	
	Wallet w;
	w.AddCrystal( crystal );
	chit->GetItemComponent()->AddGold( w );

	return chit;
}


void LumosChitBag::NewWalletChits( const grinliz::Vector3F& pos, const Wallet& wallet )
{
	NewGoldChit( pos, wallet.gold );
	for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
		for( int j=0; j<wallet.crystal[i]; ++j ) {
			NewCrystalChit( pos, i, true );
		}
	}
}


Chit* LumosChitBag::NewItemChit( const grinliz::Vector3F& _pos, GameItem* orphanItem, bool fuzz, bool onGround )
{
	GLASSERT( !orphanItem->Intrinsic() );
	GLASSERT( !orphanItem->resource.empty() );

	Vector3F pos = _pos;
	if ( fuzz ) {
		pos.x = floorf(pos.x) + random.Uniform();
		pos.z = floorf(pos.z) + random.Uniform();
	}

	Chit* chit = this->NewChit();
	chit->Add( new SpatialComponent());
	chit->Add( new ItemComponent( engine, worldMap, orphanItem ));
	chit->Add( new RenderComponent( engine, orphanItem->ResourceName() ));

	if ( onGround ) {
		const ModelResource* res = chit->GetRenderComponent()->MainResource();
		Rectangle3F aabb = res->AABB();
		pos.y = -aabb.min.y;

		if ( aabb.SizeY() < 0.1f ) {
			pos.y += 0.1f;
		}
	}
	chit->GetSpatialComponent()->SetPosition( pos );
	chit->GetItemComponent()->SetHardpoints();
	return chit;
}


void LumosChitBag::HandleBolt( const Bolt& bolt, const ModelVoxel& mv )
{
	GLASSERT( engine );
	Chit* chitShooter = GetChit( bolt.chitID );	// may be null
	int shooterTeam = -1;
	if ( chitShooter && chitShooter->GetItemComponent() ) {
		shooterTeam = chitShooter->GetItemComponent()->GetItem()->primaryTeam;
	}
	int explosive = bolt.effect & GameItem::EFFECT_EXPLOSIVE;
 
	if ( !explosive ) {
		if ( mv.Hit() ) {
			Chit* chitHit = mv.ModelHit() ? mv.model->userData : 0;
			DamageDesc dd( bolt.damage, bolt.effect );

			if ( chitHit ) {
				GLASSERT( GetChit( chitHit->ID() ) == chitHit );
				if ( chitHit->GetItemComponent() &&
					 chitHit->GetItemComponent()->GetItem()->primaryTeam == shooterTeam ) 
				{
					// do nothing. don't shoot own team.
				}
				else {
		
					ChitDamageInfo info( dd );
					info.originID = bolt.chitID;
					info.awardXP  = true;
					info.isMelee  = false;
					info.isExplosion = false;
					info.originOfImpact = mv.at;

					ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &info );
					chitHit->SendMessage( msg, 0 );
				}
			}
			else {
				worldMap->VoxelHit( mv.voxel, dd );
			}
		}
	}
	else {
		// How it used to work. Now only uses radius:
			// Here don't worry about the chit hit. Just ray cast to see
			// who is in range of the explosion and takes damage.
		
			// Back up the origin of the bolt just a bit, so it doesn't keep
			// intersecting the model it hit. Then do ray checks around to 
			// see what gets hit by the explosion.

		float rewind = Min( 0.1f, 0.5f*bolt.len );
		GLASSERT( Equal( bolt.dir.Length(), 1.f, 0.001f ));
		Vector3F origin = mv.at - bolt.dir * rewind;

		DamageDesc dd( bolt.damage, bolt.effect );
		BattleMechanics::GenerateExplosionMsgs( dd, origin, bolt.chitID, engine, this );

		if ( mv.VoxelHit() ) {
			worldMap->VoxelHit( mv.voxel, dd );
		}
	}
}


void LumosChitBag::AddItem( const char* name, Chit* chit, Engine* engine, int team, int level )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get( name, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	GameItem item = *(itemDefArr[0]);
	item.primaryTeam = team;
	item.traits.SetExpFromLevel( level );
	item.InitState();

	if ( !chit->GetItemComponent() ) {
		ItemComponent* ic = new ItemComponent( engine, worldMap, item );
		chit->Add( ic );
		for( int i=1; i<itemDefArr.Size(); ++i ) {
			ic->AddToInventory( new GameItem( *(itemDefArr[i]) ) );
		}
	}
	else {
		GLASSERT( itemDefArr.Size() == 1 );
		chit->GetItemComponent()->AddToInventory( new GameItem( item ) );
	}
}


int LumosChitBag::MapGridUse( int x, int y )
{
	GLASSERT( MAX_BUILDING_SIZE == 2 );
	// An object is either at the center or influence by
	// something with 0.5 + EPS radius.

	static const float OFFSET = 0.6f;

	Rectangle2F r;
	r.min.x = (float)x + 0.5f - OFFSET;
	r.min.y = (float)y + 0.5f - OFFSET;
	r.max.x = (float)x + 0.5f + OFFSET;
	r.max.y = (float)y + 0.5f + OFFSET;

	ChitHasMapSpatial hasMapSpatial;
	QuerySpatialHash( &inUseArr, r, 0, &hasMapSpatial );
	int flags = 0;
	for( int i=0; i<inUseArr.Size(); ++i ) {
		MapSpatialComponent* ms = GET_SUB_COMPONENT( inUseArr[i], SpatialComponent, MapSpatialComponent );
		GLASSERT( ms );
		if ( ms->Bounds().Contains( x, y )) {
			flags |= ms->Mode();
		}
	}
	return flags;
}



CoreScript* LumosChitBag::IsBoundToCore( Chit* chit, bool mustBeStanding )
{
	// Since the player can eject and walk around, the core
	// the player is bound to can be anywhere in the world.
	int coreID = 0;
	if ( chit && chitToCoreTable.Query( chit->ID(), &coreID )) {
		Chit* cc = GetChit( coreID );
		if ( cc ) {
			ScriptComponent* sc = cc->GetScriptComponent();
			GLASSERT( sc );
			IScript* script = sc->Script();
			GLASSERT( script );
			CoreScript* coreScript = script->ToCoreScript();
			GLASSERT( coreScript );
			bool standing = false;
			Chit* chit = coreScript->GetAttached(&standing);

			if ( mustBeStanding ) 
				return (standing && chit) ? coreScript : 0;
			return coreScript;
		}
	}
	return 0;
}


CoreScript* LumosChitBag::GetCore( const grinliz::Vector2I& sector )
{
	const SectorData& sd = worldMap->GetSector( sector );
	if ( sd.HasCore() ) {
		Vector2I pos2i = sd.core;
		Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

		CChitArray array;
		ItemNameFilter coreFilter( IStringConst::core );
		QuerySpatialHash( &array, pos2, 0.1f, 0, &coreFilter );
		GLASSERT( !array.Empty() );

		if ( !array.Empty() ) {
			Chit* cc = array[0];
			ScriptComponent* sc = cc->GetScriptComponent();
			GLASSERT( sc );
			IScript* script = sc->Script();
			GLASSERT( script );
			CoreScript* coreScript = script->ToCoreScript();
			GLASSERT( coreScript );
			return coreScript;
		}
	}
	return 0;
}


ItemNameFilter::ItemNameFilter()
{
	cNames = 0;
	iNames = 0;
	count  = 0;
}


ItemNameFilter::ItemNameFilter( const char* arr[], int n )
{
	cNames = arr;
	iNames = 0;
	count  = n;
}


ItemNameFilter::ItemNameFilter( const grinliz::IString& istring )
{
	cNames = 0;
	iNames = &istring;
	count  = 1;
}


ItemNameFilter::ItemNameFilter( const grinliz::IString* arr, int n )
{
	cNames = 0;
	iNames = arr;
	count  = n;
}


bool ItemNameFilter::Accept( Chit* chit )
{
	const GameItem* item = chit->GetItem();
	if ( item ) {
		if ( cNames ) {
			const char* key = item->Name();
			for( int i=0; i<count; ++i ) {
				if ( StrEqual( cNames[i], key ))
					return true;
			}
		}
		else if ( iNames ) {
			const IString& key = item->name;
			for( int i=0; i<count; ++i ) {
				if ( key == iNames[i] )
					return true;
			}
		}
	}
	return false;
}


GoldCrystalFilter::GoldCrystalFilter()
{
	arr[0] = IStringConst::gold;
	arr[1] = IStringConst::crystal_green;
	arr[2] = IStringConst::crystal_red;
	arr[3] = IStringConst::crystal_blue;
	arr[4] = IStringConst::crystal_violet;
	iNames = arr;
	count = 5;
}


bool BuildingFilter::Accept( Chit* chit ) 
{
	// Assumed to be MapSpatial with "building" flagged on.
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	return msc && msc->Building();
}


PlantFilter::PlantFilter( int type, int max )
{
	typeFilter = type;
	maxStage = max;
}


bool PlantFilter::Accept( Chit* chit )
{
	int type = 0;
	int stage = 0;
	bool okay = PlantScript::IsPlant( chit, &type, &stage ) != 0;
	if ( okay ) {
		if ( typeFilter >= 0 && (type != typeFilter)) {
			okay = false;
		}
	}
	if ( okay && stage > maxStage ) {
		okay = false;
	}
	return okay;
}


bool RemovableFilter::Accept( Chit* chit )
{
	return buildingFilter.Accept( chit ) || plantFilter.Accept( chit );
}


bool ChitHasMapSpatial::Accept( Chit* chit ) 
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	return msc != 0;
}


Bolt* LumosChitBag::NewBolt(	const Vector3F& pos,
								Vector3F dir,
								int effectFlags,
								int chitID,
								float damage,
								float speed,
								bool trail )
{
	Bolt* bolt = ChitBag::NewBolt();

	dir.Normalize();
	bolt->head = pos + dir*0.5f;
	bolt->len = 0.5f;
	bolt->dir = dir;

	const Game::Palette* palette = Game::GetMainPalette();

	switch( effectFlags & (GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK) ) {
	case 0:													bolt->color = palette->GetV4F( 1, PAL_GREEN );	break;
	case GameItem::EFFECT_FIRE:								bolt->color = palette->GetV4F( 1, PAL_RED );	break;
	case GameItem::EFFECT_SHOCK:							bolt->color = palette->GetV4F( 1, PAL_BLUE );	break;
	case GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK:	bolt->color = palette->GetV4F( 1, PAL_PURPLE );	break;
	default:
		GLASSERT( 0 );
		break;
	}

	bolt->chitID = chitID;
	bolt->damage = damage;
	bolt->effect = effectFlags;
	bolt->particle  = trail;
	bolt->speed = speed;

	return bolt;
}
