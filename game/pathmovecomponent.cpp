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
#include "../xegame/chitcontext.h"

#include "../engine/loosequadtree.h"

#include "../Shiny/include/Shiny.h"

#include "../script/worldgen.h"

#include <cstring>
#include <cmath>

using namespace grinliz;

//#define DEBUG_PMC

void PathMoveComponent::Serialize( XStream* item )
{
	this->BeginSerialize( item, Name() );
	XARC_SER( item, queued.pos );
	XARC_SER( item, queued.heading );
	XARC_SER( item, queued.sectorPort.sector );
	XARC_SER( item, queued.sectorPort.port );
	XARC_SER( item, dest.pos );
	XARC_SER( item, dest.heading );
	XARC_SER( item, dest.sectorPort.sector );
	XARC_SER( item, dest.sectorPort.port );
	this->EndSerialize( item );
}


void PathMoveComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );
	blockForceApplied = false;
	avoidForceApplied = false;
	forceCount = 0;
	path.Clear();

	// Serialization case:
	// If there is a queue location, use that, else use the current location.
	// But only do this if we are the main component.
	if (parentChit->GetMoveComponent() == this) {
		if (queued.pos.x > 0) {
			QueueDest(queued.pos, &queued.heading, &queued.sectorPort);
		}
		else if (dest.pos.x > 0) {
			QueueDest(dest.pos, &dest.heading, &dest.sectorPort);
		}
	}
}


void PathMoveComponent::OnRemove()
{
//	if ( !path.Empty() ) {
//		parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED ), this ); 
//	}
	super::OnRemove();
}



void PathMoveComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}


float GameMoveComponent::Speed() const
{
	float speed = DEFAULT_MOVE_SPEED;
	if ( parentChit->GetItem() ) {
		parentChit->GetItem()->keyValues.Get( ISC::speed, &speed );
	}
	return speed;
}


Vector3F PathMoveComponent::Velocity()
{
	Vector3F v = { 0, 0, 0 };
	if ( this->IsMoving() ) {
		v = parentChit->Heading() * Speed();
	}
	return v;
}


void PathMoveComponent::QueueDest( const grinliz::Vector2F& d, const grinliz::Vector2F* h, const SectorPort* sector )
{
	GLASSERT(  d.x > 0 && d.y > 0 );

	queued.pos = d;
	queued.heading.Zero();
	if ( h ) {
		queued.heading = *h;
	}
	queued.sectorPort.Zero();
	if ( sector ) {
		queued.sectorPort = *sector;
	}
	parentChit->SetTickNeeded();
}


void PathMoveComponent::QueueDest( Chit* target ) 
{
	Vector2F v = ToWorld2F(target->Position());
	Vector2F delta = v - ToWorld2F(parentChit->Position());
	delta.Normalize();
	QueueDest( v, &delta );

	parentChit->SetTickNeeded();
}


bool PathMoveComponent::NeedComputeDest()
{
	if ( queued.pos.IsZero() ) 
		return false;

	// Optimize - can tweak existing path? 
	// - If there is a path, and this doesn't change it, do nothing.
	// - If there is a path, and this only changes the end rotation, just do that.
	// But if the force count is high, don't optimize. We may be stuck.
	if ( !path.Empty() && !ForceCountHigh() ) {
		static const float EPS = 0.1f;
		if ( dest.pos.Equal( queued.pos, EPS ) ) {
			dest.heading = queued.heading;
			queued.Clear();
			return false;
		}
	}
	return true;
}


