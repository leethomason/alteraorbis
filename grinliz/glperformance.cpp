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


#ifdef _WIN32
	#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <stdio.h>

#include "gldebug.h"
#include "glperformance.h"
#include "glutil.h"

using namespace grinliz;
using namespace std;

Performance::Sample* Performance::samples = 0;
int Performance::nSamples = 0;
PerfData Performance::perfData;
int Performance::nPerfData = 0;


void Performance::Process()
{
	// Processing is rather tricky.
	// It's a map of a map...etc.
	nPerfData = 0;
	root = perfRoot = perfData;

	for( int i=0; i<nSamples; ++i ) {
		Sample* s = &samples[i];


	}
}

