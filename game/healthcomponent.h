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

#ifndef HEALTH_COMPONENT_INCLUDED
#define HEALTH_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../script/battlemechanics.h"

class Engine;

/*
	Binds to the Item component for values; essentually the script that
	drives health, effects, etc.
*/
class HealthComponent : public Component
{
private:
	typedef Component super;

public:
	HealthComponent() : destroyed(0)	{}
	virtual ~HealthComponent()	{}

	virtual const char* Name() const { return "HealthComponent"; }
	virtual HealthComponent* ToHealthComponent() { return this; }

	virtual void Serialize( XStream* xs );

	virtual void DebugStr( grinliz::GLString* str )		{ str->AppendFormat( "[Health] " ); }
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual int DoTick( U32 delta );
	float DestroyedFraction() const { return (float)destroyed/(float)COUNTDOWN; }

private:
	enum { COUNTDOWN = 800 };
	void DeltaHealth();

	U32 destroyed;
	BattleMechanics battleMechanics;
};


#endif // HEALTH_COMPONENT_INCLUDED