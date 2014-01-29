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
#include "lumosmath.h"
#include "lumosgame.h"	// FIXME: namegen should be in script

#include "../scenes/characterscene.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"
#include "../engine/particle.h"

#include "../script/battlemechanics.h"
#include "../script/itemscript.h"
#include "../script/scriptcomponent.h"
#include "../script/corescript.h"
#include "../script/plantscript.h"
#include "../script/procedural.h"


//#define DEBUG_EXPLOSION

using namespace grinliz;

LumosChitBag::LumosChitBag( const ChitContext& c) : ChitBag(c), sceneID(-1), sceneData(0)
{
	memset( mapSpatialHash, 0, sizeof(MapSpatialComponent*)*NUM_SECTORS*NUM_SECTORS);
	memset( coreCache, 0, sizeof(Chit*)*NUM_SECTORS*NUM_SECTORS );
}


LumosChitBag::~LumosChitBag()
{
	// Call the parent function so that chits aren't 
	// aren't using deleted memory.
	DeleteAll();
	delete sceneData;
}


#if 0
void LumosChitBag::AddToSpatialHash( Chit* chit, int x, int y )
{
	//MoBFilter filter : This seems like a good idea, accept that the chit can
	// of course change its components, has an update sequence,
	// and may or may not match criteria. 
	Vector2I sector = { 0, 0 };
	if ( worldMap->UsingSectors() ) {
		sector.Set( x / SECTOR_SIZE, y / SECTOR_SIZE );
	}
	chitsInSector[sector.y*NUM_SECTORS+sector.x].Push( chit );
	super::AddToSpatialHash( chit, x, y );
}
#endif


void LumosChitBag::AddToBuildingHash( MapSpatialComponent* chit, int x, int y )
{
	int sx = x / SECTOR_SIZE;
	int sy = y / SECTOR_SIZE;
	GLASSERT( sx >= 0 && sx < NUM_SECTORS );
	GLASSERT( sy >= 0 && sy < NUM_SECTORS );
	GLASSERT( chit->nextBuilding == 0 );
	
	chit->nextBuilding = mapSpatialHash[sy*NUM_SECTORS+sx];
	mapSpatialHash[sy*NUM_SECTORS+sx] = chit;
}


void LumosChitBag::RemoveFromBuildingHash( MapSpatialComponent* chit, int x, int y )
{
	int sx = x / SECTOR_SIZE;
	int sy = y / SECTOR_SIZE;
	GLASSERT( sx >= 0 && sx < NUM_SECTORS );
	GLASSERT( sy >= 0 && sy < NUM_SECTORS );
	GLASSERT( mapSpatialHash[sy*NUM_SECTORS+sx] );

	MapSpatialComponent* prev = 0;
	for( MapSpatialComponent* it = mapSpatialHash[sy*NUM_SECTORS+sx]; it; prev = it, it = it->nextBuilding ) {
		if ( it == chit ) {
			if ( prev ) {
				prev->nextBuilding = it->nextBuilding;
			}
			else {
				mapSpatialHash[sy*NUM_SECTORS+sx] = it->nextBuilding;
			}
			it->nextBuilding = 0;
			return;
		}
	}
	GLASSERT( 0 );
}


Chit* LumosChitBag::FindBuilding(	const grinliz::IString&  name, 
									const grinliz::Vector2I& sector, 
									const grinliz::Vector2F* pos, 
									int flags,
									CDynArray<Chit*>* arr,
									IChitAccept* filter )
{
	CDynArray<Chit*>& match = arr ? *arr : findMatch;	// sleazy reference trick to point to either passed in or local.
	match.Clear();
	findWeight.Clear();

	for( MapSpatialComponent* it = mapSpatialHash[sector.y*NUM_SECTORS+sector.x]; it; it = it->nextBuilding ) {
		Chit* chit = it->ParentChit();
		GLASSERT( chit );
		if ( filter && !filter->Accept( chit )) {				// if a filter, check it.
			continue;
		}

		const GameItem* item = chit->GetItem();

		if ( item && ( name.empty() || item->IName() == name )) {	// name, if empty, matches everything
			match.Push( chit );
		}
	}

	// If we found nothing, or we don't care about the position, return early.
	// Else deal with choice / sorting / etc. below.
	if ( match.Empty() )
		return 0;
	if ( !pos )
		return match[0];

	// NEAREST scans and finds the closest one.
	// RANDOM_NEAR chooses one at random, but weighted by the (inverse) of the distance
	if ( flags == NEAREST ) {
		float closest = ( match[0]->GetSpatialComponent()->GetPosition2D() - *pos ).LengthSquared();
		int   ci = 0;
		for( int i=1; i<match.Size(); ++i ) {
			float len2 = ( match[i]->GetSpatialComponent()->GetPosition2D() - *pos ).LengthSquared();
			if ( len2 < closest ) {
				closest = len2;
				ci = i;
			}
		}
		return match[ci];
	}
	if ( flags == RANDOM_NEAR ) {
		for( int i=0; i<match.Size(); ++i ) {
			float len = ( match[i]->GetSpatialComponent()->GetPosition2D() - *pos ).Length();
			findWeight.Push( 1.0f/len );
		}
		int index = random.Select( findWeight.Mem(), findWeight.Size() );
		return match[index];
	}

	// Bad flag? Something didn't return?
	GLASSERT( 0 );
	return 0;
}


