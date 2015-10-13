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

	grinliz::Vector2I PrioritySendHerd(Chit* herd);
	grinliz::Vector2I ShouldSendHerd(Chit* herd);
	bool SectorIsEvil(const grinliz::Vector2I& sector);

	void Swarm(const grinliz::IString& critter, const grinliz::Vector2I& start, const grinliz::Vector2I& end);
	void GreatBattle(const grinliz::Vector2I& pos);
	void EvilRising(const grinliz::Vector2I& pos);
	void DeletePlot();

private:
	void StartRandomPlot();

	Plot* plot = 0;

	CTicker plotTicker;
	CTicker attackTicker;
	bool attractLesser = false;
	bool attractGreater = false;
};

#endif // DIRECTOR_INCLUDED
