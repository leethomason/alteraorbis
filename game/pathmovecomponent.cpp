/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pathmovecomponent.h"
#include "worldmap.h"
#include "worldgrid.h"
#include "gamelimits.h"
#include "gridmovecomponent.h"
#include "lumoschitbag.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"

#include "../engine/loosequadtree.h"

#include "../grinliz/glperformance.h"

#include "../script/worldgen.h"

#include <cstring>
#include <cmath>

using namespace grinliz;

//#define DEBUG_PMC

void PathMoveComponent::Serialize( XStream* item )
{
	this->BeginSerialize( item, Name() );
	XARC_SER( item, queued.pos );
	XARC_SER( item, queued.rotation );
	XARC_SER( item, queued.sectorPort.sector );
	XARC_SER( item, queued.sectorPort.port );
	XARC_SER( item, dest.pos );
	XARC_SER( item, dest.rotation );
	XARC_SER( item, dest.sectorPort.sector );
	XARC_SER( item, dest.sectorPort.port );
	this->EndSerialize( item );
}


void PathMoveComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	blockForceApplied = false;
	avoidForceApplied = false;
	forceCount = 0;
	
	// Serialization case:
	// if there is a queue location, use that, else use the current location.
	if ( queued.pos.x >= 0 ) {
		QueueDest( queued.pos, queued.rotation, &queued.sectorPort );
	}
	else if ( dest.pos.x >= 0 ) {
		QueueDest( dest.pos, dest.rotation, &dest.sectorPort );
	}
}


void PathMoveComponent::OnRemove()
{
	super::OnRemove();
}



void PathMoveComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}


float PathMoveComponent::Speed() const
{
	return MOVE_SPEED;
}


void PathMoveComponent::CalcVelocity( grinliz::Vector3F* v )
{
	v->Zero();
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( spatial && this->IsMoving() ) {
		*v = spatial->GetHeading() * MOVE_SPEED;
	}
}


void PathMoveComponent::QueueDest( grinliz::Vector2F d, float r, const SectorPort* sector )
{
	GLASSERT(  d.x >= 0 && d.y >= 0 );

	queued.pos = d;
	queued.rotation = r;
	queued.sectorPort.Zero();
	if ( sector ) {
		queued.sectorPort = *sector;
	}
}


void PathMoveComponent::QueueDest( Chit* target ) 
{
	SpatialComponent* targetSpatial = target->GetSpatialComponent();
	SpatialComponent* srcSpatial = parentChit->GetSpatialComponent();
	if ( targetSpatial && srcSpatial ) {
		Vector2F v = targetSpatial->GetPosition2D();
		Vector2F delta = v - srcSpatial->GetPosition2D();
		float angle = RotationXZDegrees( delta.x, delta.y );
		QueueDest( v, angle );
	}
}


bool PathMoveComponent::NeedComputeDest()
{
	if ( queued.pos.x < 0 ) 
		return false;

	// Optimize - can tweak existing path? 
	// - If there is a path, and this doesn't change it, do nothing.
	// - If there is a path, and this only changes the end rotation, just do that.
	// But if the force count is high, don't optimize. We may be stuck.
	if ( HasPath() && !ForceCountHigh() ) {
		static const float EPS = 0.01f;
		if ( dest.pos.Equal( queued.pos, EPS ) && Equal( dest.rotation, queued.rotation, EPS ) ) {
			queued.Clear();
			return false;
		}
		if ( dest.pos.Equal( queued.pos, EPS ) ) {
			dest.rotation = queued.rotation;
			queued.Clear();
			return false;
		}
	}
	return true;
}


