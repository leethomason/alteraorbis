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


#if defined(_MSC_VER) && defined(_M_IX86)
namespace grinliz {
	typedef U64 TimeUnit;

	inline U64 FastTime()
	{
		union 
		{
			U64 result;
			struct
			{
				U32 lo;
				U32 hi;
			} split;
		} u;
		u.result = 0;

		_asm {
			//pushad;	// don't need - aren't using "emit"
			cpuid;		// force all previous instructions to complete - else out of order execution can confuse things
			rdtsc;
			mov u.split.hi, edx;
			mov u.split.lo, eax;
			//popad;
		}				
		return u.result;
	}
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


static const int GL_MAX_SAMPLES = 10*1000;
static const int GL_MAX_PERFDATA = 100;

struct PerfData
{
	const char* name;
	int			depth;
	TimeUnit	inclusiveTU;
	double		inclusiveMSec;

	TimeUnit	start;
	enum { MAX_CHILDREN = 10 };
	PerfData* child[MAX_CHILDREN];
};


/**
*/
class Performance
{
  public:

	static void Push( const char* name, bool enter )	{ 
		if ( nSamples < GL_MAX_SAMPLES ) {
			samples[nSamples++].Set( name, enter, FastTime() );
		}
	}
	static void Free()		{ delete samples[]; samples = 0; nSamples = 0; }
	static void Clear()		{ nSamples = 0; }
	static void Process();

	static PerfData* perfRoot;

  protected:
	struct Sample
	{
		const char* name;
		bool		entry;
		TimeUnit	time;

		void Set( const char* _name, bool _enter, TimeUnit _time ) { name = _name; entry = _enter; time = _time; }
	};
	static Sample* samples;
	static int nSamples;

	static PerfData perfData[GL_MAX_PERFDATA];
	static int nPerfData;
};


struct PerformanceData
{
	PerformanceData( const char* _name ) : name( _name ) { Performance::Push( name, true ); }
	~PerformanceData()									 { Performance::Push( name, false ); }
	const char* name;
};


#ifdef GRINLIZ_PROFILE
	#define GRINLIZ_PERFTRACK PerformanceData data( __FUNCTION__ );
	#define GRINLIZ_PERFTRACK_NAME( x ) PerformanceData data( x );
#else
	#define GRINLIZ_PERFTRACK			{}
	#define GRINLIZ_PERFTRACK_NAME( x )	{}
#endif


};		

#endif
