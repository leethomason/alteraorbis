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
	x Evil rising
		x evil/corrupt flora
		x appearance, leads to beast master plot
	x Foo the beast master (Shrogshriff flunky leading a "swarm" quest for destruction)
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

	virtual grinliz::Vector2I PrioritySendHerd(Chit* chit) { grinliz::Vector2I zero = { 0, 0 }; return zero; }
	virtual grinliz::Vector2I ShouldSendHerd(Chit* chit) = 0;


	virtual void Serialize(XStream*) = 0;
	virtual bool DoTick(U32 time) = 0;

	static Plot* Factory(const char*);

	void SetContext(const ChitContext* _context) { context = _context; }
	virtual bool SectorIsEvil(const grinliz::Vector2I& sector) { return false;  }

protected:
	void ToChaos(const grinliz::Vector2I& sector, const grinliz::IString& critter);

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


class EvilRisingPlot : public Plot
{
public:
	EvilRisingPlot() :badGuyID(0), stage(0)	{
		destSector.Zero();
	}
		
	void Init(const ChitContext* _context, 
			  const grinliz::Vector2I& _destSector, 
			  const grinliz::IString& _critter,
			  int _critterTeam,
			  const grinliz::IString& _demiName,
			  bool _demiIsFemale);

	virtual grinliz::Vector2I PrioritySendHerd(Chit* chit);
	virtual grinliz::Vector2I ShouldSendHerd(Chit* chit);
	virtual void Serialize(XStream*);
	virtual bool DoTick(U32 time);

	virtual bool SectorIsEvil(const grinliz::Vector2I& sector);

private:
	enum {
		GROWTH_STAGE,
		SWARM_STAGE
	};
	static const U32 MAX_PLOT_TIME = 10 * 60 * 1000;
	static const int SWARM_TIME = 2 * (60 * 1000);	// Minutes per sector. Needs tuning.

	int					stage;
	int					badGuyID;
	CTicker				overTime;
	CTicker				eventTime;
	grinliz::Vector2I	destSector;
	grinliz::IString	critter;
	int					critterTeam;
	grinliz::IString	demiName;
	bool				demiIsFemale;
};

#endif // ALTERA_PLOT_INCLUDED