Chit* LumosChitBag::NewBuilding( const Vector2I& pos, const char* name, int team )
{
	const ChitContext* context = GetContext();
	Chit* chit = NewChit();

	GameItem rootItem = ItemDefDB::Instance()->Get( name );

	// Hack...how to do this better??
	if ( rootItem.IResourceName() == "pyramid0" ) {
		CStr<32> str;
		str.Format( "pyramid%d", random.Rand(3) );
		rootItem.SetResource( str.c_str() );
	}

	int cx=1;
	rootItem.keyValues.GetInt( "size", &cx );
	int porch=0;
	rootItem.keyValues.GetInt( "porch", &porch );

	MapSpatialComponent* msc = new MapSpatialComponent();
	msc->SetMapPosition( pos.x, pos.y, cx, cx );
	msc->SetMode( GRID_BLOCKED );
	msc->SetBuilding( true, porch != 0 );
	
	chit->Add( msc );
	chit->Add( new RenderComponent( rootItem.ResourceName() ));
	chit->Add( new HealthComponent());
	AddItem( name, chit, context->engine, team, 0 );

	IString proc = rootItem.keyValues.GetIString( "procedural" );
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

	context->engine->particleSystem->EmitPD( "constructiondone", ToWorld3F( pos ), V3F_UP, 0 );

	return chit;
}


Chit* LumosChitBag::NewMonsterChit( const Vector3F& pos, const char* name, int team )
{
	const ChitContext* context = GetContext();
	Chit* chit = NewChit();

	chit->Add( new SpatialComponent());
	AddItem( name, chit, context->engine, team, 0 );

	chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
	chit->Add( new PathMoveComponent());
	chit->Add( new AIComponent());

	chit->GetSpatialComponent()->SetPosition( pos );

	chit->Add( new HealthComponent());

	Wallet w = ReserveBank::Instance()->WithdrawMonster();
	if ( chit->GetItem()->keyValues.GetIString( "mob" ) == "greater" ) {
		// can return NUM_CRYSTAL_TYPES, if out, which is fine.
		w.AddCrystal( ReserveBank::Instance()->WithdrawRandomCrystal() );;
		
		NewsHistory* history = NewsHistory::Instance();
		if ( history ) {
			history->Add( NewsEvent( NewsEvent::GREATER_MOB_CREATED, ToWorld2F(pos), chit, 0 ));
			chit->GetItem()->keyValues.Set( "destroyMsg", "d", NewsEvent::GREATER_MOB_KILLED );
		}
	}
	chit->GetItem()->wallet.Add( w );	
	return chit;
}


Chit* LumosChitBag::NewDenizen( const grinliz::Vector2I& pos, int team )
{
	const ChitContext* context = GetContext();
	bool female = true;
	const char* assetName = "humanFemale";
	if ( random.Bit() ) {
		female = false;
		assetName = "humanMale";
	}

	Chit* chit = NewChit();

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( assetName ));
	chit->Add( new PathMoveComponent());

	AddItem( assetName, chit, context->engine, team, 0 );
	chit->GetItem()->wallet.AddGold( ReserveBank::Instance()->WithdrawDenizen() );
	chit->GetItem()->GetTraitsMutable()->Roll( random.Rand() );

	IString nameGen = chit->GetItem()->keyValues.GetIString( "nameGen" );
	if ( !nameGen.empty() ) {
		LumosGame* game = chit->GetChitBag()->GetContext()->game;
		if ( game ) {
			chit->GetItem()->SetProperName( StringPool::Intern( 
				game->GenName(	nameGen.c_str(), 
								chit->ID(),
								4, 8 )));
		}
	}

	AIComponent* ai = new AIComponent();
	chit->Add( ai );

	chit->Add( new HealthComponent());
	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );
