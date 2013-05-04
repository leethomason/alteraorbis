#include "volcanoscript.h"

#include "../engine/serialize.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

#include "../game/worldmap.h"
#include "../game/worldinfo.h"
#include "../game/reservebank.h"
#include "../game/lumoschitbag.h"

#include "../xarchive/glstreamer.h"

using namespace grinliz;
using namespace tinyxml2;

static const U32 SPREAD_RATE = 4000;

VolcanoScript::VolcanoScript( WorldMap* p_map, int p_size )
{
	worldMap = p_map;
	maxSize = p_size;
	size = 0;
}


void VolcanoScript::Init( const ScriptContext& heap )
{
	SpatialComponent* sc = heap.chit->GetSpatialComponent();
	GLASSERT( sc );
	if ( sc ) {
		Vector2F pos = sc->GetPosition2D();
		//GLOUTPUT(( "VolcanoScript::Init. pos=%d,%d\n", (int)pos.x, (int)pos.y ));
		worldMap->SetMagma( (int)pos.x, (int)pos.y, true );

		NewsEvent event( 0, pos, StringPool::Intern( "volcano", true ));
		heap.chit->GetChitBag()->AddNews( event );
	}
}


void VolcanoScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, "VolcanoScript" );
	XARC_SER( xs, size );
	XARC_SER( xs, maxSize );
	XarcClose( xs );
}


int VolcanoScript::DoTick( const ScriptContext& ctx, U32 delta, U32 since )
{
	SpatialComponent* sc = ctx.chit->GetSpatialComponent();
	Vector2I pos = { 0,  0 };
	GLASSERT( sc );
	if ( sc ) {
		Vector2F posF = sc->GetPosition2D();
		pos.Set( (int)posF.x, (int)posF.y );
	}
	else {
		ctx.chit->QueueDelete();
	}

	Rectangle2I b = worldMap->Bounds();
	int rad = ctx.time / SPREAD_RATE;
	if ( rad > size ) {
		// Cool (and set) the inner rectangle, make the new rectangle magma.
		// The origin stays magma until we're done.
		size = Min( rad-1, maxSize );
		
		Rectangle2I r;
		r.min = r.max = pos;
		r.Outset( size );

		for( int y=r.min.y; y<=r.max.y; ++y ) {
			for( int x=r.min.x; x<=r.max.x; ++x ) {
				if ( b.Contains( x, y ) ) {
					worldMap->SetMagma( x, y, false );
					const WorldGrid& g = worldMap->GetWorldGrid( x, y );
					// Does lots of error checking. Can set without checks:
					worldMap->SetRock( x, y, g.NominalRockHeight(), 0, false );
				}
			}
		}

		size = rad;

		if ( rad < maxSize ) {
			worldMap->SetMagma( pos.x, pos.y, true );
			for( int y=r.min.y; y<=r.max.y; ++y ) {
				for( int x=r.min.x; x<=r.max.x; ++x ) {
					if ( b.Contains( x, y )) {
						if ( y == r.min.y || y == r.max.y || x == r.min.x || x == r.max.x ) {
							worldMap->SetMagma( x, y, true );
						}
					}
				}
			}
		}
		else {
			// Distribute gold and crystal.
			Vector2I sector = { pos.x/SECTOR_SIZE, pos.y/SECTOR_SIZE };
			const SectorData& sd = worldMap->GetWorldInfo().GetSector( sector );
			if ( sd.ports ) {
				for( int i=0; i<4; ++i ) {
					int x = r.min.x + ctx.chit->random.Rand( r.Width() );
					int y = r.min.y + ctx.chit->random.Rand( r.Height() );
					if (    worldMap->GetWorldGrid( x, y ).RockHeight()
						 || worldMap->GetWorldGrid( x, y ).PoolHeight()) 
					{
						int gold = ReserveBank::Instance()->WithdrawVolcanoGold();
						Vector3F v3 = { (float)x+0.5f, 0, (float)y+0.5f };
						ctx.chit->GetLumosChitBag()->NewGoldChit( v3, gold );
					}
				}
				if (    worldMap->GetWorldGrid( pos.x, pos.y ).RockHeight()
					 || worldMap->GetWorldGrid( pos.x, pos.y ).PoolHeight()) 
				{	
					int gold = 0;
					int crystal = NO_CRYSTAL;
					Vector3F v3 = { (float)pos.x+0.5f, 0, (float)pos.y+0.5f };
					Wallet wallet = ReserveBank::Instance()->WithdrawVolcano();
					ctx.chit->GetLumosChitBag()->NewWalletChits( v3, wallet );
				}
			}
			ctx.chit->QueueDelete();
		}
	}
	// Only need to be called back as often as it spreads,
	// but give a little more resolution for loading, etc.
	return SPREAD_RATE / 2 + ctx.chit->random.Rand( SPREAD_RATE / 4 );
}


