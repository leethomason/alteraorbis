#ifndef DIRECTOR_INCLUDED
#define DIRECTOR_INCLUDED

#include "../xegame/component.h"
#include "../xegame/chit.h"
#include "../xegame/cticker.h"

class Director : public Component, public IChitListener
{
private:
	typedef Component super;

public:

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
	enum class EPlot {
		NONE,
		SWARM
	};

	void AdvancePlot();

	CTicker attackTicker;
	bool attractLesser = false;
	bool attractGreater = false;

	EPlot plot = EPlot::NONE;
	static const int SWARM_TIME = 60 * 1000;

	// Plot data:
	CTicker plotTicker;
	grinliz::Vector2I plotStart, plotEnd, plotCurrent;
	grinliz::IString  plotCritter;
};

#endif // DIRECTOR_INCLUDED
