#ifndef ALTERA_PLOT_INCLUDED
#define ALTERA_PLOT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glvector.h"
#include "../xegame/cticker.h"

class Chit;
class ChitContext;
class XStream;

/*
	- Foo the beast master (Shrogshriff flunky leading a "swarm" quest for destruction)
		- mantis, red-mantis, trilo
		- Truulga flunky for trolls
		- turn to chaos at the end
	- Ring forge
		- cursed rings forged; given to lesser
		- once enough in denizen hands, become chaos uber-weapons
		- transform owner?
		- occupy mother core?
	- Golem King / Dragon Queen 
		- battle across domains
		- establish home base w/ enough gold
*/

class Plot
{
public:
	Plot()	{}

	virtual grinliz::Vector2I ShouldSendHerd(Chit* chit) = 0;
	virtual void Serialize(XStream*) = 0;
	virtual bool DoTick(U32 time) = 0;

	static Plot* Factory(const char*);

	void SetContext(const ChitContext* _context) { context = _context; }

protected:
	const ChitContext* context = 0;
};


class SwarmPlot : public Plot
{
public:
	SwarmPlot()	{
		start.Zero();
		end.Zero();
		current.Zero();
	}
		
	void Init(const ChitContext* context, 
			  const grinliz::IString& critter, 
			  const grinliz::Vector2I& start, 
			  const grinliz::Vector2I& end);

	virtual grinliz::Vector2I ShouldSendHerd(Chit* chit);
	virtual void Serialize(XStream*);
	virtual bool DoTick(U32 time);

private:
	bool AdvancePlot();
	static const int SWARM_TIME = 2 * (60 * 1000);	// Minutes per sector. Needs tuning.

	CTicker ticker;
	grinliz::Vector2I start, end, current;
	grinliz::IString  critter;
};


class GreatBattlePlot : public Plot
{
public:
	GreatBattlePlot()	{
		dest.Zero();
	}
		
	void Init(const ChitContext* _context, const grinliz::Vector2I& _dest);

	virtual grinliz::Vector2I ShouldSendHerd(Chit* chit);
	virtual void Serialize(XStream*);
	virtual bool DoTick(U32 time);

private:
	bool AdvancePlot();
	static const int BATTLE_TIME = 4 * (60 * 1000);	// Minutes to summon. Needs tuning.

	CTicker ticker;
	grinliz::Vector2I dest;
};

#endif // ALTERA_PLOT_INCLUDED
