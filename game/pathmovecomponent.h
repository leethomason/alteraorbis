#ifndef PATH_MOVE_COMPONENT_INCLUDED
#define PATH_MOVE_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../grinliz/glvector.h"
#include "gamelimits.h"

class WorldMap;


/*	Move along a path.
	
	Future note: Pathing should be async and done by message passing.
*/
class PathMoveComponent : public MoveComponent
{
public:
	PathMoveComponent( WorldMap* _map ) : map( _map ), nPath( 0 ), pos( 0 ), rotationFirst(true) {}
	virtual ~PathMoveComponent() {}

	const char* Name() const { return "PathMoveComponent"; }

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual bool NeedsTick()					{ return true; }
	virtual void DoTick( U32 delta );

	void SetDest( const grinliz::Vector2F& dest );
	// Set whether rotation is prioritized over movement. (Default
	// to true.) If false, then motion will happen and the rotation
	// is set from the motion.
	void SetRotationFirst( bool r ) { rotationFirst = r; }

private:
	float Travel( float rate, U32 time ) const {
		return rate * ((float)time) * 0.001f;
	}
	
	void GetPosRot( grinliz::Vector2F* pos, float* rot );
	void SetPosRot( grinliz::Vector2F& pos, float rot );
	
	void MoveFirst( U32 delta );
	void RotationFirst( U32 delta );

	WorldMap* map;
	int nPath;
	int pos;
	grinliz::Vector2F dest;
	bool rotationFirst;

	grinliz::Vector2F path[MAX_MOVE_PATH];
};

#endif // PATH_MOVE_COMPONENT_INCLUDED
