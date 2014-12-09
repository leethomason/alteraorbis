#include "visitorweb.h"
#include "../grinliz/glarrayutil.h"
#include "gamelimits.h"

using namespace grinliz;

void Web::Calc(const Vector2I* _cores, int nCores)
{
	edges.Clear();
	if (nCores == 0) return;

	CArray<Vector2I, NUM_SECTORS * NUM_SECTORS> cores, inSet;

	for (int i = 0; i < nCores; ++i) {
		GLASSERT(cores.HasCap());
		cores.Push(_cores[i]);
	}

	int startIndex = 0;
	const Vector2I center = { NUM_SECTORS / 2, NUM_SECTORS / 2 };
	GL_ARRAY_FIND_MAX(cores, nCores, (-(center - ele).LengthSquared()), &startIndex, 1);
	GLASSERT(startIndex >= 0);

	origin = cores[startIndex];
	inSet.Push(cores[startIndex]);
	cores.SwapRemove(startIndex);

	// 'cores' is the out list, 'web' is the in list.
	while (!cores.Empty()) {
		Vector2I bestSrc = { 0, 0 };
		Vector2I bestDst = { 0, 0 };
		int bestIndex = 0;
		int bestScore = INT_MIN;

		for (int i = 0; i < cores.Size(); ++i) {
			for (int k = 0; k < inSet.Size(); ++k) {
				int score = -(inSet[k] - cores[i]).LengthSquared();
				if (score > bestScore) {
					bestScore = score;
					bestSrc = inSet[k];
					bestDst = cores[i];
					bestIndex = i;
				}
			}
		}
		GLASSERT(!bestSrc.IsZero());
		GLASSERT(!bestDst.IsZero());
		cores.SwapRemove(bestIndex);
		inSet.Push(bestDst);

		WebLink link = { bestSrc, bestDst };
		if (edges.Find(link) < 0) {
			edges.Push(link);
		}
	}

}