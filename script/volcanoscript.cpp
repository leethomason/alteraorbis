#include "volcanoscript.h"

#include "../engine/serialize.h"

#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

#include "../game/worldmap.h"
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
		GLOUTPUT(( "VolcanoScript::Init. pos=%d,%d\n", (int)pos.x, (int)pos.y ));
		worldMap->SetMagma( (int)pos.x, (int)pos.y, true );

		NewsEvent event = { pos, StringPool::Intern( "volcano", true ) };
		heap.chit->GetChitBag()->AddNews( event );
	}
}


void VolcanoScript::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( size );
	XE_ARCHIVE( maxSize );
}


void VolcanoScript::Serialize( const ScriptContext& ctx, XStream* xs )
{
	XarcOpen( xs, "VolcanoScript" );
	XARC_SER( xs, size );
	XARC_SER( xs, maxSize );
	XarcClose( xs );
}


void VolcanoScript::Load( const ScriptContext& ctx, const tinyxml2::XMLElement* child )
{
	size = maxSize = 0;
	GLASSERT( child );
	
	if ( child ) {
		Archive( 0, child );
	}
}


void VolcanoScript::Save( const ScriptContext& ctx, tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "VolcanoScript" );
	Archive( printer, 0 );
	printer->CloseElement();
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
					if ( g.IsLand() && !g.IsBlocked() ) {
						worldMap->SetRock( x, y, g.NominalRockHeight(), 0, false );
					}
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
			ctx.chit->QueueDelete();
		}
	}
	// Only need to be called back as often as it spreads,
	// but give a little more resolution for loading, etc.
	return SPREAD_RATE / 2 + ctx.chit->random.Rand( SPREAD_RATE / 4 );
}


