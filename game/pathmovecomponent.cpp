#include "pathmovecomponent.h"
#include "worldmap.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitbag.h"

#include "../engine/loosequadtree.h"

#include <cstring>
#include <cmath>

using namespace grinliz;

void PathMoveComponent::OnAdd( Chit* chit )
{
	MoveComponent::OnAdd( chit );
	nPath = pathPos = repath = 0;
	dest.Zero();
	blockForceApplied = false;
	avoidForceApplied = false;
	isStuck = false;
}


void PathMoveComponent::OnRemove()
{
	MoveComponent::OnRemove();
}


void PathMoveComponent::SetDest( const Vector2F& d )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );
	RenderComponent* render = parentChit->GetRenderComponent();
	GLASSERT( render );

	const Vector3F& posVec = spatial->GetPosition();

	Vector2F start = { posVec.x, posVec.z };
	dest = d;
	nPath = 0;
	pathPos = 0;

	// Make sure the 'dest' is actually a point we can get to.
	float radius = render->RadiusOfBase();
	if ( map->ApplyBlockEffect( d, radius, &dest ) ) {
		GLOUTPUT(( "Dest adjusted. (%.1f,%.1f) -> (%.1f,%.1f)\n", d.x, d.y, dest.x, dest.y ));
	}

	bool okay = map->CalcPath( start, dest, path, &nPath, MAX_MOVE_PATH, pathDebugging ); 
	if ( !okay ) {
		nPath = pathPos = 0;
		SendMessage( MSG_DESTINATION_BLOCKED );
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


void PathMoveComponent::SetPosRot( const grinliz::Vector2F& pos, float rot )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );
	spatial->SetPosition( pos.x, 0, pos.y );
	spatial->SetYRotation( rot );
}


float PathMoveComponent::GetDistToNext2( const Vector2F& current )
{
	float dx = current.x - path[pathPos].x;
	float dy = current.y - path[pathPos].y;
	return dx*dx + dy*dy;
}


void PathMoveComponent::MoveFirst( U32 delta )
{
	if ( pathPos < nPath ) {

		float travel = Travel( MOVE_SPEED, delta );
		Vector2F startingPos2 = pos2;

		while ( travel > 0 && pathPos < nPath ) {
			startingPos2 = pos2;

			float distToNext = (pos2-path[pathPos]).Length();
			if ( distToNext <= travel ) {
				// Move to the next waypoint
				travel -= distToNext;
				pos2 = path[pathPos];
				++pathPos;
			}
			else {
				pos2 = Lerp( pos2, path[pathPos], travel / distToNext );
				travel = 0;
			}
		}
		Vector2F delta = pos2 - startingPos2;
		if ( delta.LengthSquared() ) {
			rot = ToDegree( atan2f( delta.x, delta.y ) );
		}
	}
}


void PathMoveComponent::RotationFirst( U32 delta )
{
	if ( pathPos < nPath ) {
		float travel    = Travel( MOVE_SPEED, delta );
		float travelRot	= Travel( ROTATION_SPEED, delta );

		while ( travel > 0 && travelRot > 0 && pathPos < nPath ) {
			Vector2F next  = path[pathPos];
			Vector2F delta = next - pos2;
			float    dist = delta.Length();
			
			// check for very close & patch.
			static const float EPS = 0.01f;
			if ( dist <= EPS ) {
				travel -= dist;
				pos2 = next;
				++pathPos;
				continue;
			}

			float targetRot = NormalizeAngleDegrees( ToDegree( atan2f( delta.x, delta.y )));

			float deltaRot, bias;
			MinDeltaDegrees( rot, targetRot, &deltaRot, &bias );

			if ( travelRot > deltaRot ) {
				rot = targetRot;
				travelRot -= deltaRot;
			}
			else {
				rot += travelRot*bias;
				rot = NormalizeAngleDegrees( rot );
				travelRot = 0;
			}

			// We we don't chase the destination point, line
			// it up closer
			float rotationLimit = ROTATION_LIMIT;
			if ( dist < 1.0f ) {
				rotationLimit = Lerp( 0.0f, ROTATION_LIMIT, dist );
			}
			
			if ( deltaRot < rotationLimit ) {
				Vector2F norm = { sinf(ToRadian(rot)), cosf(ToRadian(rot)) };

				if ( dist <= travel ) {
					travel -= dist;
					pos2 = next;
					++pathPos;
				}
				else {
					pos2 += norm * travel;
					travel = 0;
				}
			}
		}
	}
}


