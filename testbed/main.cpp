#include "../grinliz/gldebug.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glspatialhash.h"

using namespace grinliz;

void TestSpatialHash()
{
	SpatialHash<int> spatial;
	CDynArray<int> query;

	Vector2I v = { 0, 0 };

	spatial.Add(v, 0);
	spatial.Query(v, &query);
	GLASSERT(query.Size() == 1 && query[0] == 0);
	spatial.Remove(v, 0);
	GLASSERT(spatial.Query(v, &query) == 0);
	GLASSERT(spatial.Empty());

	static int NUM = 2000;
	static int MULT = 7;
	static int MAP = 1024;

	for (int i = 0; i < NUM; ++i) {
		for (int j = 0; j < 4; ++j) {
			Vector2I pos = { (i*MULT) / MAP, (i*MULT) % MAP };
			spatial.Add(pos, j);
		}
	}
	for (int i = 0; i < NUM; ++i) {
		Vector2I pos = { (i*MULT) / MAP, (i*MULT) % MAP };
		spatial.Remove(pos, i%4);
	}
	for (int i = 0; i < NUM; ++i) {
		Vector2I pos = { (i*MULT) / MAP, (i*MULT) % MAP };
		spatial.Query(pos, &query);
		GLASSERT(query.Size() == 3);
		for (int j = 0; j < query.Size(); ++j) {
			GLASSERT(query[j] >= 0 && query[j] < 4 && query[j] != (i % 4));
		}
		for (int j = 0; j < query.Size(); ++j) {
			spatial.Remove(pos, query[j]);
		}
	}
	GLASSERT(spatial.Empty());

	printf("Spatial Hash test. nAllocated=%d nProbes=%d nSteps=%d eff=%.3f density=%.3f\n",
		   spatial.NumAllocated(), spatial.NumProbes(), spatial.NumSteps(), spatial.Efficiency(), spatial.Density());
}

int main(int argc, const char* argv[])
{
	Matrix4::Test();
	Quaternion::Test();
	TestSpatialHash();
	return 0;
}