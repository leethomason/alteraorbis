#ifndef DIRECTOR_INCLUDED
#define DIRECTOR_INCLUDED

#include "../xegame/component.h"
#include "../xegame/chit.h"
#include "../xegame/cticker.h"

class Plot;

class Director : public Component, public IChitListener
{
private:
	typedef Component super;

public:
	Director();
	virtual ~Director();

	virtual const char* Name() const { return "Director"; }

	virtual void DebugStr(grinliz::GLString* str)	{ str->AppendFormat("[Director] "); }

	virtual void Serialize( XStream* );
	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual int DoTick(U32 delta);

	grinliz::Vector2I ShouldSendHerd(Chit* herd);

	void Swarm(const grinliz::IString& critter, const grinliz::Vector2I& start, const grinliz::Vector2I& end);

private:
	Plot* plot = 0;

	CTicker plotTicker;
	CTicker attackTicker;
	bool attractLesser = false;
	bool attractGreater = false;
};

#endif // DIRECTOR_INCLUDED