//	chit->GetItemComponent()->SetHardpoints(); automatic now

	Vector2I sector = ToSector( pos );
	GetCore( sector )->AddCitizen( chit );

	NewsHistory* history = NewsHistory::Instance();
	if ( history ) {
		history->Add( NewsEvent( NewsEvent::DENIZEN_CREATED, ToWorld2F(pos), chit, 0 ));
		chit->GetItem()->keyValues.Set( "destroyMsg", "d", NewsEvent::DENIZEN_KILLED );
	}

	return chit;
}


Chit* LumosChitBag::NewWorkerChit( const Vector3F& pos, int team )
{
	const ChitContext* context = GetContext();
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "worker" );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( rootItem.ResourceName() ));
	chit->Add( new PathMoveComponent());
	chit->Add( new AIComponent());
	chit->GetSpatialComponent()->SetPosition( pos );
	AddItem( rootItem.Name(), chit, context->engine, team, 0 );
	chit->Add( new HealthComponent());
	chit->Add( new DebugStateComponent( context->worldMap ));
	return chit;
}


Chit* LumosChitBag::NewVisitor( int visitorIndex )
{
	const ChitContext* context = GetContext();
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "visitor" );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( rootItem.ResourceName() ));

	AIComponent* ai = new AIComponent();
	chit->Add( ai );
	GLASSERT( visitorIndex >= 0 && visitorIndex < Visitors::NUM_VISITORS );
	ai->SetVisitorIndex( visitorIndex );
	Visitors::Instance()->visitorData[visitorIndex].Connect();	// initialize.

	// Visitors start at world center, with gridMove, and go from there.
	Vector3F pos = { (float)context->worldMap->Width()*0.5f, 0.0f, (float)context->worldMap->Height()*0.5f };
	chit->GetSpatialComponent()->SetPosition( pos );

	GridMoveComponent* gmc = new GridMoveComponent();
	chit->Add( gmc );

	SectorPort sp = context->worldMap->RandomPort( &random );
	gmc->SetDest( sp );

	AddItem( rootItem.Name(), chit, context->engine, TEAM_VISITOR, 0 );
	chit->Add( new HealthComponent());
	chit->Add( new VisitorStateComponent());
	return chit;
}


Chit* LumosChitBag::QueryRemovable( const grinliz::Vector2I& pos2i, bool ignorePlants )
{
	Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

	CChitArray		array;
	RemovableFilter removableFilter;
	BuildingFilter  buildingFilter;

	IChitAccept* filter = &removableFilter;
	if ( ignorePlants ) {
		filter = &buildingFilter;
	}

	QuerySpatialHash( &array, pos2, 0.1f, 0, filter );

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


Chit* LumosChitBag::QueryPlant( const grinliz::Vector2I& pos, int* type, int* stage )
{
	CChitArray plants;
	PlantFilter plantFilter;
	QuerySpatialHash( &plants, ToWorld2F( pos ), 0.2f, 0, &plantFilter );

	if ( plants.Size() ) {
		if ( PlantScript::IsPlant( plants[0], type, stage )) {
			return plants[0];
		}
	}
	return 0;
}


Chit* LumosChitBag::QueryBuilding( const grinliz::Rectangle2I& bounds )
{
	GLASSERT( MAX_BUILDING_SIZE == 2 );	// else adjust logic
	Vector2I sector = ToSector( bounds.min );

	for( MapSpatialComponent* it = mapSpatialHash[SectorIndex(sector)]; it; it = it->nextBuilding ) {
		if ( it->Bounds().Intersect( bounds )) {
			GLASSERT( it->Building() );
			return it->ParentChit();
		}
	}
	return 0;
}


Chit* LumosChitBag::QueryPorch( const grinliz::Vector2I& pos )
{
	Vector2I sector = ToSector(pos);
	for( MapSpatialComponent* it = mapSpatialHash[SectorIndex(sector)]; it; it = it->nextBuilding ) {
		if ( it->PorchPos().Contains( pos )) {
			return it->ParentChit();
		}
	}
	return 0;
}


Chit* LumosChitBag::NewGoldChit( const grinliz::Vector3F& pos, int amount )
{
	if ( !amount )
		return 0;
	const ChitContext* context = GetContext();

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
		AddItem( "gold", chit, context->engine, 0, 0 );
		chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
		chit->GetSpatialComponent()->SetPosition( pos );

	}
	chit->GetItem()->wallet.AddGold( amount );
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

	const ChitContext* context = GetContext();

	Chit* chit = this->NewChit();
	chit->Add( new SpatialComponent());
	AddItem( name, chit, context->engine, 0, 0 );
	chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
	chit->GetSpatialComponent()->SetPosition( pos );
	
	Wallet w;
	w.AddCrystal( crystal );
	chit->GetItem()->wallet.Add( w );

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
	GLASSERT( !orphanItem->IResourceName().empty() );

	Vector3F pos = _pos;
	if ( fuzz ) {
		pos.x = floorf(pos.x) + random.Uniform();
		pos.z = floorf(pos.z) + random.Uniform();
	}

	Chit* chit = this->NewChit();
	chit->Add( new SpatialComponent());
	chit->Add( new ItemComponent( orphanItem ));
	chit->Add( new RenderComponent( orphanItem->ResourceName() ));

	if ( onGround ) {
		const ModelResource* res = chit->GetRenderComponent()->MainResource();
		Rectangle3F aabb = res->AABB();
		pos.y = -aabb.min.y;

		if ( aabb.SizeY() < 0.1f ) {
			pos.y += 0.1f;
		}
	}
	chit->GetSpatialComponent()->SetPosition( pos );
//	chit->GetItemComponent()->SetHardpoints(); automatic
	return chit;
}


