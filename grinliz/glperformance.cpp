/*
Copyright (c) 2000-2010 Lee Thomason (www.grinninglizard.com)
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


#ifdef _MSC_VER
#include <windows.h>
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <stdio.h>

#include "gldebug.h"
#include "glperformance.h"
#include "glutil.h"
#include "glstringutil.h"

using namespace grinliz;
using namespace std;

#if defined(_MSC_VER)
U64 grinliz::FastTime()
{
	U64 t;
	QueryPerformanceCounter( (LARGE_INTEGER*) &t );
	return t;
}


U64 grinliz::FastFrequency()
{
	U64 f;
	QueryPerformanceFrequency( (LARGE_INTEGER*) &f );
	return f;
}
#endif

Performance::Sample* Performance::samples = 0;
int Performance::nSamples = 0;
PerfData* Performance::perfData = 0;
int Performance::nPerfData = 0;
PerfData* Performance::root = 0;


void Performance::EndFrame()
{
	if ( perfData ) {
		for( int i=0; i<nPerfData; ++i ) {
			PerfData* p = perfData + i;
			p->inclusiveTU = 0;
			p->inclusiveMSec = 0.f;
		}
	}
	nSamples = 0;
}


void Performance::Process()
{
	if ( !perfData ) {
		perfData = new PerfData[ GL_MAX_PERFDATA ];
	}
	memset( perfData, 0, sizeof(*perfData)*GL_MAX_PERFDATA );

	// Processing is rather tricky.
	// It's a map of a map...etc.
	nPerfData = 1;
	root = perfData;
	
	root->name = "main";

	for( int i=0; i<nSamples; ++i ) {
		Sample* s = &samples[i];

		if ( s->entry ) {
			int c = 0;
			for( ; c<PerfData::MAX_CHILDREN; ++c ) {
				if ( root->child[c] == 0 ) {
					// New call stack.
					root->child[c] = &perfData[nPerfData++];
					GLASSERT( nPerfData < GL_MAX_PERFDATA );
					root->child[c]->parent = root;
					break;
				}
				if ( StrEqual( root->child[c]->name, s->name ) ) {
					break;
				}
			}
			GLASSERT( c != PerfData::MAX_CHILDREN );
			
			root = root->child[c];
			root->name = s->name;
			root->start = s->time;
			root->callCount++;
		}
		else {
			GLASSERT( StrEqual( root->name, s->name ) );
			root->inclusiveTU += s->time - root->start;
			root = root->parent;
		}
	}

	double freq = 1000./(double)FastFrequency();
	for( int i=0; i<nPerfData; ++i ) {
		PerfData* pd = perfData+i;
		pd->inclusiveMSec = (double)(pd->inclusiveTU) * freq;
	}
}


void Performance::WalkRec( int depth, const PerfData* data, IPerformancePrinter* printer )
{
	GLASSERT( depth < 10 );
	if ( depth >= 0 ) {
		printer->PrintPerf( depth, *data );
	}
	for( int i=0; i<PerfData::MAX_CHILDREN; ++i ) {
		if ( data->child[i] ) {
			WalkRec(  depth+1, data->child[i], printer );
		}
	}
}
