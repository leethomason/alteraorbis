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
	virtual void CalcVelocity( grinliz::Vector3F* v ) { v->Zero(); }	// FIXME

	void SetDest( int sectorX, int sectorY, int port );
	void DeleteWhenDone( bool _deleteWhenDone )			{ deleteWhenDone = _deleteWhenDone; }

private:
	GridEdge ToGridEdge( int sectorX, int sectorY, int port );

	enum {
		NOT_INIT,
		ON_BOARD,
		TRAVELLING,
		OFF_BOARD,
		DONE
	};
	WorldMap*			worldMap;
	int					state;
	GridEdge			current;
	grinliz::Vector2I	sectorDest;
	int					portDest;
	bool				deleteWhenDone;
	float				speed;
	grinliz::CDynArray< GridEdge > path;
};

#endif // GRID_MOVE_COMPONENT_INCLUDED