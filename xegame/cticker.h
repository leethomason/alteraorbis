#ifndef CTICKER_INCLUDED
#define CTICKER_INCLUDED

#include "../grinliz/glrandom.h"

class XStream;

class CTicker
{
public:
	CTicker( int _period ) : period(_period), time(0) {}

	int Delta( U32 d ) {
		int n = 0;
		time -= d;
		while ( time <= 0 ) {
			++n;
			time += period;
		}
		return n;
	}

	int Next() const { return time; }
	void Reset() { time = period; }
	void Randomize( int seed ) {
		grinliz::Random r(seed);
		r.Rand();
		time = r.Rand( period );
	}
	void Within( int t ) { 
		if ( t < time ) time = t;
	}

	void Serialize( XStream* xs, const char* name );

private:
	int period;
	int time;
};

#endif