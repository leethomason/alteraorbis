#ifndef PATH_MOVE_COMPONENT_INCLUDED
#define PATH_MOVE_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../grinliz/glvector.h"
#include "gamelimits.h"

class WorldMap;
class SpaceTree;

/*	Move along a path.
	Future note: Pathing should be async and done by message passing.
*/
class PathMoveComponent : public MoveComponent
{
public:
	enum {
		MSG_DESTINATION_REACHED,
		MSG_DESTINATION_BLOCKED
	};

	PathMoveComponent(	WorldMap* _map,				// required; used to avoids blocks when moving. 
						SpaceTree* _spaceTree )		// optional: will try to avoid collisions with other objects
		: map( _map ), spaceTree( _spaceTree ), nPathPos( 0 ), pathPos( 0 ), rotationFirst(true), pathDebugging( false ) {}
	virtual ~PathMoveComponent() {}

	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "PathMoveComponent" ) ) return this;
		return MoveComponent::ToComponent( name );
	}

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual bool NeedsTick()					{ return true; }
	virtual void DoTick( U32 delta );

	void SetDest( const grinliz::Vector2F& dest );
	void SetDest( float x, float y )	{ grinliz::Vector2F v = { x, y }; SetDest( v ); }

	// Set whether rotation is prioritized over movement. (Default
	// to true.) If false, then motion will happen and the rotation
	// is set from the motion.
	void SetRotationFirst( bool r ) { rotationFirst = r; }
	void SetPathDebugging( bool d )	{ pathDebugging = d; }

	// Status info
	int BlockForceApplied() const	{ return blockForceApplied; }
	bool IsStuck() const			{ return isStuck; }
	bool IsMoving() const			{ return pathPos < nPathPos; }
	bool IsAvoiding() const			{ return avoidForceApplied; }

private:
	float Travel( float rate, U32 time ) const {
		return rate * ((float)time) * 0.001f;
	}
	
	void GetPosRot( grinliz::Vector2F* pos, float* rot );
	void SetPosRot( const grinliz::Vector2F& pos, float rot );
	float GetDistToNext2( const grinliz::Vector2F& currentPos );

	// Move, then set rotation from movement.
	void MoveFirst( U32 delta );
	// Rotate, then move in that direction.
	void RotationFirst( U32 delta );
	// Try to avoid walking through others.
	// Returns 'true' if the destination is being squatted.
	bool AvoidOthers( U32 delta );
	// Keep from hitting world objects.
	void ApplyBlocks();

	WorldMap*	map;
	SpaceTree*	spaceTree;
	int nPathPos;				// size of path
	int pathPos;			// index of where we are on path
	int repath;				// counter to see if we are stuck
	grinliz::Vector2F dest;	// final destination

	grinliz::Vector2F pos2;		// only valid during tick!
	float    rot;				// only valid during tick!

	bool rotationFirst;
	bool pathDebugging;

	bool blockForceApplied;		
	bool avoidForceApplied;
	bool isStuck;

	grinliz::Vector2F path[MAX_MOVE_PATH];
};

#endif // PATH_MOVE_COMPONENT_INCLUDED
