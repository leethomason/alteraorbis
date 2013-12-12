/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CTICKER_INCLUDED
#define CTICKER_INCLUDED

#include "../grinliz/glrandom.h"

class XStream;

class CTicker
{
public:
	explicit CTicker( int _period ) : period(_period), time(_period) {}

	// Sets time passed. Returns the number of times
	// that this ticker fired.
	int Delta( U32 d ) {
		int n = 0;
		time -= int(d);
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
	// Remember it counts *down*
	void SetTime( int t ) {
		time = t;
	}
	int Period() const { return period; }
	void SetPeriod( int t ) { period = t; }

	void Serialize( XStream* xs, const char* name );

private:
	int period;
	int time;
};

#endif