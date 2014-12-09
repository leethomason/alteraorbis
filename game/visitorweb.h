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

	bool Empty() const { return edges.Empty(); }
	int NumLinks() const;
	int Link(int i, grinliz::Vector2I* s0, grinliz::Vector2I* s1);

private:
	struct WebLink {
		grinliz::Vector2I sector0, sector1;
		bool operator==(const WebLink& rhs) const {
			return (sector0 == rhs.sector0 && sector1 == rhs.sector1)
				|| (sector1 == rhs.sector0 && sector0 == rhs.sector1);
		}
	};

	grinliz::Vector2I origin;
	grinliz::CDynArray<WebLink> edges;
};


#endif // VISITOR_WEB_INCLUDED
