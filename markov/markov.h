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
	struct DoubletSort
	{
		static bool Less( const Triplet& v0, const Triplet& v1 )	{ 
			U32 a = (v0.value[0] << 8) + v0.value[1];
			U32 b = (v1.value[0] << 8) + v1.value[1];
			return a < b; 
		}
	};

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

private:
	void FindPair( char a, char b, int* start, int* count );

	grinliz::Random random;
	//grinliz::CDynArray< MarkovBuilder::Triplet > sortedArr;
	const MarkovBuilder::Triplet* arr;
	int nTriplets;
};

#endif // MARKOV_INCLUDED
