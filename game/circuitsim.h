#ifndef CIRCUIT_SIM_INCLUDED
#define CIRCUIT_SIM_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glrectangle.h"
#include "../gamui/gamui.h"
#include "../xegame/chit.h"

class WorldMap;
class Engine;
class Model;
class LumosChitBag;
class XStream;
struct ChitContext;

// WARNING: partial duplicate in BuildScript.h
// WARNING: partial duplicate in FluidTestScene.h
// WARNING: also need to add rendering in the WorldMap.cpp
enum {
	CIRCUIT_NONE,
	CIRCUIT_SWITCH,
	CIRCUIT_BATTERY,
	CIRCUIT_POWER_UP,
	CIRCUIT_BEND,
	CIRCUIT_FORK_2,
	CIRCUIT_ICE,
	CIRCUIT_STOP,
	CIRCUIT_DETECT_ENEMY,
	CIRCUIT_TRANSISTOR_A,	// needs to be last of the regular circuits: some special code that there is one transistor in 2 states.
	CIRCUIT_TRANSISTOR_B,
	CIRCUIT_LINE_NS_GREEN,
	CIRCUIT_LINE_EW_GREEN,
	CIRCUIT_LINE_CROSS_GREEN,
	CIRCUIT_LINE_NS_PAVE,
	CIRCUIT_LINE_EW_PAVE,
	CIRCUIT_LINE_CROSS_PAVE,

	CIRCUIT_LINE_START = CIRCUIT_LINE_NS_GREEN,
	CIRCUIT_LINE_END = CIRCUIT_LINE_CROSS_PAVE+1
};


class CircuitSim
{
public:
	CircuitSim(const ChitContext* context);
	~CircuitSim();

	void Serialize(XStream* xs);

	void TriggerSwitch(const grinliz::Vector2I& pos);
	void TriggerDetector(const grinliz::Vector2I& pos);
	void DoTick(U32 delta);

	void DrawGroups(const grinliz::Vector2I& sector, gamui::Canvas* canvas);

private:
	const ChitContext* context;

	enum {
		POWER_GROUP,
		SENSOR_GROUP,
		DEVICE_GROUP,
		
		NUM_GROUPS
	};

	struct Group {
		grinliz::Rectangle2I bounds;
		grinliz::CArray<int, 16> idArr;
	};

	grinliz::CDynArray<Group> groups[NUM_GROUPS];
	grinliz::CDynArray<Chit*> queryArr;

	gamui::Canvas canvas;
};

#endif // CIRCUIT_SIM_INCLUDED
