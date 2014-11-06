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


#ifndef GRINLIZ_DEBUG_INCLUDED
#define GRINLIZ_DEBUG_INCLUDED

#if defined( _DEBUG ) || defined( DEBUG ) || defined (__DEBUG__)
	#ifndef DEBUG
		#define DEBUG
	#endif
#endif

#include <stdlib.h>
#include <stdio.h>

extern bool gDebugging;	// global debugging flag

void SetReleaseLog(FILE* fp);
void SetCheckGLError(bool error);

void relprintf( const char* format, ... );
#if defined(DEBUG)
	#if defined(_MSC_VER)
		void dprintf( const char* format, ... );
		void logprintf( const char* format, ... );
		void WinDebugBreak();
		
		#define GLASSERT( x )		if ( !(x)) { _asm { int 3 } }
		inline bool GL_FUNC_ASSERT(bool x)	{ GLASSERT(x); return x; }
		#define GLOUTPUT( x )		dprintf x
		#define GLOUTPUT_REL( x )	relprintf x
	#elif defined (ANDROID_NDK)
		#include <android/log.h>
		void dprintf( const char* format, ... );
		#define GLASSERT( x )		if ( !(x)) { __android_log_assert( "assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__ ); }
		#define	GLOUTPUT( x )		dprintf x
	#else
		#include <assert.h>
        #include <stdio.h>
		#define GLASSERT		assert
		#define GLOUTPUT( x )	printf x	
	#endif
#else
	#define GLOUTPUT( x )
	#define GLLOG( x )
	#define GLOUTPUT_REL( x )	relprintf x
	#define GLASSERT( x )		if ( gDebugging && (!(x))) { relprintf( "ASSERT in '%s' at %d.\n", __FILE__, __LINE__ ); relprintf( "ASSERT: %s\n", #x ); }
#endif

#if defined(DEBUG)
	
	#if defined(_MSC_VER)
		#define GRINLIZ_DEBUG_MEM
	#endif

	#ifdef GRINLIZ_DEBUG_MEM
		void* DebugNew( size_t size, bool arrayType, const char* name, int line );
		
		inline void* operator new[]( size_t size, const char* file, int line ) {
			return DebugNew( size, true, file, line );
		}

		inline void* operator new( size_t size, const char* file, int line ) {
			return DebugNew( size, false, file, line );
		}

		void TrackMalloc( const void*, size_t size );
		//void TrackRealloc( const void*, size_t newSize );
		void TrackFree( const void* );

		inline void* Malloc( size_t size ) {
			void* v = malloc( size );
			TrackMalloc( v, size );
			return v;
		}

		inline void* Realloc( void* v, size_t size ) {
			if ( v ) {
				TrackFree( v );
			}
			v = realloc( v, size );
			TrackMalloc( v, size );
			return v;
		}

		inline void Free( void* v ) {
			TrackFree( v );
			free( v );
		}

		void MemLeakCheck();
		void MemStartCheck();
		void MemHeapCheck();
	#else
		inline void MemLeakCheck()	{}
		inline void MemStartCheck()	{}
		inline void MemHeapCheck()	{}
		inline void TrackMalloc( const void*, size_t size )	{}
		inline void TrackFree( const void* )	{}
		inline void* Malloc( size_t size ) {
			void* v = malloc( size );
			return v;
		}

		inline void* Realloc( void* v, size_t size ) {
			v = realloc( v, size );
			return v;
		}

		inline void Free( void* v ) {
			free( v );
		}
	#endif
#else
	inline void MemLeakCheck()	{}
	inline void MemStartCheck()	{}
	inline void* Malloc( size_t size ) {
		void* v = malloc( size );
		return v;
	}

	inline void* Realloc( void* v, size_t size ) {
		v = realloc( v, size );
		return v;
	}

	inline void Free( void* v ) {
		free( v );
	}
#endif

#endif // file
