#ifndef LUMOS_VISITOR_INCLUDED
#define LUMOS_VISITOR_INCLUDED

// Each visitor has their own personality and memory, seperate
// from the chit AI.
struct VisitorData
{
	int id;	// chit id, and whether in-world or not.
};

#endif // LUMOS_VISITOR_INCLUDED
