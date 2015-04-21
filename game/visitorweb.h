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

class Web
{
public:
	Web()	{}
	~Web()	{}

	void Calc(const grinliz::Vector2I* exclude);
	void Calc(const grinliz::Vector2I* points, int n);

	const grinliz::Vector2I& Origin() const { return origin; }

	bool Empty() const		{ return edges.Empty(); }
	int NumEdges() const	{ return edges.Size(); }
	void Edge(int i, grinliz::Vector2I* s0, grinliz::Vector2I* s1) const;

	struct Node {
		Node() { sector.Zero(); strength = 1.0; }
		Node(const Node& node) { 
			sector = node.sector; 
			strength = node.strength;
			for (int i = 0; i < child.Size(); ++i) {
				child.Push(node.child[i]);
			}
		}

		grinliz::Vector2I			sector;
		grinliz::CDynArray<Node*>	child;
		float						strength;
	};

	int NumNodes() const				{ return nodes.Size(); }
	const Node* NodeAt(int i) const		{ return &nodes[i]; }

	const Node* Root() const			{ return nodes.Empty() ? 0 : nodes.Mem(); }
	const Node* FindNode(const grinliz::Vector2I& v) const;

private:
	struct WebLink {
		grinliz::Vector2I sector0, sector1;
		bool operator==(const WebLink& rhs) const {
			return (sector0 == rhs.sector0 && sector1 == rhs.sector1)
				|| (sector1 == rhs.sector0 && sector0 == rhs.sector1);
		}
	};

	void BuildNodeRec(Node* node, grinliz::CDynArray<Web::WebLink>* edgeSet );
	void DumpNodeRec(Node* node, int depth);

	grinliz::Vector2I origin;
	grinliz::CDynArray<WebLink> edges;
	grinliz::CDynArray<Node> nodes;
};


#endif // VISITOR_WEB_INCLUDED
