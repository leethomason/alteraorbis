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

	void Connect(const grinliz::Vector2I& a, const grinliz::Vector2I& b);

private:

	struct Group {
		grinliz::Rectangle2I bounds;
		grinliz::CArray<int, 16> idArr;			// FIXME: there are lots of small groups - inefficient. And buggy.
		grinliz::CArray<int, 16> connections;	// FIXME: same as above...
	};

	struct Connection {
		grinliz::Vector2I a, b;		// somewhere in the group bounds
		int type;
	};

	enum class EParticleType {
		control,
		power
	};

	struct Particle {
		EParticleType type;
		grinliz::Vector2F pos;
		grinliz::Vector2F dest;
	};

	class CompValueVector2I {
	public:
		template <class T>
		static U32 Hash( const T& v)					{ return v.y*16000 + v.x; }
		template <class T>
		static bool Equal( const T& v0, const T& v1 )	{ return v0 == v1; }
	};

	void FillGroup(Group* g, const grinliz::Vector2I& pos, Chit* chit);
	// Connects groups that can be connected.
	bool ConnectionValid(const grinliz::Vector2I& a, const grinliz::Vector2I& b, int* type, Group **groupA, Group** groupB);
	bool FindGroup(const grinliz::Vector2I& pos, int* groupType, int* index);
	void FindConnections(const Group& group, grinliz::CDynArray<const Connection*> *connections);
	// Run through the connections, validate they are okay, throw away the bad ones.
	//void ValidateConnections();
	void ParticleArrived(const Particle& p)	{}

	const ChitContext* context;

	enum {
		POWER_GROUP,
		SENSOR_GROUP,
		DEVICE_GROUP,
		
		NUM_GROUPS
	};

	// cache/temporaries
	grinliz::CDynArray<Chit*> queryArr, combinedArr;
	grinliz::HashTable<grinliz::Vector2I, Chit*, CompValueVector2I> hashTable;
	grinliz::CDynArray<const Connection*> queryConn;

	// Data
	grinliz::CDynArray<Group> groups[NUM_GROUPS];
	grinliz::CDynArray<Connection> connections;
	grinliz::CDynArray<Particle> particles;

	gamui::Canvas canvas[NUM_GROUPS];
};

#endif // CIRCUIT_SIM_INCLUDED
