#ifndef GRID_MOVE_COMPONENT_INCLUDED
#define GRID_MOVE_COMPONENT_INCLUDED

#include "gamemovecomponent.h"
#include "worldinfo.h"

class GridMoveComponent : public GameMoveComponent
{
private:
	typedef GameMoveComponent super;
public:
	GridMoveComponent( WorldMap* );
	virtual ~GridMoveComponent();

	virtual const char* Name() const { return "GridMoveComponent"; }
	virtual void DebugStr( grinliz::GLString* str );

	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual int DoTick( U32 delta, U32 since );
	virtual bool IsMoving() const;
	virtual void CalcVelocity( grinliz::Vector3F* v ) { *v = velocity; }

	void SetDest( int sectorX, int sectorY, int port );
	// Always does; may need to restore as an option.
	//void DeleteAndRestorePathMCWhenDone( bool _deleteWhenDone )			{ deleteWhenDone = _deleteWhenDone; }

private:
	enum {
		NOT_INIT,
		ON_BOARD,
		TRAVELLING,
		OFF_BOARD,
		DONE
	};
	WorldMap*			worldMap;
	int					state;
	grinliz::Vector2I	sectorDest;
	int					portDest;
	//bool				deleteWhenDone;
	float				speed;

	// Not serialized:
	grinliz::Vector3F velocity;
	grinliz::CDynArray< GridEdge > path;
};

#endif // GRID_MOVE_COMPONENT_INCLUDED
