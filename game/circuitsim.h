#ifndef CIRCUIT_SIM_INCLUDED
#define CIRCUIT_SIM_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../xegame/chit.h"

class WorldMap;
class Engine;
class Model;
class LumosChitBag;
class XStream;

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
	CIRCUIT_TRANSISTOR_A,	// needs to be last: some special code that there is one transistor in 2 states.
	CIRCUIT_TRANSISTOR_B,
	NUM_CIRCUITS
};


class CircuitSim
{
public:
	CircuitSim(WorldMap* worldMap, Engine* engine, LumosChitBag* chitBag);
	~CircuitSim();

	void Serialize(XStream* xs);

	void TriggerSwitch(const grinliz::Vector2I& pos);
	void TriggerDetector(const grinliz::Vector2I& pos);
	void DoTick(U32 delta);

	static int NameToID(grinliz::IString name);

private:
	struct Electron {
		int charge;	// 0: spark, 1: charge
		int dir;
		float t;
		grinliz::Vector2I pos;
		Model* model;		// not serialized (on demand)

		void Serialize(XStream* xs);
	};

	void CreateElectron(const grinliz::Vector2I& pos, int dir4, int charge);
	// Returns 'true' if the original charge/spark is consumed.
	bool ElectronArrives(Electron*);
	void EmitSparkExplosion(const grinliz::Vector2F& pos);
	void Explosion(const grinliz::Vector2I& pos, int charge, bool circuitDestroyed);
	void ApplyPowerUp(const grinliz::Vector2I& pos, int charge);

	WorldMap* worldMap;
	Engine* engine;
	LumosChitBag* chitBag;

	grinliz::CDynArray< Electron > electrons;	// sparks and charges
	grinliz::CDynArray< Electron > electronsCopy;	// sparks and charges
};

#endif // CIRCUIT_SIM_INCLUDED
