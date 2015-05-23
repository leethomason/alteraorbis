#include <limits.h>
#include "visitorweb.h"
#include "gamelimits.h"
#include "../grinliz/glarrayutil.h"
#include "../script/corescript.h"
#include "../game/team.h"

using namespace grinliz;


void MinSpanTree::Calc(const grinliz::Vector2I* points, int n)
{
	nodes.Clear();
	if (n < 1) return;

	Node start;
	start.pos = points[0];
	start.parentPos = points[0];
	nodes.Push(start);

	CDynArray<Vector2I> inSet;
	inSet.Reserve(n-1);
	for (int i = 1; i < n; ++i) {
		inSet.Push(points[i]);
	}

	while (!inSet.Empty()) {
		int bestNode = 0;
		int bestIn = 0;
		int bestScore = INT_MAX;

		for (int i = 0; i < nodes.Size(); ++i) {
			for (int k = 0; k < inSet.Size(); ++k) {
				int score = (inSet[k] - nodes[i].pos).LengthSquared();
				if (score < bestScore) {
					bestScore = score;
					bestNode = i;
					bestIn = k;
				}
			}
		}
		GLASSERT(bestScore != INT_MAX);
		Vector2I vInSet = inSet[bestIn];
		inSet.SwapRemove(bestIn);

		Node newNode;
		newNode.pos = vInSet;
		newNode.parentPos = nodes[bestNode].pos;
		newNode.nextSibling = nodes[bestNode].firstChild;
		nodes[bestNode].firstChild = nodes.Size();
		nodes.Push(newNode);
	}
	RecWalkStr(0, 1.0f);
}


int MinSpanTree::CountChildren(const Node& node) const
{
	int nChildren = 0;
	int childIdx = node.firstChild;
	while (childIdx) {
		++nChildren;
		childIdx = nodes[childIdx].nextSibling;
	}
	return nChildren;
}


const MinSpanTree::Node* MinSpanTree::ChildNode(const Node& parent, int n) const
{
	int nChildren = 0;
	int childIdx = parent.firstChild;
	while (childIdx) {
		if (nChildren == n)
			return &nodes[childIdx];

		childIdx = nodes[childIdx].nextSibling;
		++nChildren;
	}
	return nullptr;
}

void MinSpanTree::RecWalkStr(int nodeIdx, float str)
{
	nodes[nodeIdx].strength = str;
	int nChildren = CountChildren(nodes[nodeIdx]);
	if (nChildren) {
		float childStr = str / float(nChildren);
		int childIdx = nodes[nodeIdx].firstChild;
		while (childIdx) {
			RecWalkStr(childIdx, childStr);
			childIdx = nodes[childIdx].nextSibling;
		}
	}
}


const MinSpanTree::Node* Web::FindNode(const grinliz::Vector2I& v) const
{
	for (int i = 0; i < tree.NumNodes(); ++i) {
		const MinSpanTree::Node& node = tree.NodeAt(i);
		if (node.pos == v) {
			return &node;
		}
	}
	return 0;
}


void Web::Calc(const Vector2I* exclude)
{
	static Vector2I origin = { NUM_SECTORS / 2, NUM_SECTORS / 2 };
	CArray<Vector2I, NUM_SECTORS * NUM_SECTORS> cores;
	cores.Push(origin);

	int n = 0;
	CoreScript** list = CoreScript::GetCoreList(&n);
	for (int i = 0; i < n; ++i) {
		CoreScript* cs = list[i];
		Vector2I sector = ToSector(cs->ParentChit()->Position());
		if (    (sector != origin) && cs && cs->InUse() 
			 && Team::Instance()->GetRelationship(cs->ParentChit()->Team(), TEAM_VISITOR) != ERelate::ENEMY) 
		{
			GLASSERT(cores.HasCap());
			if (!exclude || (*exclude != sector)) {
				cores.Push(sector);
			}
		}
	}
	tree.Calc(cores.Mem(), cores.Size());
}