void PathMoveComponent::ComputeDest()
{
	//PROFILE_FUNC();
	const ChitContext* context = Context();

	const Vector2F& posVec = ToWorld2F(parentChit->Position());
	bool sameSector = true;
	if (context->worldMap->UsingSectors()) {
		Vector2I v0 = ToSector(posVec.x, posVec.y);
		Vector2I v1 = ToSector(queued.pos.x, queued.pos.y);
		sameSector = (v0 == v1);
	}

	if (	!sameSector
	     || queued.pos.x <= 0 || queued.pos.y <= 0 
		 || queued.pos.x >= (float)context->worldMap->Width() || queued.pos.y >= (float)context->worldMap->Height() ) 
	{
		this->Clear();
		parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED ), this ); 
		return;
	}

	GLASSERT( queued.pos.x >= 0 && queued.pos.y >= 0 );
	dest = queued;
	GLASSERT( dest.pos.x >= 0 && dest.pos.y >= 0 );
	queued.Clear();
	pathPos = 0;

	// Make sure the 'dest' is actually a point we can get to.
	float radius = parentChit->GetRenderComponent() ? parentChit->GetRenderComponent()->RadiusOfBase() : MAX_BASE_RADIUS;
	Vector2F d = dest.pos;
	if ( context->worldMap->ApplyBlockEffect( d, radius, WorldMap::BT_PASSABLE, &dest.pos ) ) {
#ifdef DEBUG_PMC
		GLOUTPUT(( "Dest adjusted. (%.1f,%.1f) -> (%.1f,%.1f)\n", d.x, d.y, dest.pos.x, dest.pos.y ));
#endif
	}

	float cost=0;
	bool okay = context->worldMap->CalcPath( posVec, dest.pos, &path, &cost, pathDebugging ); 
	if ( !okay ) {
		SetNoPath();
		parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED ), this ); 
	}
	else {
		GLASSERT( !path.Empty() );
		GLASSERT( pathPos == 0 );
		GLASSERT( dest.pos.x >= 0 && dest.pos.y >= 0 );
		// We are back on track:
		forceCount /= 2;
	}
	// If pos < nPathPos, then pathing happens!
}


void PathMoveComponent::GetPosRot( grinliz::Vector2F* pos, grinliz::Vector2F* heading )
{
	*pos = ToWorld2F(parentChit->Position());
	*heading = parentChit->Heading2D();
}


void PathMoveComponent::SetPosRot( const grinliz::Vector2F& _pos, const grinliz::Vector2F& heading )
{
	Vector2F pos = _pos;

	const ChitContext* context = Context();

	Rectangle2F b;
	b.Set( 0, 0, (float)context->worldMap->Width(), (float)context->worldMap->Height() );
	b.Outset( -0.1f );
	pos.x = Clamp( pos.x, b.min.x, b.max.x );
	pos.y = Clamp( pos.y, b.min.y, b.max.y );

	// FIXME: could be more efficient with half-angles and setting
	// the quaternion directly.
	float rotation = RotationXZDegrees( heading.x, heading.y );
	Vector3F pos3 = { pos.x, 0, pos.y };
	Quaternion q;
	q.FromAxisAngle( V3F_UP, rotation );
	
	parentChit->SetPosRot( pos3, q );
}


bool PathMoveComponent::ApplyRotation( float travelRot, const Vector2F& targetHeading, Vector2F* heading )
{

	float dot = DotProduct( targetHeading, *heading );
	if ( dot > 0.999f ) {
		*heading = targetHeading;
		return true;
	}

	Vector3F cross;
	CrossProduct( *heading, targetHeading, &cross );

	Matrix2 rot;
	float s = cross.z < 0.f ? -1.f : 1.f;	// don't use Sign(), because it can return 0
	rot.SetRotation( s * travelRot );
	*heading = rot * (*heading);

	float newDot = DotProduct( targetHeading, *heading );
	Vector3F newCross;
	CrossProduct( *heading, targetHeading, &newCross );

	if (    ( Sign(dot) == Sign(newDot) )
		 && ( Sign(cross.z) != Sign(newCross.z))) 
	{
		// over-rotated.
		*heading = targetHeading;
		return true;
	}
	return false;
}


