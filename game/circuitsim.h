#ifndef CIRCUIT_SIM_INCLUDED
#define CIRCUIT_SIM_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glrandom.h"
#include "../gamui/gamui.h"
#include "../xegame/chit.h"
#include "../xegame/cticker.h"

class WorldMap;
class Engine;
class Model;
class LumosChitBag;
class XStream;
class ChitContext;


class CircuitSim
{
public:
	CircuitSim(const ChitContext* context, const grinliz::Vector2I& sector);
	~CircuitSim();

	void Serialize(XStream* xs);

	void TriggerSwitch(const grinliz::Vector2I& pos);
	void TriggerDetector(const grinliz::Vector2I& pos);

	void DoTick(U32 delta);

	void Connect(const grinliz::Vector2I& a, const grinliz::Vector2I& b);

	void DragStart(const grinliz::Vector2F& v);
	void Drag(const grinliz::Vector2F& v);
	void DragEnd(const grinliz::Vector2F& v);

	void EnableOverlay(bool enable) { enableOverlay = enable; ticker.SetReady(); }

private:
	struct Group {
		grinliz::Rectangle2I bounds;
	};

	struct Connection {
		grinliz::Vector2I a, b;		// somewhere in the group bounds
		int type;
		const void* sortA;
		const void* sortB;

		void Serialize(XStream* xs);
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

		void Serialize(XStream* xs);
	};

	void CalcGroups();
	void DrawGroups();
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
	grinliz::Vector2F dragStart, dragCurrent;
	grinliz::Vector2I sector;
	CTicker ticker;

	// Data, but not serialized.
	grinliz::CDynArray<Group> groups[NUM_GROUPS];
	gamui::Canvas canvas[2][NUM_GROUPS];

	// Data
	bool enableOverlay;
	grinliz::CDynArray<Connection> connections;
	grinliz::CDynArray<Particle> particles;
	grinliz::HashTable<grinliz::Vector2I, int, CompValueVector2I> roundRobbin;	// which device's turn is it to fire? 
};

#endif // CIRCUIT_SIM_INCLUDED
