#ifndef TESTMAP_INCLUDED
#define TESTMAP_INCLUDED

#include "../engine/map.h"
#include "../grinliz/glvector.h"

class TestMap : public Map
{
public:
	TestMap( int w, int h );
	~TestMap();

	virtual void Draw3D();

private:
	grinliz::Vector3F*	vertex;	// 4 vertex quads
	U16*				index;	// 6 index / quad

};

#endif // TESTMAP_INCLUDED