#ifndef HEALTH_COMPONENT_INCLUDED
#define HEALTH_COMPONENT_INCLUDED

#include "../xegame/component.h"

class HealthComponent : public Component
{
public:
	HealthComponent() : health( 0xffff ), maxHealth( 0xffff ) {}
	virtual ~HealthComponent() {}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "HealthComponent" ) ) return this;
		return 0;
	}

	virtual void DebugStr( grinliz::GLString* str )		{ str->Format( "[Health] hp=%d ", health ); }

	void SetHealth( int h );
	void SetHealth( int _health, int _max ) { GLASSERT( _max >= _health ); SetMaxHealth( _max ); SetHealth( _health ); }
	void SetMaxHealth( int m );
	int  GetHealth() const			{ return health; }
	int  GetMaxHealth() const		{ return maxHealth; }
	float GetHealthFraction() const { return (float)health / (float)maxHealth; }

private:
	int health;
	int maxHealth;
};


#endif // HEALTH_COMPONENT_INCLUDED