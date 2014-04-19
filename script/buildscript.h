#ifndef BUILD_SCRIPT_INCLUDED
#define BUILD_SCRIPT_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../ai/aineeds.h"

struct BuildData
{
	const char*			cName;
	const char*			cStructure;
	int					group;
	int					cost;
	int					size;			// 1 or 2
	grinliz::IString	name;			// "Vault"
	grinliz::IString	label;			// "Vault\n50"
	grinliz::IString	structure;		// "vault"
	ai::Needs			needs;			// needs advertised by the building
	int					standTime;
};


// Utility class. Cheap to create.
class BuildScript
{
public:
	BuildScript()	{}

	enum {
		NONE_ACTIVE=0,
		CANCEL,
		CLEAR,
		ROTATE,
		PAVE,
		ICE,
		KIOSK_N,
		KIOSK_M,
		KIOSK_C,
		KIOSK_S,
		VAULT,
		FACTORY,
		POWER,
		BED,
		MARKET,
		BAR,
		GUARDPOST,
		FARM,
		DISTILLERY,
		NUM_OPTIONS,

		NUM_GROUPS = 4,

		GROUP_UTILITY = 0,
		GROUP_VISITOR,
		GROUP_ECONOMY,
		GROUP_BATTLE,
		GROUP_INDUSTRY
	};

	static bool IsBuild( int action ) { return action >= PAVE; }
	static bool IsClear( int action ) { return action == CLEAR; }

	const BuildData& GetData( int i );
	const BuildData* GetDataFromStructure( const grinliz::IString& structure );

private:
	static BuildData buildData[NUM_OPTIONS];
};

#endif // BUILD_SCRIPT_INCLUDED
