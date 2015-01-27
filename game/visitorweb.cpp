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

	const Vector2I center = { NUM_SECTORS / 2, NUM_SECTORS / 2 };
	int startIndex = cores.FindMax(center, [](const Vector2I& center, const Vector2I& core){
		return -(center - core).LengthSquared();
	});
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
	node->sector = origin;
	const Node* save = nodes.Mem();

	CDynArray<Web::WebLink> edgeSet;
	for (int i = 0; i < edges.Size(); ++i) {
		edgeSet.Push(edges[i]);
	}

	BuildNodeRec(node, &edgeSet);
	GLASSERT(save == nodes.Mem());
	//DumpNodeRec(&nodes[0], 0);
}


void Web::DumpNodeRec(Node* node, int depth)
{
	for (int i = 0; i < depth; ++i) GLOUTPUT(("  "));
	GLOUTPUT(("%x, %x\n", node->sector.x, node->sector.y));
	for (int i = 0; i < node->child.Size(); ++i)
		DumpNodeRec(node->child[0], depth + 1);
}


void Web::BuildNodeRec( Node* node, grinliz::CDynArray<Web::WebLink>* e)
{
	int nChildren = 0;
	while (true) {
		int i = e->Find(node->sector, [](const Vector2I& sector, const Web::WebLink& link) {
			return (link.sector0 == sector) || (link.sector1 == sector);
		});
		if (i < 0) {
			break;
		}
		Node* next = nodes.PushArr(1);
		next->sector = ((*e)[i].sector0 == node->sector) ? (*e)[i].sector1 : (*e)[i].sector0;
		e->SwapRemove(i);
		node->child.Push(next);
		++nChildren;
	}
	if (nChildren == 0) return;

	float strength = node->strength / float(nChildren);
	for (int i = 0; i < node->child.Size(); ++i) {
		node->child[i]->strength = strength;
		BuildNodeRec(node->child[i], e);
	}
}
