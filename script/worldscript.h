#ifndef WORLD_SCRIPT_INCLUDED
#define WORLD_SCRIPT_INCLUDED

#include "../grinliz/glcontainer.h"
#include "../grinliz/glrectangle.h"

class Chit;
class Engine;
class Model;

class WorldScript
{
public:
	// Return true if this is a "top level" chit that sits in the world.
	static Chit* IsTopLevel( Model* model );

	// Query for the top level chits in a rectangular area
	static void QueryChits( const grinliz::Rectangle2F& bounds, Engine* engine, grinliz::CDynArray<Chit*> *chitArr );

};


#endif // WORLD_SCRIPT_INCLUDED
