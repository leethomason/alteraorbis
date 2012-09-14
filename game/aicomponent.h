#ifndef AI_COMPONENT_INCLUDED
#define AI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrectangle.h"
#include "../script/battlemechanics.h"

class WorldMap;
class Engine;

inline float UtilityCubic( float y0, float y1, float x ) {
	x = grinliz::Clamp( x, 0.0f, 1.0f );
	float a = grinliz::Lerp( y0, y1, x*x*x );
	return a;
}

inline float UtilityFade( float y0, float y1, float x ) {
	x = grinliz::Clamp( x, 0.0f, 1.0f );
	float a = grinliz::Lerp( y0, y1, grinliz::Fade5( x ) );
	return a;
}

inline float UtilityLinear( float y0, float y1, float x ) {
	x = grinliz::Clamp( x, 0.0f, 1.0f );
	float a = grinliz::Lerp( y0, y1, x );
	return a;
}

// Highest at x=0.5
inline float UtilityParabolic( float y0, float maxY, float y1, float x ) {
	x = grinliz::Clamp( x, 0.0f, 1.0f );
	float xp = x - 0.5f;
	float a = 1.0f - ( xp*xp )*4.f;
	return a;
}


// Combat AI: needs refactoring
class AIComponent : public Component
{
public:
	AIComponent( Engine* _engine, WorldMap* _worldMap );
	virtual ~AIComponent();

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "AIComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual bool DoTick( U32 delta );
	virtual bool DoSlowTick();
	virtual void DebugStr( grinliz::GLString* str );
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void OnChitEvent( const ChitEvent& event );

	// Approximate. enemyList may not be flushed.
	bool AwareOfEnemy() const { return enemyList.Size() > 0; }

private:
	enum {
		FRIENDLY,
		ENEMY,
		NEUTRAL,
		
		MAX_TRACK = 8,
	};

	void UpdateCombatInfo( const grinliz::Rectangle2F* _zone=0 );
	int GetTeamStatus( Chit* other );
	bool LineOfSight( Chit* src, Chit* target );
	void Think();	// Choose a new action.

	Engine*		engine;
	WorldMap*	map;

	enum {
		// Possible actions:
		NO_ACTION,
		MELEE,			// Go to the target and hit it. The basic combat action.
		SHOOT,			// Shoot targets. Fall back to melee if no shot, which
						// gets closer to target. Run and Gun if it makes sense.
		//MOVE			// move to a better location (rejoin squad or during re-load)
		//RELOAD
		//RUN_AND_GUN
		NUM_ACTIONS,
	};
	int currentAction;

	void DoMelee();
	void DoShoot();

	grinliz::CArray<int, MAX_TRACK> friendList;
	grinliz::CArray<int, MAX_TRACK> enemyList;
	grinliz::CDynArray<Chit*>		chitArr;
	BattleMechanics battleMechanics;
};


#endif // AI_COMPONENT_INCLUDED