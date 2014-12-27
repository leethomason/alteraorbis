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

	void Calc(const grinliz::Vector2I* points, int n);

	const grinliz::Vector2I& Origin() const { return origin; }

	bool Empty() const		{ return edges.Empty(); }
	int NumEdges() const	{ return edges.Size(); }
	void Edge(int i, grinliz::Vector2I* s0, grinliz::Vector2I* s1) const;

	struct Node {
		Node() { sector.Zero(); }
		Node(const Node& node) { 
			sector = node.sector; 
			for (int i = 0; i < adjacent.Size(); ++i) {
				adjacent.Push(node.adjacent[i]);
			}
		}

		grinliz::Vector2I sector;
		grinliz::CDynArray<Node*> adjacent;
	};
	const Node* Root() const { return nodes.Empty() ? 0 : nodes.Mem(); }
	const Node* FindNode(const grinliz::Vector2I& v) const;

private:
	struct WebLink {
		grinliz::Vector2I sector0, sector1;
		bool operator==(const WebLink& rhs) const {
			return (sector0 == rhs.sector0 && sector1 == rhs.sector1)
				|| (sector1 == rhs.sector0 && sector0 == rhs.sector1);
		}
	};

	void BuildNodeRec(grinliz::Vector2I pos, Node* node, grinliz::CDynArray<bool>* edgeSet );

	grinliz::Vector2I origin;
	grinliz::CDynArray<WebLink> edges;
	grinliz::CDynArray<Node> nodes;
};


#endif // VISITOR_WEB_INCLUDED
