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

/*
	on switch
	off switch
	temple (battery)
	gate / silica creator (how to do this???)
	enemy detector
*/


class CircuitSim
{
public:
	CircuitSim(const ChitContext* context);
	~CircuitSim();

	void Serialize(XStream* xs);

	void TriggerSwitch(const grinliz::Vector2I& pos);
	void TriggerDetector(const grinliz::Vector2I& pos);
	void DoTick(U32 delta);

	bool visible;

	void CalcGroups(const grinliz::Vector2I& sector);
	void DrawGroups();

private:

	struct Group {
		grinliz::Rectangle2I bounds;
		grinliz::CArray<int, 16> idArr;
	};
	class CompValueVector2I {
	public:
		template <class T>
		static U32 Hash( const T& v)					{ return v.y*16000 + v.x; }
		template <class T>
		static bool Equal( const T& v0, const T& v1 )	{ return v0 == v1; }
	};

	void FillGroup(Group* g, const grinliz::Vector2I& pos, Chit* chit);

	const ChitContext* context;

	enum {
		POWER_GROUP,
		SENSOR_GROUP,
		DEVICE_GROUP,
		
		NUM_GROUPS
	};

	grinliz::CDynArray<Group> groups[NUM_GROUPS];
	grinliz::CDynArray<Chit*> queryArr, combinedArr;
	grinliz::HashTable<grinliz::Vector2I, Chit*, CompValueVector2I> hashTable;

	gamui::Canvas canvas[NUM_GROUPS];
};

#endif // CIRCUIT_SIM_INCLUDED
