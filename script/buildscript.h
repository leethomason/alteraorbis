#ifndef BUILD_SCRIPT_INCLUDED
#define BUILD_SCRIPT_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../ai/aineeds.h"
#include "../game/circuitsim.h"
#include "../gamui/gamui.h"

struct BuildData
{
	const char*			cName;
	const char*			cStructure;
	int					group;
	const char*			desc;				// what does this do?
	const char*			requirementDesc;	// what requirements are needed to build this?

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

	void LabelWithCount(int count, grinliz::CStr<64>* str) const;

	static grinliz::Rectangle2I Bounds(int size, const grinliz::Vector2I& pos);
	static grinliz::Rectangle2I PorchBounds(int size, const grinliz::Vector2I& pos, int rot0_3);
	static int DrawBounds(const grinliz::Rectangle2I& bounds, const grinliz::Rectangle2I& porchBounds,
						  gamui::Image* imageArr);
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

		SWITCH_ON,
		SWITCH_OFF,
		DETECTOR,
		TURRET,
		GATE,

		NUM_PLAYER_OPTIONS,

		// Start non-player buildings.
		TROLL_STATUE = NUM_PLAYER_OPTIONS,
		TROLL_BRAZIER,
		KAMAKIRI_STATUE,
		GOBMAN_STATUE,

		NUM_TOTAL_OPTIONS,

		GROUP_UTILITY = 0,
		GROUP_VISITOR,
		GROUP_ECONOMY,
		GROUP_BATTLE,
		GROUP_INDUSTRY,

		END_CIRCUITS = NUM_PLAYER_OPTIONS,
	};

	static bool IsBuild( int action ) { return action >= PAVE; }
	static bool IsClear( int action ) { return action == CLEAR; }

	const BuildData& GetData( int i );
	const BuildData* GetDataFromStructure( const grinliz::IString& structure, int* id );

private:
	static BuildData buildData[NUM_TOTAL_OPTIONS];
};

#endif // BUILD_SCRIPT_INCLUDED
