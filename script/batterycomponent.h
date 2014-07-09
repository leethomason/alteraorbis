#ifndef BATTERY_COMPONENT_INCLUDED
#define BATTERY_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/cticker.h"

class BatteryComponent : public Component
{
private:
	typedef Component super;

public:
	BatteryComponent() : charge(0), ticker(2000)	{}
	virtual ~BatteryComponent()	{}

	virtual const char* Name() const { return "BatteryComponent"; }

	virtual void Serialize( XStream* xs );

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();

	virtual void DebugStr( grinliz::GLString* str )		{ str->AppendFormat( "[Battery] %d ", charge ); }
	virtual int DoTick( U32 delta );

	int Charge() const { return charge; }
	int UseCharge() { int c = charge; charge = 0; ticker.Reset();  return c; }

private:
	int charge;
	CTicker ticker;
};


#endif // BATTERY_COMPONENT_INCLUDED