bool PathMoveComponent::RotationFirst( U32 _dt, Vector2F* pos2, Vector2F* heading )
{
	//PROFILE_FUNC();

	float speed		= Speed();
	float dt		= float(_dt)*0.001f;

	while (    (dt > 0.0001 )
			&& (pathPos < path.Size()) ) 
	{
		float travel    = Travel( speed, dt );		// distance = speed * time, time = distance / speed
		float travelRot	= Travel( ROTATION_SPEED, dt );

		Vector2F next  = path[pathPos];		// next waypoint
		Vector2F delta = next - (*pos2);	// vector to next waypoint
		float    dist = delta.Length();		// distance to next waypoint
			
		// check for very close & patch.
		static const float EPS = 0.01f;
		if ( dist <= EPS ) {
			dt -= dist / speed;	
			*pos2 = next;
			++pathPos;
			continue;
		}

		// Do vector based motion.
		Vector2F targetHeading = delta;
		targetHeading.Normalize();

		// Rotation while moving takes no extra time.
		bool rotationDone = ApplyRotation( travelRot, targetHeading, heading );
		float dot = DotProduct( targetHeading, *heading );

		if ( dist <= travel ) {
			if ( dot > 0.5f ) {
				// we are close enough to hit dest. Patch.
				*pos2 = next;
				*heading = targetHeading;
				++pathPos;
				dt -= dist / speed;
				continue;
			}
		}

		// The right algorithm is tricky...tried
		// lots of approaches, regret not documenting them.
		// It's all about avoiding "orbiting" conditions.
		if (       rotationDone 
				|| ( dist > travel*12.0f && dot > 0.5f ))	// dist > travel*12 bit keeps from orbiting. 
		{												// dot > 0.5f makes the avatar turn in the generally correct direction before moving
			*pos2 += (*heading) * travel;
		}
		dt -= dist / speed;
	}

	bool done = pathPos == path.Size();
	if ( pathPos == path.Size() && !dest.heading.IsZero() ) {
		// Pure rotation:
		float travelRot	= Travel( ROTATION_SPEED, dt );
		done = ApplyRotation( travelRot, dest.heading, heading );
	}
	return done;
}


void PathMoveComponent::AvoidOthers(U32 delta, grinliz::Vector2F* pos2, grinliz::Vector2F* heading)
{
	//PROFILE_FUNC();

	static const float PATH_AVOID_DISTANCE = MAX_BASE_RADIUS*2.0f;

	avoidForceApplied = false;

	RenderComponent* render = parentChit->GetRenderComponent();
	if (!render) return;

	Rectangle2F bounds;
	bounds.min = bounds.max = *pos2;
	bounds.Outset(PATH_AVOID_DISTANCE);

	ChitHasMoveComponent hasMoveComponentFilter;
	CChitArray chitArr;
	Context()->chitBag->QuerySpatialHash(&chitArr, bounds, parentChit, &hasMoveComponentFilter);

	/* Push in a normal direction and slow down, but don't accelerate. */

	if (!chitArr.Empty()) {
		float radius = parentChit->GetRenderComponent()->RadiusOfBase();

		Vector2F avoid = { 0, 0 };

		for (int i = 0; i < chitArr.Size(); ++i) {
			Chit* chit = chitArr[i];
			GLASSERT(chit != parentChit);

			if (!chit->GetMoveComponent()->ShouldAvoid()) {
				continue;
			}

			Vector2F itPos2 = ToWorld2F(chit->Position());
			float itRadius = chit->GetRenderComponent()->RadiusOfBase();

			float d = ((*pos2) - itPos2).Length();
			float r = radius + itRadius;

			if (d < r) {
				avoidForceApplied = true;
				// Want the other to respond to force, so wake it up:
				chit->SetTickNeeded();

				// Move away from the centers so the bases don't overlap.
				Vector2F normal = *pos2 - itPos2;
				normal.Normalize();
				float alignment = DotProduct(-normal, *heading); // how "in the way" is this?

				// Not getting stuck forever is very important. It breaks
				// pathing where a CalcPath() is expected to get there.
				// Limiting the magnitute to a fraction of the travel 
				// speed avoids deadlocks.
				float mag = Min(r - d, 0.5f * Travel(Speed(), delta));

				//if ( alignment > 0 ) {
				normal.Multiply(mag);
				avoid += normal;
				//}

				// Apply a sidestep vector so they don't just push.
				if (alignment > 0.7f) {
					Vector2F right = { heading->x, -heading->y };
					avoid += right * (0.5f*mag);
				}
			}
		}
		*pos2 += avoid;
	}
}


