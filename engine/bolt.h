#ifndef LUMOS_BOLT_INCLUDED
#define LUMOS_BOLT_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glcontainer.h"

#include "vertex.h"

class Engine;

struct Bolt {
	Bolt() {
		head.Set( 0, 0, 0 );
		len = 0;
		dir.Set( 1, 0, 0 );
		impact = false;
		color.Set( 1, 1, 1, 1 );
	} 

	grinliz::Vector3F	head;
	float				len;
	grinliz::Vector3F	dir;	// normal vector
	bool				impact;	// 'head' is the impact location
	grinliz::Vector4F	color;
	//int					chitID;	// who fired this bolt

	static void TickAll( grinliz::CDynArray<Bolt>* bolts, U32 delta, Engine* engine );
};


class BoltRenderer
{
public:
	BoltRenderer();
	~BoltRenderer()	{}

	void DrawAll( const Bolt* bolts, int nBolts, Engine* engine );

private:
	enum { MAX_BOLTS = 400 };
	int nBolts;

	U16			index[MAX_BOLTS*6];
	PTCVertex	vertex[MAX_BOLTS*4];
};

#endif // LUMOS_BOLT_INCLUDED