void PathMoveComponent::ComputeDest()
{
	GRINLIZ_PERFTRACK;
	ComponentSet thisComp( parentChit, Chit::SPATIAL_BIT | Chit::RENDER_BIT | ComponentSet::IS_ALIVE );
	if ( !thisComp.okay )
		return;

	const Vector2F& posVec = thisComp.spatial->GetPosition2D();
	bool sameSector = true;
	if ( map->UsingSectors() ) {
		Vector2I v0 = SectorData::SectorID( posVec.x, posVec.y );
		Vector2I v1 = SectorData::SectorID( queued.pos.x, queued.pos.y );
		sameSector = (v0 == v1);
	}

	if (	!sameSector
	     || queued.pos.x <= 0 || queued.pos.y <= 0 
		 || queued.pos.x >= (float)map->Width() || queued.pos.y >= (float)map->Height() ) 
	{
		this->Clear();
		parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED ), this ); 
		return;
	}


	GLASSERT( queued.pos.x >= 0 && queued.pos.y >= 0 );
	dest = queued;
	GLASSERT( dest.pos.x >= 0 && dest.pos.y >= 0 );
	queued.Clear();
	nPathPos = 0;
	pathPos = 0;

	// Make sure the 'dest' is actually a point we can get to.
	float radius = thisComp.render->RadiusOfBase();
	Vector2F d = dest.pos;
	if ( map->ApplyBlockEffect( d, radius, &dest.pos ) ) {
#ifdef DEBUG_PMC
		GLOUTPUT(( "Dest adjusted. (%.1f,%.1f) -> (%.1f,%.1f)\n", d.x, d.y, dest.pos.x, dest.pos.y ));
#endif
	}

	float cost=0;
	bool okay = map->CalcPath( posVec, dest.pos, path, &nPathPos, MAX_MOVE_PATH, &cost, pathDebugging ); 
	if ( !okay ) {
		SetNoPath();
		parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED ), this ); 
	}
	else {
		GLASSERT( nPathPos > 0 );
		GLASSERT( pathPos == 0 );
		GLASSERT( dest.pos.x >= 0 && dest.pos.y >= 0 );
		// We are back on track:
		forceCount /= 2;
	}
	// If pos < nPathPos, then pathing happens!
}


void PathMoveComponent::GetPosRot( grinliz::Vector2F* pos, float* rot )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );
	const Vector3F& pos3 = spatial->GetPosition();
	pos->Set( pos3.x, pos3.z );
	GLASSERT( map->Bounds().Contains( (int)pos->x, (int)pos->y ));
	*rot = spatial->GetYRotation();
}


void PathMoveComponent::SetPosRot( grinliz::Vector2F pos, float rot )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	GLASSERT( spatial );

	Rectangle2F b;
	b.Set( 0, 0, (float)map->Width(), (float)map->Height() );
	b.Outset( -0.1f );
	pos.x = Clamp( pos.x, b.min.x, b.max.x );
	pos.y = Clamp( pos.y, b.min.y, b.max.y );

	spatial->SetPosition( pos.x, 0, pos.y );
	spatial->SetYRotation( rot );
}


float PathMoveComponent::GetDistToNext2( const Vector2F& current )
{
	float dx = current.x - path[pathPos].x;
	float dy = current.y - path[pathPos].y;
	return dx*dx + dy*dy;
}


