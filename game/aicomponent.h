#ifndef AI_COMPONENT_INCLUDED
#define AI_COMPONENT_INCLUDED

#include "../xegame/component.h"

class AIComponent : public Component
{
public:
	AIComponent( int _team );
	virtual ~AIComponent();

	virtual bool NeedsTick()					{ return true; }
	virtual void DoTick( U32 delta );
	virtual void DebugStr( grinliz::GLString* str );

private:
	int team;
};


#endif // AI_COMPONENT_INCLUDED