#include "pathmovecomponent.h"
#include "worldmap.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

#include <cstring>

using namespace grinliz;

void PathMoveComponent::OnAdd( Chit* chit )
{
	MoveComponent::OnAdd( chit );
	nPath = pos = 0;
	dest.Zero();
}


void PathMoveComponent::OnRemove()
{
	MoveComponent::OnRemove();
}


void PathMoveComponent::SetDest( const Vector2F& d )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );
	const Vector3F& posVec = spatial->GetPosition();

	Vector2F start = { posVec.x, posVec.z };
	dest = d;
	nPath = 0;
	pos = 0;

	bool okay = map->CalcPath( start, dest, path, &nPath, MAX_MOVE_PATH, false ); 
	if ( !okay ) {
		nPath = pos = 0;
	}
	// If pos < nPath, then pathing happens!
}


void PathMoveComponent::GetPosRot( grinliz::Vector2F* pos, float* rot )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );
	const Vector3F& pos3 = spatial->GetPosition();
	pos->Set( pos3.x, pos3.z );
	*rot = spatial->GetYRotation();
}


void PathMoveComponent::SetPosRot( grinliz::Vector2F& pos, float rot )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );
	spatial->SetPosition( pos.x, 0, pos.y );
	spatial->SetYRotation( rot );
}


void PathMoveComponent::MoveFirst( U32 delta )
{
	if ( pos < nPath ) {

		float travel = Travel( MOVE_SPEED, delta );
		Vector2F pos2;
		float rot;
		GetPosRot( &pos2, &rot );
		Vector2F startingPos2 = pos2;

		while ( travel > 0 && pos < nPath ) {
			startingPos2 = pos2;

			float distToNext = (pos2-path[pos]).Length();
			if ( distToNext <= travel ) {
				// Move to the next waypoint
				travel -= distToNext;
				pos2 = path[pos];
				++pos;
			}
			else {
				pos2 = Lerp( pos2, path[pos], travel / distToNext );
				travel = 0;
			}
		}
		Vector2F delta = pos2 - startingPos2;
		if ( delta.LengthSquared() ) {
			rot = ToDegree( atan2f( delta.x, delta.y ) );
		}
		SetPosRot( pos2, rot );
	}
}


void PathMoveComponent::DoTick( U32 delta )
{
	MoveFirst( delta );
}
