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


#ifndef GRINLIZ_CONTAINER_INCLUDED
#define GRINLIZ_CONTAINER_INCLUDED

#include "gldebug.h"
#include "gltypes.h"

namespace grinliz
{


/* A dynamic array class for shallow c-structs. (No constructor, no destructor, 
	can be copied.) Basic number of objects are allocated in-line, so there is
	no initial allocation.
*/
template < class T, int CACHE=2 >
class CDynArray
{
public:
	CDynArray() : mem( cache ), size( 0 ), capacity( CACHE ) {
	}

	~CDynArray() {
		if ( mem != cache ) {
			free( mem );
		}
	}

	T& operator[]( int i )				{ GLASSERT( i>=0 && i<(int)size ); return mem[i]; }
	const T& operator[]( int i ) const	{ GLASSERT( i>=0 && i<(int)size ); return mem[i]; }

	void Push( T t ) {
		EnsureCap( size+1 );
		mem[size++] = t;
	}

	T* Push() {
		EnsureCap( size+1 );
		size++;
		return &mem[size-1];
	}

	T* PushArr( int count ) {
		EnsureCap( size+count );
		T* result = &mem[size];
		size += count;
		return result;
	}

	T Pop() {
		GLASSERT( size > 0 );
		return mem[--size];
	}

	void SwapRemove( int i ) {
		GLASSERT( i<(int)size );
		GLASSERT( size > 0 );
		grinliz::Swap( &mem[i], &mem[size-1] );
		Pop();
	}

	int Size() const		{ return size; }
	void Trim( int sz )		{ GLASSERT( sz <= size );
							  size = sz;
							}
	
	void Clear()			{ size = 0; }
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return mem; }
	T* Mem()				{ return mem; }

private:
	void EnsureCap( int count ) {
		if ( count > capacity ) {
			capacity = CeilPowerOf2( count );
			if ( mem == cache ) {
				mem = (T*) malloc( capacity*sizeof(T) );
				memcpy( mem, cache, size*sizeof(T) );
			}
			else {
				mem = (T*) realloc( mem, capacity*sizeof(T) );
			}
		}
	}

	T* mem;
	int size;
	int capacity;
	T cache[CACHE];
};





/* A fixed array class for c-structs.
 */
template < class T, int CAPACITY >
class CArray
{
public:
	CArray() : size( 0 )	{}
	~CArray()				{}

	T& operator[]( int i )				{ GLASSERT( i>=0 && i<(int)size ); return vec[i]; }
	const T& operator[]( int i ) const	{ GLASSERT( i>=0 && i<(int)size ); return vec[i]; }

	void Push( T t ) {
		GLASSERT( size < CAPACITY );
		vec[size++] = t;
	}

	T* PushArr( int n ) {
		GLASSERT( size+n <= CAPACITY );
		T* rst = &vec[size];
		size += n;
		return rst;
	}
	T* Push() {
		GLASSERT( size < CAPACITY );
		size++;
		return &vec[size-1];
	}

	unsigned Size() const	{ return size; }
	unsigned Capacity() const { return CAPACITY; }
	
	void Clear()			{ 
		#ifdef DEBUG
			memset( vec, 0xab, sizeof(T)*CAPACITY );
		#endif
		size = 0; 
	}
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return vec; }
	void SwapRemove( int i ) {
		GLASSERT( size > 0 );
		GLASSERT( i >= 0 && i < (int)size );
		vec[i] = vec[size-1];
		--size;
	}

private:
	T vec[CAPACITY];
	unsigned size;
};


}	// namespace grinliz
#endif
