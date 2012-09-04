#ifndef HEALTH_COMPONENT_INCLUDED
#define HEALTH_COMPONENT_INCLUDED

#include "../xegame/component.h"

/*
	Binds to the Item component for values; essentually the script that
	drives health, effects, etc.
*/
class HealthComponent : public Component
{
public:
	HealthComponent() : destroyed(false)	{}
	virtual ~HealthComponent()	{}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "HealthComponent" ) ) return this;
		return 0;
	}

	virtual void DebugStr( grinliz::GLString* str )		{ str->Format( "[Health] " ); }

	float GetHealthFraction() const;
	void  DeltaHealth();

private:
	bool destroyed;
};


#endif // HEALTH_COMPONENT_INCLUDED