#if 0
void PathMoveComponent::AvoidOthers( U32 delta )
{
	if ( !spaceTree ) return;
	RenderComponent* render = parentChit->GetRenderComponent();
	if ( !render ) return;
	if ( (render->GetFlags() & MODEL_USER_AVOIDS ) == 0 ) return;
	
	Rectangle3F bounds;
	bounds.Set( pos2.x-PATH_AVOID_DISTANCE, -0.1f, pos2.y-PATH_AVOID_DISTANCE, pos2.x+PATH_AVOID_DISTANCE, 0.1f, pos2.y+PATH_AVOID_DISTANCE );
	Model* root = spaceTree->Query( bounds, MODEL_USER_AVOIDS, 0 );

	if ( root && root->userData != parentChit ) {
		Vector3F heading = parentChit->GetSpatialComponent()->GetHeading();
		Vector2F heading2 = { heading.x, heading.z };
		Vector3F pos3    = parentChit->GetSpatialComponent()->GetPosition();
		Vector3F avoid = { 0, 0 };
		static const Vector3F UP = { 0, 1, 0 };

		while( root ) {
			Chit* chit = root->userData;
			if ( chit && chit != parentChit ) {

				if ( IntersectRayCircle( chit->GetSpatialComponent()->GetPosition2D(),
										 chit->GetRenderComponent()->RadiusOfBase(),
										 pos2, heading2 )
					== INTERSECT )
				{
					Vector3F delta = chit->GetSpatialComponent()->GetPosition() - pos3;
					Vector3F right;
					CrossProduct( delta, UP, &right );

					Vector3F cross;
					CrossProduct( heading, delta, &cross );
					avoid += ((cross.z >= 0) ? right : -right);
				}
			}
			root = root->next;
		}
		avoid.y = 0;	// be sure.
		if ( avoid.LengthSquared() > 1 )
			avoid.Normalize();
		avoid.Multiply( Travel( AVOID_SPEED_FRACTION*MOVE_SPEED, delta ));
		pos2.x += avoid.x;
		pos2.y += avoid.z;
	}
}
#endif


void PathMoveComponent::AvoidOthers( U32 delta )
{
	avoidForceApplied = false;

	if ( !spaceTree ) return;
	RenderComponent* render = parentChit->GetRenderComponent();
	if ( !render ) return;
	if ( (render->GetFlags() & MODEL_USER_AVOIDS ) == 0 ) return;
	
	Rectangle3F bounds;
	bounds.Set( pos2.x-PATH_AVOID_DISTANCE, -0.1f, pos2.y-PATH_AVOID_DISTANCE, pos2.x+PATH_AVOID_DISTANCE, 0.1f, pos2.y+PATH_AVOID_DISTANCE );
	Model* root = spaceTree->Query( bounds, MODEL_USER_AVOIDS, 0 );

	if ( root && root->userData != parentChit ) {
		Vector3F pos3    = { pos2.x, 0, pos2.y };
		float radius     = parentChit->GetRenderComponent()->RadiusOfBase();
		Vector3F avoid = { 0, 0 };
		static const Vector3F UP = { 0, 1, 0 };

		Vector3F wayPoint   = { path[pathPos].x, 0, path[pathPos].y };
		Vector3F destNormal = wayPoint - pos3;
		destNormal.Normalize();

		while( root ) {
			Chit* chit = root->userData;
			if ( chit && chit != parentChit ) {
				
				Vector3F itPos3 = chit->GetSpatialComponent()->GetPosition();
				float itRadius  = chit->GetRenderComponent()->RadiusOfBase(); 

				float d = (pos3-itPos3).Length();
				float r = radius + itRadius;

				if ( d < r ) {
					avoidForceApplied = true;

					// Move away from the centers so the bases don't overlap.
					Vector3F normal = pos3 - itPos3;
					normal.y = 0;
					normal.Normalize();
					float alignment = DotProduct( -normal, destNormal ); // how "in the way" is this?
					// Limit push-back so that the chit will eventually
					// get to the dest. Keeps dormant chits from squatting
					// the way points.
					float magnitude = r - d;
					magnitude = Min( magnitude, Travel( MOVE_SPEED, delta ) );
					normal.Multiply( r - d );
					avoid += normal;

					// Apply a sidestep vector so they don't just push.
					if ( alignment > 0.7f ) {
						Vector3F right;
						CrossProduct( destNormal, UP, &right );
						avoid += right * (0.5f * (r-d) );
					}	
				}
			}
			root = root->next;
		}
		avoid.y = 0;	// be sure.
		pos2.x += avoid.x;
		pos2.y += avoid.z;
	}
}


void PathMoveComponent::ApplyBlocks()
{
	RenderComponent* render = parentChit->GetRenderComponent();

	Vector2F newPos2 = pos2;
	float rotation = 0;
	float radius = render->RadiusOfBase();

	WorldMap::BlockResult result = map->ApplyBlockEffect( pos2, radius, &newPos2 );
	blockForceApplied = ( result == WorldMap::FORCE_APPLIED );
	isStuck = ( result == WorldMap::STUCK );
	
	pos2 = newPos2;
}


void PathMoveComponent::DoTick( U32 delta )
{
	if ( pathPos < nPath ) {
		GetPosRot( &pos2, &rot );
		int startPathPos = pathPos;
		float distToNext = GetDistToNext2( pos2 );

		if ( rotationFirst )
			RotationFirst( delta );
		else
			MoveFirst( delta );

		AvoidOthers( delta );
		ApplyBlocks();
		SetPosRot( pos2, rot );

		// Position set: nothing below can change.
		// Do we need to repath because we're stuck?
		if (    blockForceApplied  
			 && startPathPos == pathPos
			 && GetDistToNext2( pos2 ) >= distToNext ) 
		{ 
			++repath;
		}
		else {
			repath = 0;
		}
		if ( repath == 3 ) {
			GLOUTPUT(( "Repath\n" ));
			Vector2F d = dest;
			SetDest( d );
			repath = 0;
		}

		// Are we at the end of the path data?
		if ( pathPos == nPath ) {
			if ( dest.Equal( path[nPath-1], parentChit->GetRenderComponent()->RadiusOfBase() ) ) {
				// actually reached the end!
				SendMessage( MSG_DESTINATION_REACHED );
			}
			else {
				// continue path:
				Vector2F d = dest;
				SetDest( d );
			}
		}
	}
}
