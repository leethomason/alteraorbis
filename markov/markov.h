#ifndef MARKOV_INCLUDED
#define MARKOV_INCLUDED

#include "../grinliz/glstringutil.h"

class MarkovBuilder
{
public:
	MarkovBuilder();
	~MarkovBuilder();

	void Push( const grinliz::GLString& str );
};

#endif // MARKOV_INCLUDED