int PathMoveComponent::DoTick( U32 delta )
{
	PROFILE_FUNC();

//	int id = parentChit->ID();
	const ChitContext* context = Context();

	if ( NeedComputeDest() ) {
		pathPos = 0;			// clear the old path.
		path.Clear();
		ComputeDest();			// ComputeDest can fail, send message, then cause re-queue
	}

	blockForceApplied = false;
	avoidForceApplied = false;
	isMoving = false;
	SectorPort portJump;
	int time = VERY_LONG_TICK;
	bool floating = false;

	// Start with the physics move:
	Vector3F pos3 = parentChit->Position();
	bool fluid = ApplyFluid(delta, &pos3, &floating );
	if (fluid) {
		parentChit->SetPosition(pos3);
		++forceCount;
		if (floating)
			return 0;
		time = 0;
	}


	Vector2F pos, heading;
	GetPosRot( &pos, &heading );

	if ( !path.Empty() ) {

		// We should be doing something!
		time = 0;
		isMoving = true;
		
		bool pathDone = RotationFirst( delta, &pos, &heading );

		bool portFound = false;
		if ( dest.sectorPort.IsValid() ) {
			Vector2I pos2i = { (int)pos.x, (int)pos.y };

			// Sector and Port of departure:
			Vector2I departureSector = ToSector(pos2i);
			const SectorData& sd = context->worldMap->GetWorldInfo().GetSector( departureSector );
			int departurePort = sd.NearestPort( dest.pos );

			Rectangle2I portBounds = sd.GetPortLoc( departurePort );
			if ( portBounds.Contains( pos2i )) {
				// Close enough. Too much jugging on the ports is a real hassle.
				portFound = true;
				portJump = dest.sectorPort;
				pathDone = true;
			}
		}
		if ( pathDone ) {
			parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_REACHED), this );
			SetNoPath();
		}

		if ( !portFound && forceCount > FORCE_COUNT_EXCESSIVE ) {
	#ifdef DEBUG_PMC
			GLOUTPUT(( "Block force excessive. forceCount=%d\n", forceCount ));
	#endif
			SetNoPath();
			parentChit->SendMessage( ChitMsg( ChitMsg::PATHMOVE_DESTINATION_BLOCKED), this );

			forceCount = 0;
		}
	}

	AvoidOthers( delta, &pos, &heading );
	ApplyBlocks( &pos, &this->blockForceApplied );

	if ( portJump.IsValid() ) {
		ChitMsg msg(ChitMsg::PATHMOVE_TO_GRIDMOVE, 0, &portJump);
		parentChit->SendMessage(msg, this);

		GridMoveComponent* gmc = new GridMoveComponent();
		gmc->SetDest( portJump );
		parentChit->Add(gmc);
		return 0;
	}

	if ( avoidForceApplied ) {
		time = 0;
		forceCount += 1;
	}
	if ( blockForceApplied ) {
		time = 0;
		forceCount += 2;
	}
	if ( !avoidForceApplied && !blockForceApplied && forceCount > 0 ) {
		--forceCount;
		time = 0;
	}

	SetPosRot( pos, heading );
	return time;
}


void PathMoveComponent::DebugStr( grinliz::GLString* str )
{
	if ( pathPos < path.Size() ) {
		str->AppendFormat( "[PathMove]=%d/%d ", pathPos, path.Size() );
	}
	else {
		str->AppendFormat( "[PathMove]=still " );
	}
}
