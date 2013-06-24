#ifndef LUMOS_VISITOR_INCLUDED
#define LUMOS_VISITOR_INCLUDED

class XStream;

// Each visitor has their own personality and memory, seperate
// from the chit AI.
struct VisitorData
{
	VisitorData() : id( 0 ) {}
	void Serialize( XStream* xs );
	int id;	// chit id, and whether in-world or not.
};


class Visitors
{
public:
	Visitors();
	~Visitors();

	void Serialize( XStream* xs );

	enum { NUM_VISITORS = 200 };
	VisitorData visitorData[NUM_VISITORS];

	// only returns existing; doesn't create.
	static Visitors* Instance()	{ return instance; }

private:
	static Visitors* instance;
};

#endif // LUMOS_VISITOR_INCLUDED
