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

#ifndef MARKOV_INCLUDED
#define MARKOV_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glrandom.h"

class MarkovBuilder
{
public:
	struct Triplet
	{
		static U32 Hash( const Triplet& v)							{ return v.All(); }
		static bool Equal( const Triplet& v0, const Triplet& v1 )	{ return v0.All() == v1.All(); }

		Triplet() {
			value[0] = value[1] = value[2] = value[3] = 0;
		}
		U32 All() const { return *((U32*)value); }

		char value[4];	// a,b,c,0
	};

	MarkovBuilder()		{}
	~MarkovBuilder()	{}

	void Push( const grinliz::GLString& str );
	void Process();
	const grinliz::CDynArray< Triplet >& Triplets() const	{ return sortedArr; }
	const char* Data() const								{ return (const char*)sortedArr.Mem(); }
	int NumBytes() const									{ return sortedArr.Size()*sizeof(Triplet); }

private:
	grinliz::CDynArray< grinliz::GLString >			names;
	grinliz::HashTable< Triplet, Triplet, Triplet > table;
	grinliz::CDynArray< Triplet >					sortedArr;
	
};


class MarkovGenerator
{
public:
	// This class operates on a pointer to the data - no
	// local copy - good to be aware.
	MarkovGenerator( const char* data, int nBytes, int seed );
	bool Name( grinliz::GLString* name, int maxLen );
	void Analyze();

private:
	enum {MAX_BUF = 64};
	
	void FindPair( char a, char b, int* start, int* count );
	void AnalyzeRec(char* buf, int nChar, int* options);

	grinliz::Random random;
	const MarkovBuilder::Triplet* arr;
	int nTriplets;
};

#endif // MARKOV_INCLUDED
