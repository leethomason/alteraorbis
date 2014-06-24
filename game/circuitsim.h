#ifndef CIRCUIT_SIM_INCLUDED
#define CIRCUIT_SIM_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"

class WorldMap;
class Engine;
class Model;

class CircuitSim
{
public:
	CircuitSim(WorldMap* worldMap, Engine* engine);
	~CircuitSim();

	void Activate(const grinliz::Vector2I& pos);
	void DoTick(U32 delta);

private:
	void CreateSpark(const grinliz::Vector2I& pos, int rot4);
	bool SparkArrives(const grinliz::Vector2I& pos);
	void EmitSparkExplosion(const grinliz::Vector2F& pos);

	WorldMap* worldMap;
	Engine* engine;

	struct Electron {
		int charge;	// 0: spark, 1: charge
		int dir;
		float t;
		grinliz::Vector2I pos;
		Model* model;
	};

	grinliz::CDynArray< Electron > electrons;	// sparks and charges
};

#endif // CIRCUIT_SIM_INCLUDED
