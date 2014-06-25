#ifndef CIRCUIT_SIM_INCLUDED
#define CIRCUIT_SIM_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"

class WorldMap;
class Engine;
class Model;
class LumosChitBag;

enum {
	CIRCUIT_NONE,
	CIRCUIT_SWITCH,
	CIRCUIT_BATTERY,
	CIRCUIT_POWER_UP,
};


class CircuitSim
{
public:
	CircuitSim(WorldMap* worldMap, Engine* engine, LumosChitBag* chitBag);
	~CircuitSim();

	void Activate(const grinliz::Vector2I& pos);
	void DoTick(U32 delta);

private:
	struct Electron {
		int charge;	// 0: spark, 1: charge
		int dir;
		float t;
		grinliz::Vector2I pos;
		Model* model;
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
