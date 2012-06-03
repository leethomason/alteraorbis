#ifndef HEALTH_COMPONENT_INCLUDED
#define HEALTH_COMPONENT_INCLUDED

#include "../xegame/component.h"

class HealthComponent : public Component
{
public:
	HealthComponent() : health( 100 ) {}
	virtual ~HealthComponent() {}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "HealthComponent" ) ) return this;
		return 0;
	}

	virtual void DebugStr( grinliz::GLString* str )		{ str->Format( "[Health] hp=%d ", health ); }

	void SetHealth( int h );
	int  Health() const			{ return health; }

private:
	int health;
};


#endif // HEALTH_COMPONENT_INCLUDED