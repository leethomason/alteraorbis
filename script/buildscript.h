#ifndef BUILD_SCRIPT_INCLUDED
#define BUILD_SCRIPT_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../ai/aineeds.h"
#include "../game/circuitsim.h"

struct BuildData
{
	const char*			cName;
	const char*			cStructure;
	int					group;
	const char*			desc;				// what does this do?
	const char*			requirementDesc;	// what requirements are needed to build this?
	int					circuit;

	int					cost;
	int					size;			// 1 or 2
	grinliz::IString	name;			// "Vault"
	grinliz::IString	label;			// "Vault\n50"
	grinliz::IString	structure;		// "vault"
	ai::Needs			needs;			// needs advertised by the building
	int					standTime;
	bool				porch;

	enum {ZONE_NONE, ZONE_INDUSTRIAL, ZONE_NATURAL};
	int					zone;
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

		SLEEPTUBE,
		TEMPLE,
		GUARDPOST,

		FARM,
		DISTILLERY,
		BAR,

		MARKET,
		FORGE,
		EXCHANGE,
		VAULT,

		KIOSK,

		// Trickiness:
		// CIRCUIT_NONE isn't used for building, so CIRCUIT_START maps to CIRCUIT_SWITCH
		// The TRANSISTORS are duplicates in the UI.
		// Easier - if dangerous - to duplicate.
		CIRCUITFAB,
		BATTERY,
		TURRET,
		BUILD_CIRCUIT_SWITCH,
		//BUILD_CIRCUIT_ZAPPER,	// disabled for now. turrets mostly cover this
		BUILD_CIRCUIT_BEND,
		BUILD_CIRCUTI_FORK_2,
		BUILD_CIRCUIT_ICE,
		BUILD_CIRCUIT_STOP,
		BUILD_CIRCUIT_DETECT_ENEMY,
		BUILD_CIRCUIT_TRANSISTOR,

		NUM_PLAYER_OPTIONS,

		// Start non-player buildings.
		TROLL_STATUE = NUM_PLAYER_OPTIONS,
		TROLL_BRAZIER,
		KAMAKIRI_STATUE,

		NUM_TOTAL_OPTIONS,

		GROUP_UTILITY = 0,
		GROUP_VISITOR,
		GROUP_ECONOMY,
		GROUP_BATTLE,
		GROUP_INDUSTRY,

		END_CIRCUITS = NUM_PLAYER_OPTIONS,
	};

	static bool IsCircuit(int action) {
		if (action >= BUILD_CIRCUIT_SWITCH && action < END_CIRCUITS && action != BATTERY) {
			return true;
		}
		return false;
	}
	static bool IsBuild( int action ) { return action >= PAVE; }
	static bool IsClear( int action ) { return action == CLEAR; }

	const BuildData& GetData( int i );
	const BuildData* GetDataFromStructure( const grinliz::IString& structure, int* id );

private:
	static BuildData buildData[NUM_TOTAL_OPTIONS];
};

#endif // BUILD_SCRIPT_INCLUDED