void LumosChitBag::HandleBolt( const Bolt& bolt, const ModelVoxel& mv )
{
	const ChitContext* context = GetContext();
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
			else if ( mv.VoxelHit() ) {
				context->worldMap->VoxelHit( mv.voxel, dd );
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
		BattleMechanics::GenerateExplosionMsgs( dd, origin, bolt.chitID, context->engine, this );

		if ( mv.VoxelHit() ) {
			context->worldMap->VoxelHit( mv.voxel, dd );
		}
	}
}


void LumosChitBag::AddItem( const char* name, Chit* chit, Engine* engine, int team, int level, const char* altRes )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get( name, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	GameItem item = *(itemDefArr[0]);
	if ( altRes ) {
		item.SetResource( altRes );
	}
	item.primaryTeam = team;
	item.GetTraitsMutable()->SetExpFromLevel( level );
	item.InitState();

	if ( !chit->GetItemComponent() ) {
		ItemComponent* ic = new ItemComponent( item );
		chit->Add( ic );
		for( int i=1; i<itemDefArr.Size(); ++i ) {
			ic->AddToInventory( new GameItem( *(itemDefArr[i] )));
		}
	}
	else {
		GLASSERT( itemDefArr.Size() == 1 );
		chit->GetItemComponent()->AddToInventory( new GameItem( item) );
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


CoreScript* LumosChitBag::GetCore( const grinliz::Vector2I& sector )
{
	const ChitContext* context = GetContext();	
	const SectorData& sd = context->worldMap->GetSector( sector );
	if ( sd.HasCore() ) {
		int index = sector.y * NUM_SECTORS + sector.x;
		if ( !coreCache[index] ) {
			Vector2I pos2i = sd.core;
			Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

			CChitArray array;
			ItemNameFilter coreFilter( IStringConst::core );
			QuerySpatialHash( &array, pos2, 0.1f, 0, &coreFilter );
			GLASSERT( !array.Empty() );

			if ( !array.Empty() ) {
				coreCache[index] = array[0];
			}
		}
		Chit* cc = coreCache[index];
		ScriptComponent* sc = cc->GetScriptComponent();
		GLASSERT( sc );
		IScript* script = sc->Script();
		GLASSERT( script );
		CoreScript* coreScript = script->ToCoreScript();
		GLASSERT( coreScript );
		return coreScript;
	}
	return 0;
}


bool ItemFlagFilter::Accept( Chit* chit )
{
	GameItem* item = chit->GetItem();
	if ( !item ) return false;

	if ( ( (item->flags & required) == required ) &&
		 ( (item->flags & excluded) == 0 ))
	{
		return true;
	}
	return false;
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
			const IString& key = item->IName();
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
	if ( msc && msc->Building() ) {
		return true;
	}
	return false;
}


bool MoBFilter::Accept( Chit* chit )
{
	return chit && chit->GetItem() && chit->GetAIComponent();
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


bool LootFilter::Accept( Chit* chit )
{
	/*
		Currently weapons and shields. Could be
		expanded to other sorts of things when
		they become available. (Keys? Decoration?)
	*/
	const GameItem* item = chit->GetItem();
	if ( item ) {
		if (    item->ToMeleeWeapon()
			|| item->ToRangedWeapon()
			|| item->ToShield() ) 
		{
			return true;
		}
	}
	return  false;
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


void LumosChitBag::PushScene( int id, SceneData* data )
{
	if ( sceneID >= 0 ) {
		GLLOG(( "Dupe scene pushed.\n" ));
		delete data;
	}
	else {
		sceneID = id;
		sceneData = data;
	}
}


bool LumosChitBag::PopScene( int* id, SceneData** data )
{
	if ( sceneID >= 0 ) {
		*id = sceneID;
		*data = sceneData;
		sceneID = -1;
		sceneData = 0;
		return true;
	}
	return false;
}

