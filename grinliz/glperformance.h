/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/



#ifndef GRINLIZ_PERFORMANCE_MEASURE
#define GRINLIZ_PERFORMANCE_MEASURE

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif
#if defined(__APPLE__)
//#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>
# endif

#include "gldebug.h"
#include "gltypes.h"
#include <stdio.h>


#if defined(_MSC_VER)
namespace grinliz {
	typedef U64 TimeUnit;

	U64 FastTime();
	U64 FastFrequency();
}

#elif defined(__GNUC__) && defined(__i386__) 
namespace grinliz {
	typedef U64 TimeUnit;

	inline U64 FastTime()
	{
		U64 val;
		 __asm__ __volatile__ ("rdtsc" : "=A" (val));
		 return val;
	}
}
#elif defined (__APPLE__) && defined(__arm__)
namespace grinliz {
	typedef U64 TimeUnit;
	inline U64 FastTime() {
		return mach_absolute_time();
	}
}
#else
#include <time.h>
namespace grinliz {
	typedef clock_t TimeUnit;
	inline TimeUnit FastTime() { return clock(); }
}
#endif

namespace grinliz {

class QuickProfile
{
public:
	QuickProfile( const char* name)		{ 
		startTime = FastTime(); 
		this->name = name;
	}
	~QuickProfile()		{ 
		U64 endTime = FastTime();	
		GLOUTPUT(( "%s %d MClocks\n", name, (int)((endTime-startTime)/(U64)(1000*1000)) ));
	}

private:
	TimeUnit startTime;
	const char* name;
};

		
static const int GL_MAX_SAMPLES = 1000*1000;
static const int GL_MAX_PERFDATA = 100;

struct PerfData
{
	const char* name;
	TimeUnit	inclusiveTU;
	double		inclusiveMSec;
	int			callCount;
	double		maxInclusiveMSec;

	void Clear() {
		inclusiveTU = 0;
		inclusiveMSec = 0;
		callCount = 0;
		start = 0;
	}

	TimeUnit	start;
	enum { MAX_CHILDREN = 10 };

	PerfData* parent;
	PerfData* child[MAX_CHILDREN];
};


class IPerformancePrinter
{
public:
	virtual void PrintPerf( int depth, const PerfData& data ) = 0;
};


/**
*/
class Performance
{
  public:

	static void Push( const char* name, bool enter )	{ 
		if ( !sampling )
			return;

		if ( !samples ) {
			samples = new Sample[GL_MAX_SAMPLES];
		}
		if ( nSamples < GL_MAX_SAMPLES ) {
			samples[nSamples++].Set( name, enter, FastTime() );
		}
	}
	static void Free()		{ delete [] samples; samples = 0; nSamples = 0; delete [] perfData; perfData = 0; nPerfData = 0; }
	
	// Commit the samples:
	static void Process();
	static void EndFrame()				{ framesSampled++; }
	// Print the last "processed()"
	static void Walk( IPerformancePrinter* printer ) { WalkRec( -1, perfData, printer ); }

	static void SetSampling( bool value )	{ sampling = value; }
	static void ClearSamples()				{ nSamples = 0; }

  protected:
	static void WalkRec( int depth, const PerfData* data, IPerformancePrinter* );
	struct Sample
	{
		const char* name;
		bool		entry;
		TimeUnit	time;

		void Set( const char* _name, bool _enter, TimeUnit _time ) { name = _name; entry = _enter; time = _time; }
	};
	static bool sampling;
	static Sample* samples;
	static int nSamples;

	static PerfData* perfData;
	static int nPerfData;
	static PerfData* root;
	static int framesSampled;
};


struct PerformanceTracker
{
	PerformanceTracker( const char* _name ) : name( _name ) { Performance::Push( name, true ); }
	~PerformanceTracker()									{ Performance::Push( name, false ); }
	const char* name;
};


#ifdef GRINLIZ_PROFILE
	#define GRINLIZ_PERFTRACK PerformanceTracker data( __FUNCTION__ );
	#define GRINLIZ_PERFTRACK_NAME( x ) PerformanceTracker data( x );
#else
	#define GRINLIZ_PERFTRACK			{}
	#define GRINLIZ_PERFTRACK_NAME( x )	{}
#endif


};		

#endif
