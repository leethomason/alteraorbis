/*
Copyright (c) 2000-2007 Lee Thomason (www.grinninglizard.com)

Grinning Lizard Utilities. Note that software that uses the 
utility package (including Lilith3D and Kyra) have more restrictive
licences which applies to code outside of the utility package.


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
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#endif	//_WIN32

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#include "gldebug.h"
#include <stdio.h>
#include <time.h>
#include "glstringutil.h"

bool gDebugging = true;

#ifdef DEBUG

#if defined(_MSC_VER)
	#define GRINLIZ_DEBUG_MEM
#endif
//#define GRINLIZ_DEBUG_MEM_DEEP

#ifdef GRINLIZ_DEBUG_MEM

size_t memTotal = 0;
size_t memWatermark = 0;
long memNewCount = 0;
long memDeleteCount = 0;
long memMallocCount = 0;
long memFreeCount = 0;
unsigned long MEM_MAGIC0 = 0xbaada55a;
unsigned long MEM_MAGIC1 = 0xbaada22a;
unsigned long MEM_DELETED0 = 0x12345678;
unsigned long MEM_DELETED1 = 0x9ABCDEF0;
unsigned long idPool = 0;
bool checking = false;
int distribution[32] = { 0 };

struct MemCheckHead
{
	size_t size;
	bool arrayType;
	unsigned long id;
	const char* name;
	int line;

	MemCheckHead* next;
	MemCheckHead* prev;
	unsigned long magic;
};

MemCheckHead* root = 0;

struct MemCheckTail
{
	unsigned long magic;
};


struct MTrack
{
	const void* mem;
	size_t size;
};

MTrack* mtrackRoot = 0;
unsigned long nMTrack = 0;
unsigned long mTrackAlloc = 0;

U32 logBase2( U32 v ) 
{
	// I don't love this implementation, and I
	// don't love the table alternatives either.
	U32 r=0;
	while ( v >>= 1 ) {
		++r;
	}
	if ( r == 24 ) {
		int debug = 1;
	}
	return r;
}

#ifdef GRINLIZ_DEBUG_MEM

void TrackMalloc( const void* mem, size_t size )
{
	if ( nMTrack == mTrackAlloc ) {
		mTrackAlloc += 64;
		mtrackRoot = (MTrack*)realloc( mtrackRoot, mTrackAlloc*sizeof(MTrack) );
	}
	mtrackRoot[ nMTrack ].mem = mem;
	mtrackRoot[ nMTrack ].size = size;
	++nMTrack;

	memTotal += size;
	if ( memTotal > memWatermark )
		memWatermark = memTotal;
	
	memMallocCount++;
	U32 log2 = logBase2(size);
	GLASSERT( log2 < 32 );
	distribution[ log2 ] += 1;
}


void TrackFree( const void* mem )	
{
	for( unsigned long i=0; i<nMTrack; ++i ) {
		if ( mtrackRoot[i].mem == mem ) {
			memTotal -= mtrackRoot[i].size;
			memFreeCount++;
			mtrackRoot[i] = mtrackRoot[nMTrack-1];
			--nMTrack;
			return;
		}
	}
	GLASSERT( 0 );
}


void* DebugNew( size_t size, bool arrayType, const char* name, int line )
{
	void* mem = 0;
	if ( size == 0 )
		size = 1;

	GLASSERT( size );
	size_t allocateSize = size + sizeof(MemCheckHead) + sizeof(MemCheckTail);
	mem = malloc( allocateSize );

	MemCheckHead* head = (MemCheckHead*)(mem);
	MemCheckTail* tail = (MemCheckTail*)((unsigned char*)mem+sizeof(MemCheckHead)+size);
	void* body = (void*)((unsigned char*)mem+sizeof(MemCheckHead));
	GLASSERT( body );

	head->size = size;
	head->arrayType = arrayType;
	head->id = idPool++;
	head->name = name;
	head->line = line;

	head->magic = MEM_MAGIC0;
	tail->magic = MEM_MAGIC1;
	head->prev = head->next = 0;

	distribution[ logBase2(size) ] += 1;

	if ( checking )
	{
		++memNewCount;
		memTotal += size;
		if ( memTotal > memWatermark )
			memWatermark = memTotal;

		if ( root )
			root->prev = head;
		head->next = root;
		head->prev = 0;
		root = head;
	}

	// #BREAKHERE
	if ( head->id == 18996 ) {
		int debug = 1;
	}

#ifdef GRINLIZ_DEBUG_MEM_DEEP
	MemHeapCheck();
#endif
	return body;
}


void DebugDelete( void* mem, bool arrayType )
{
#ifdef GRINLIZ_DEBUG_MEM_DEEP
	MemHeapCheck();
#endif
	if ( mem ) {
		MemCheckHead* head = (MemCheckHead*)((unsigned char*)mem-sizeof(MemCheckHead));
		MemCheckTail* tail = (MemCheckTail*)((unsigned char*)mem+head->size);

		// For debugging, so if the asserts do fire, we still have a copy of the values.
		MemCheckHead aHead = *head;
		MemCheckTail aTail = *tail;

		GLASSERT( head->magic == MEM_MAGIC0 );
		GLASSERT( tail->magic == MEM_MAGIC1 );
		
		// This doesn't work as I expect.
		// array allocation of primitive types: uses single form.
		// array allocation of class types: uses array form.
		// so an array destructor is meaningless - could go either way.
		// can detect array destructor of single construct...
		//GLASSERT( head->arrayType == arrayType || !head->arrayType );

		if ( head->prev )
			head->prev->next = head->next;
		else
			root = head->next;

		if ( head->next )
			head->next->prev = head->prev;
		
		if ( checking )
		{
			++memDeleteCount;
			memTotal -= head->size;
		}
		head->magic = MEM_DELETED0;
		tail->magic = MEM_DELETED1;
		free(head);
	}
}
#endif

void* operator new( size_t size ) 
{
	return DebugNew( size, false, 0, 0 );
}

void* operator new[]( size_t size ) 
{
	return DebugNew( size, true, 0, 0 );
}

void operator delete[]( void* mem ) 
{
	DebugDelete( mem, true );
}

void operator delete( void* mem ) 
{
	DebugDelete( mem, false );
}

void MemLeakCheck()
{
	GLOUTPUT((	"MEMORY REPORT: watermark=%dk =%dM new count=%d. delete count=%d. %d allocations leaked.\n",
				(int)(memWatermark/1024), (int)(memWatermark/(1024*1024)),
				(int)memNewCount, (int)memDeleteCount, (int)(memNewCount-memDeleteCount) ));
	GLOUTPUT((	"               malloc count=%d. free count=%d. %d allocations leaked.\n",
				(int)memMallocCount, (int)memFreeCount, (int)(memMallocCount-memFreeCount) ));

	for( int i=0; i<30; ++i ) {
		U32 m32 = 1<<i;
		U32 div = 1;
		char suffix = 'b';
		if ( m32 >= 1024 && m32 < (1024*1024) ) {
			suffix = 'k';
			div = 1024;
		}
		else if ( m32 >= (1024*1024) ) {
			suffix = 'M';
			div = 1024*1024;
		}
		GLOUTPUT(( "  distribution[%2d] %3d%c nAlloc=%5d mem=%8.1fM\n",
				   i, 
				   m32/div, suffix,
				   distribution[i], 
				   (float)(distribution[i]*m32)/(float)(1024*1024) ));
	}

	for( MemCheckHead* node = root; node; node=node->next )
	{
		GLOUTPUT(( "  size=%d %s id=%d name=%s line=%d\n",
					(int)node->size, (node->arrayType) ? "array" : "single", (int)node->id,
					(node->name) ? node->name :  "(null)", node->line ));
	}		  

	for( unsigned long i=0; i<nMTrack; ++i ) {
		GLOUTPUT(( "  malloc size=%d\n", mtrackRoot[i].size ));
	}

	/*
		If these fire, then a memory leak has been detected. The library doesn't track
		the source. The best way to find the source is to break in the allocator,
		search for: #BREAKHERE. Each allocation has a unique ID and is printed out above.
		Once you know the ID of the thing you are leaking, you can track the source of the leak.

		It's not elegant, but it does work.
	*/
	GLASSERT( memNewCount-memDeleteCount == 0 && !root && !nMTrack );
	GLASSERT( memMallocCount - memFreeCount == 0 );

	// If this fires, the code isn't working or you never allocated memory:
	GLASSERT( memNewCount );
}