void PathMoveComponent::RotationFirst( U32 delta )
{
	GRINLIZ_PERFTRACK;

	if ( pathPos < nPathPos ) {
		float travel    = Travel( MOVE_SPEED, delta );
		float travelRot	= Travel( ROTATION_SPEED, delta );

		while ( travel > 0 && travelRot > 0 && pathPos < nPathPos ) {
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

			float targetRot = RotationXZDegrees( delta.x, delta.y );

			float bias;
			float deltaRot = MinDeltaDegrees( rot, targetRot, &bias );

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
				//Vector2F norm = { sinf(ToRadian(rot)), cosf(ToRadian(rot)) };
				Vector2F norm = parentChit->GetSpatialComponent()->GetHeading2D();
				norm.Normalize();

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


bool PathMoveComponent::AvoidOthers( U32 delta )
{
	GRINLIZ_PERFTRACK;
	static const float PATH_AVOID_DISTANCE = MAX_BASE_RADIUS*2.0f;

	avoidForceApplied = false;
	bool squattingDest = false;

	RenderComponent* render = parentChit->GetRenderComponent();
	if ( !render ) return false;
	
	Rectangle2F bounds;
	bounds.Set( pos2.x-PATH_AVOID_DISTANCE, pos2.y-PATH_AVOID_DISTANCE, 
		        pos2.x+PATH_AVOID_DISTANCE, pos2.y+PATH_AVOID_DISTANCE );

	ChitHasMoveComponent hasMoveComponentFilter;
	GetChitBag()->QuerySpatialHash( &chitArr, bounds, parentChit, &hasMoveComponentFilter );

	if ( !chitArr.Empty() ) {
		Vector3F pos3    = { pos2.x, 0, pos2.y };
		float radius     = parentChit->GetRenderComponent()->RadiusOfBase();
		Vector3F avoid = { 0, 0 };
		static const Vector3F UP = { 0, 1, 0 };

		Vector3F wayPoint   = { path[pathPos].x, 0, path[pathPos].y };
		Vector3F destNormal = wayPoint - pos3;
		destNormal.SafeNormalize( 1, 0, 0 );

		for( int i=0; i<chitArr.Size(); ++i ) {
			Chit* chit = chitArr[i];
			GLASSERT( chit != parentChit );
			
			Vector3F itPos3 = chit->GetSpatialComponent()->GetPosition();
			float itRadius  = chit->GetRenderComponent()->RadiusOfBase(); 

			float d = (pos3-itPos3).Length();
			float r = radius + itRadius;

			if ( d < r ) {
				avoidForceApplied = true;

				// Move away from the centers so the bases don't overlap.
				Vector3F normal = pos3 - itPos3;
				normal.y = 0;
				normal.SafeNormalize( -destNormal.x, -destNormal.y, -destNormal.z );
				float alignment = DotProduct( -normal, destNormal ); // how "in the way" is this?
					
				// Is this guy squatting on our dest?
				// This check is surprisingly broad, but keeps everyone from "orbiting"
				if ( (wayPoint-itPos3).LengthSquared() < r*r ) {
					// Dang squatter.
					if ( pathPos < nPathPos-1 ) {
						++pathPos;	// go around
					}
					else {
						squattingDest = true;
					}
				}
				// Not getting stuck forever is very important. It breaks
				// pathing where a CalcPath() is expected to get there.
				// Limiting the magnitute to a fraction of the travel 
				// speed avoids deadlocks.
				float mag = Min( r-d, 0.5f * Travel( MOVE_SPEED, delta ) ); 
				normal.Multiply( mag );
				avoid += normal;
				
				// Apply a sidestep vector so they don't just push.
				if ( alignment > 0.7f ) {
					Vector3F right;
					CrossProduct( destNormal, UP, &right );
					avoid += right * (0.5f*mag );
				}	
			}
		}
		avoid.y = 0;	// be sure.
		pos2.x += avoid.x;
		pos2.y += avoid.z;
	}
	return squattingDest;
}


int PathMoveComponent::DoTick( U32 delta, U32 since )
{
	GRINLIZ_PERFTRACK;

	int id = parentChit->ID();

	if ( NeedComputeDest() ) {
		pathPos = nPathPos = 0;	// clear the old path.
		ComputeDest();			// ComputeDest can fail, send message, then cause re-queue
	}

	blockForceApplied = false;
	avoidForceApplied = false;
	isMoving = false;
	SectorPort portJump;

	if ( HasPath() ) {
		// We should be doing something!
		bool squattingDest = false;
		int startPathPos = pathPos;
		float distToNext2 = GetDistToNext2( pos2 );

		if ( pathPos == nPathPos && dest.rotation >= 0 ) {
			// is rotation moving? Should isMoving be set?
			// Apply final rotation if the path is complete.
			GetPosRot( &pos2, &rot );

			float bias;
			float deltaRot = MinDeltaDegrees( rot, dest.rotation, &bias );
			float travelRot	= Travel( ROTATION_SPEED, delta );
			if ( deltaRot <= travelRot ) {
				rot = dest.rotation;
				dest.rotation = -1.f;
			}
			else {
				rot = NormalizeAngleDegrees( rot + bias*travelRot );
			}
			SetPosRot( pos2, rot );
		}
		else if ( pathPos < nPathPos ) {
			// Move down the path.
			isMoving = true;
			GetPosRot( &pos2, &rot );

			RotationFirst( delta );

			squattingDest = AvoidOthers( delta );
			ApplyBlocks( &pos2, &this->blockForceApplied );
			SetPosRot( pos2, rot );
		}

		// Position set: nothing below can change.
		if ( squattingDest ) {
			GLASSERT( pathPos >= nPathPos-1 );
			pathPos = nPathPos;
		}
		// Are we at the end of the path data?

		bool portFound = false;
		if ( dest.sectorPort.IsValid() ) {
			Vector2I pos2i = { (int)pos2.x, (int)pos2.y };

			// Sector and Port of departure:
			Vector2I departureSector = { pos2i.x/SECTOR_SIZE, pos2i.y/SECTOR_SIZE };
			const SectorData& sd = map->GetWorldInfo().GetSector( departureSector );
			int departurePort = sd.NearestPort( dest.pos );

			Rectangle2I portBounds = sd.GetPortLoc( departurePort );
			if ( portBounds.Contains( pos2i )) {
				// Close enough. Too much jugging on the ports is a real hassle.
				portFound = true;
				portJump = dest.sectorPort;
				parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_REACHED), this );
				SetNoPath();
			}

		}

		if ( !portFound ) {
			if (    ( pathPos == nPathPos )
				 && ( dest.rotation < 0 || Equal( dest.rotation, rot, 0.01f ))) 
			{
				if ( squattingDest || dest.pos.Equal( path[nPathPos-1], parentChit->GetRenderComponent()->RadiusOfBase()*0.2f ) ) {
	#ifdef DEBUG_PMC
					GLOUTPUT(( "Dest reached. squatted=%s\n", squattingDest ? "true" : "false" ));
	#endif
					// actually reached the end!
					parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_REACHED), this );
					SetNoPath();
				}
				else {
					// continue path:
					GLASSERT( dest.pos.x >= 0 );
					QueueDest( dest.pos, dest.rotation );
				}
			}
			else if ( forceCount > FORCE_COUNT_EXCESSIVE ) {
	#ifdef DEBUG_PMC
				GLOUTPUT(( "Block force excessive. forceCount=%d\n", forceCount ));
	#endif
				SetNoPath();
				parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED), this );

				//NewsEvent news( NewsEvent::PONY, pos2, StringPool::Intern( "FC Exec" ), parentChit->ID() );
				//GetChitBag()->AddNews( news );
				forceCount = 0;
			}
		}
	}
	else {
		GetPosRot( &pos2, &rot );
		AvoidOthers( delta );
		ApplyBlocks( &pos2, &this->blockForceApplied );
		SetPosRot( pos2, rot );
	};

	if ( portJump.IsValid() ) {
		GridMoveComponent* gmc = new GridMoveComponent( map );
		gmc->SetDest( portJump );
		parentChit->Swap( this, gmc );
		return 0;
	}

	if ( blockForceApplied /*|| avoidForceApplied*/ ) {
		++forceCount;
	}
	else {
		if ( forceCount > 0 ) 
			--forceCount;
	}
	return 0;
}


void PathMoveComponent::DebugStr( grinliz::GLString* str )
{
	if ( pathPos < nPathPos ) {
		str->Format( "[PathMove]=%.1f,%.1f ", dest.pos.x, dest.pos.y );
	}
	else {
		str->Format( "[PathMove]=still " );
	}
}
