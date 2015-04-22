#ifndef VISITOR_WEB_INCLUDED
#define VISITOR_WEB_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glcontainer.h"

struct WebLink {
	grinliz::Vector2I sector0, sector1;

	bool operator==(const WebLink& rhs) const { 
		return (sector0 == rhs.sector0 && sector1 == rhs.sector1) || (sector1 == rhs.sector0 && sector0 == rhs.sector1); 
	}
};


class MinSpanTree
{
public:
	MinSpanTree()	{}
	~MinSpanTree()	{}

	void Calc(const grinliz::Vector2I* points, int n);

	struct Node {
		grinliz::Vector2I pos;
		grinliz::Vector2I parentPos;
		float strength = 1;
		int  firstChild = 0;
		int nextSibling = 0;
	};

	int NumNodes() const { return nodes.Size(); }
	const Node& NodeAt(int i) const { return nodes[i]; }

	int CountChildren(const Node& node) const;
	const Node* ChildNode(const Node& parent, int n) const;

private:
	void RecWalkStr(int nodeIdx, float str);
	grinliz::CDynArray<Node> nodes;
};

class Web
{
public:
	Web()	{}
	~Web()	{}

	void Calc(const grinliz::Vector2I* exclude);
	void Calc(const grinliz::Vector2I* points, int n);

	int NumNodes() const { return tree.NumNodes(); }
	const MinSpanTree::Node& NodeAt(int i) const { return tree.NodeAt(i); }
	const MinSpanTree::Node* FindNode(const grinliz::Vector2I& v) const;
	int CountChildren(const MinSpanTree::Node& node) const { return tree.CountChildren(node); }
	const MinSpanTree::Node* ChildNode(const MinSpanTree::Node& parent, int n) const { return tree.ChildNode(parent, n); }
	bool Empty() const { return tree.NumNodes() < 2; }	// need more than just the origin.

private:
	MinSpanTree tree;
};


#endif // VISITOR_WEB_INCLUDED
