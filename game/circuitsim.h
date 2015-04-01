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
	};

	struct Connection {
		grinliz::Vector2I a, b;		// somewhere in the group bounds
		int type;
		const void* sortA;
		const void* sortB;
	};

	enum class EParticleType {
		controlOn,
		controlOff,
		power
	};

	struct Particle {
		EParticleType type;
		int powerRequest;
		grinliz::Vector2F origin;
		grinliz::Vector2F pos;
		grinliz::Vector2F dest;
		int delay;
	};

	void NewParticle(EParticleType type, int powerRequest, const grinliz::Vector2F& origin, const grinliz::Vector2F& dest, int delay = 0);

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
	grinliz::Vector2F FindPower(const Group& forGroup);
	void CleanConnections();
	void DoSensor(EParticleType type, const grinliz::Vector2I& pos);

	// Run through the connections, validate they are okay, throw away the bad ones.
	//void ValidateConnections();
	void ParticleArrived(const Particle& p);
	void DeviceOn(Chit* chit);
	void DeviceOff(Chit* chit);

	const ChitContext* context;

	enum {
		POWER_GROUP,
		SENSOR_GROUP,
		DEVICE_GROUP,
		
		NUM_GROUPS
	};

	// cache/temporaries
	grinliz::CDynArray<Chit*> queryArr, combinedArr;
	grinliz::HashTable<grinliz::Vector2I, Chit*, CompValueVector2I> hashTable;	// used in the fill algorithm
	grinliz::CDynArray<const Connection*> queryConn;
	grinliz::CDynArray<Particle> newQueue;

	// Data
	grinliz::CDynArray<Group> groups[NUM_GROUPS];
	grinliz::CDynArray<Connection> connections;
	grinliz::CDynArray<Particle> particles;
	int roundRobbin;

	gamui::Canvas canvas[NUM_GROUPS];
};

#endif // CIRCUIT_SIM_INCLUDED
