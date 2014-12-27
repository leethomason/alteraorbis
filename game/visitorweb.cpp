#include "visitorweb.h"
#include "../grinliz/glarrayutil.h"
#include "gamelimits.h"

using namespace grinliz;

void Web::Edge(int i, grinliz::Vector2I* s0, grinliz::Vector2I* s1) const
{
	const WebLink& link = edges[i];
	*s0 = link.sector0;
	*s1 = link.sector1;
}


const Web::Node* Web::FindNode(const grinliz::Vector2I& v) const
{
	for (int i = 0; i < nodes.Size(); ++i) {
		if (nodes[i].sector == v) {
			return &nodes[i];
		}
	}
	return 0;
}


void Web::Calc(const Vector2I* _cores, int nCores)
{
	edges.Clear();
	nodes.Clear();
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

	nodes.Reserve(nCores);
	Node* node = nodes.PushArr(1);
	const Node* save = nodes.Mem();

	CDynArray<bool> edgeSet;
	for (int i = 0; i < edges.Size(); ++i) {
		edgeSet.Push(false);
	}

	BuildNodeRec(origin, node, &edgeSet);
	GLASSERT(save == nodes.Mem());
}


void Web::BuildNodeRec(grinliz::Vector2I pos, Node* node, grinliz::CDynArray<bool>* edgeSet)
{
	node->sector = pos;

	int i = -1;
	GL_ARRAY_FIND(edges.Mem(), edges.Size(), ((ele.sector0 == pos || ele.sector1 == pos) && (*edgeSet)[index] == false), &i);
	if (i >= 0) {
		const WebLink& link = edges[i];
		if (link.sector0 == pos) {
			Node* next = nodes.PushArr(1);
			node->adjacent.Push(next);
			(*edgeSet)[i] = true;
			BuildNodeRec(link.sector1, next, edgeSet);
		}
		else if (link.sector1 == pos) {
			Node* next = nodes.PushArr(1);
			node->adjacent.Push(next);
			(*edgeSet)[i] = true;
			BuildNodeRec(link.sector0, next, edgeSet);
		}
	}
}
