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


//#define DEBUG_EXPLOSION

using namespace grinliz;

LumosChitBag::LumosChitBag() : engine( 0 )
{
}


LumosChitBag::~LumosChitBag()
{
	// Call the parent function so that chits aren't 
	// aren't using deleted memory.
	DeleteChits();
}



Chit* LumosChitBag::NewBuilding( const Vector2I& pos, const char* name, int team )
{
	Chit* chit = NewChit();

	const GameItem& rootItem = ItemDefDB::Instance()->Get( name );

	MapSpatialComponent* msc = new MapSpatialComponent( worldMap );
	msc->SetMapPosition( pos.x, pos.y );
	msc->SetMode( GRID_BLOCKED );
	msc->SetBuilding( true );
	
	chit->Add( msc );
	chit->Add( new RenderComponent( engine, rootItem.ResourceName() ));
	chit->Add( new HealthComponent( engine ));
	AddItem( name, chit, engine, team, 0 );

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
	chit->Add( new RenderComponent( engine, name ));
	chit->Add( new PathMoveComponent( worldMap ));
	chit->Add( new AIComponent( engine, worldMap ));

	chit->GetSpatialComponent()->SetPosition( pos );

	AddItem( name, chit, engine, team, 0 );
	chit->GetItemComponent()->AddGold( ReserveBank::Instance()->WithdrawMonster() );
	// Assume all AIs pick up gold, for now.
	chit->GetItemComponent()->SetPickup( ItemComponent::GOLD_PICKUP );

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


bool LumosChitBag::GoldFilter( Chit* chit )
{
	return ( chit->GetItem() && chit->GetItem()->name == IStringConst::kgold );
}


bool LumosChitBag::GoldCrystalFilter( Chit* chit )
{
	return (    chit->GetItem() && chit->GetItem()->name == IStringConst::kgold
			 || chit->GetItem() && chit->GetItem()->name == IStringConst::kcrystal_green
		     || chit->GetItem() && chit->GetItem()->name == IStringConst::kcrystal_red
		     || chit->GetItem() && chit->GetItem()->name == IStringConst::kcrystal_blue
			 || chit->GetItem() && chit->GetItem()->name == IStringConst::kcrystal_violet );
}


bool LumosChitBag::WorkerFilter( Chit* chit )
{
	GameItem* item = chit->GetItem();
	if ( item && (item->flags & GameItem::AI_DOES_WORK) ) {
		return true;
	}
	return false;
}


bool LumosChitBag::RemovableFilter( Chit* chit )
{
	return BuildingFilter( chit ) || PlantScript::PlantFilter( chit );
}

bool LumosChitBag::BuildingFilter( Chit* chit )
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	return msc && msc->Building();
}


bool LumosChitBag::BuildingWithPorchFilter( Chit* chit )
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	const GameItem* item = chit->GetItem();
	int porch = 0;
	if ( item ) {
		item->GetValue( "porch", &porch );
	}
	return msc && msc->Building() && porch;
}


bool LumosChitBag::KioskFilter( Chit* chit )
{
	if ( BuildingWithPorchFilter( chit ) ) {
		if ( chit->GetItem()->name == "kiosk.m" ) {
			return true;
		}
	}
	return false;
}

Chit* LumosChitBag::QueryBuilding( const grinliz::Vector2I& pos2i, bool orPorch )
{
	// Building can be up to MAX_BUILDING_SIZE
	float rad = 0.1f + (float)MAX_BUILDING_SIZE * 0.5f;	// actually a little big.
	if ( orPorch ) {
		rad += 1.0f;
	}
	Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

	Chit* porch = 0;
	CChitArray arr;

	this->QuerySpatialHash( &arr, pos2, rad, 0, BuildingFilter );
	for( int i=0; i<arr.Size(); ++i ) {
		Chit* chit = arr[i];
		MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
		GLASSERT( msc );	// all buildings should be msc's
		if ( msc ) {
			if ( msc->Bounds().Contains( pos2i )) {
				return chit;
			}
			if ( orPorch && !porch && msc->PorchPos() == pos2i ) {
				porch = chit;
			}
		}
	}
	// If we didn't find a building, but did (and should) find a porch
	return porch;
}


bool LumosChitBag::CoreFilter( Chit* chit )
{
	return ( chit->GetItem() && chit->GetItem()->name == IStringConst::kcore );
}


Chit* LumosChitBag::NewGoldChit( const grinliz::Vector3F& pos, int amount )
{
	if ( !amount )
		return 0;

	Vector2F v2 = { pos.x, pos.z };
	this->QuerySpatialHash( &chitList, v2, 1.0f, 0, GoldFilter );
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
		v2.x += random.Uniform11() * 0.2f;
		v2.y += random.Uniform11() * 0.2f;
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
		BattleMechanics::GenerateExplosionMsgs( dd, origin, bolt.chitID, engine, &chitList );

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
	item.stats.SetExpFromLevel( level );
	item.InitState();

	if ( !chit->GetItemComponent() ) {
		ItemComponent* ic = new ItemComponent( engine, worldMap, item );
		chit->Add( ic );
		for( int i=1; i<itemDefArr.Size(); ++i ) {
			ic->AddToInventory( new GameItem( *(itemDefArr[i]) ), true );
		}
	}
	else {
		GLASSERT( itemDefArr.Size() == 1 );
		chit->GetItemComponent()->AddToInventory( new GameItem( item ), true );
	}
}


bool LumosChitBag::HasMapSpatialInUse( Chit* chit ) {
	MapSpatialComponent* ms = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	if ( ms ) {
		return true;
	}
	return false;
}


int LumosChitBag::MapGridUse( int x, int y )
{
	Rectangle2F r;
	r.min.x = (float)x;
	r.min.y = (float)y;
	r.max.x = r.min.x + 1.0f;
	r.max.y = r.min.y + 1.0f;

	QuerySpatialHash( &inUseArr, r, 0, HasMapSpatialInUse );
	int flags = 0;
	for( int i=0; i<inUseArr.Size(); ++i ) {
		MapSpatialComponent* ms = GET_SUB_COMPONENT( inUseArr[i], SpatialComponent, MapSpatialComponent );
		GLASSERT( ms );
		flags |= ms->Mode();
	}
	return flags;
}



CoreScript* LumosChitBag::IsBoundToCore( Chit* chit )
{
	if ( chit && chit->GetSpatialComponent() ) {
		Vector2F pos2 = chit->GetSpatialComponent()->GetPosition2D();

		CChitArray array;
		QuerySpatialHash( &array, pos2, 0.1f, 0, CoreFilter );
		if ( !array.Empty() ) {
			Chit* cc = array[0];
			ScriptComponent* sc = cc->GetScriptComponent();
			GLASSERT( sc );
			IScript* script = sc->Script();
			GLASSERT( script );
			CoreScript* coreScript = script->ToCoreScript();
			GLASSERT( coreScript );
			return coreScript->GetAttached() == chit ? coreScript : 0;
		}
	}
	return false;
}


CoreScript* LumosChitBag::GetCore( const grinliz::Vector2I& sector )
{
	const SectorData& sd = worldMap->GetSector( sector );
	if ( sd.HasCore() ) {
		Vector2I pos2i = sd.core;
		Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

		CChitArray array;
		QuerySpatialHash( &array, pos2, 0.1f, 0, CoreFilter );
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