void MemHeapCheck()
{
	// FIXME: add in walk of malloc/free list
	unsigned long size = 0;
	for ( MemCheckHead* head = root; head; head=head->next )
	{
		MemCheckTail* tail = (MemCheckTail*)((unsigned char*)head + sizeof(MemCheckHead) + head->size);

		GLASSERT( head->magic == MEM_MAGIC0 );
		GLASSERT( tail->magic == MEM_MAGIC1 );
		size += head->size;
	}
	GLASSERT( size == memTotal );
}

void MemStartCheck()
{
	checking = true;
}


#endif // GRINLIZ_DEBUG_MEM
#endif // DEBUG

#ifdef _WIN32

void WinDebugBreak()
{
	DebugBreak();
}

void dprintf( const char* format, ... )
{
    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    vsprintf_s( buffer, 1024, format, va );
#else
    vsnprintf( buffer, 1024, format, va );
#endif
    va_end( va );

	OutputDebugStringA( buffer );

	printf( "%s", buffer );
	fflush( 0 );
}


FILE* logFP = 0;

void logprintf( const char* format, ... )
{
    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    vsprintf_s( buffer, 1024, format, va );
#else
    vsnprintf( buffer, 1024, format, va );
#endif
    va_end( va );

	if ( !logFP ) {
		logFP = fopen( "test_log.txt", "w" );
	}
	fprintf( logFP, "%s", buffer );
}

FILE* relFP = 0;

void relprintf( const char* format, ... )
{
    va_list     va;
    char		buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    vsprintf_s( buffer, 1024, format, va );
#else
    vsnprintf( buffer, 1024, format, va );
#endif
    va_end( va );

	if ( !relFP ) {
		relFP = fopen( "release_log.txt", "w" );
	}
	fprintf( relFP, "%s", buffer );
	fflush( relFP );
}
#endif


#if defined(DEBUG) && defined(ANDROID_NDK)
void dprintf( const char* format, ... )
{
    va_list     va;

    //
    //  format and output the message..
    //
    va_start( va, format );
	__android_log_vprint( ANDROID_LOG_INFO, "grinliz", format, va );
    va_end( va );

}
#endif
