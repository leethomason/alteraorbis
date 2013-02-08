/*
Copyright (c) 2000-2012 Lee Thomason (www.grinninglizard.com)
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

#include <new>
#include <cstring>

#include "gldebug.h"
#include "gltypes.h"
#include "glutil.h"
//#include "glstringutil.h"

namespace grinliz
{


class ValueSem {
public:
	template <class T>
	static void DoRemove(T& v) {}
};


class OwnedPtrSem {
public:
	template <class T>
	static void DoRemove(T& v) { delete v; }
};


/*	A dynamic array class that supports C++ classes.
	Carefully manages construct / destruct.
	NOTE: The array may move around in memory; classes that contain
	      pointers to their own memory will be corrupted.
*/
template < class T, class SEM=ValueSem >
class CDynArray
{
	enum { CACHE = 4 };
public:
	CDynArray() : size( 0 ), capacity( 0 ), nAlloc(0) {
		mem = reinterpret_cast<T*>(cache);
		GLASSERT( CACHE_SIZE*sizeof(int) >= CACHE*sizeof(T) );
	}

	~CDynArray() {
		Clear();
		if ( mem != reinterpret_cast<T*>(cache) ) {
			Free( mem );
		}
		GLASSERT( nAlloc == 0 );
	}

	T& operator[]( int i )				{ GLASSERT( i>=0 && i<(int)size ); return mem[i]; }
	const T& operator[]( int i ) const	{ GLASSERT( i>=0 && i<(int)size ); return mem[i]; }

	void Push( const T& t ) {
		EnsureCap( size+1 );
		#pragma warning ( push )
		#pragma warning ( disable : 4345 )	// PODs will get constructors generated for them. Yes I know.
		new (mem+size) T( t );	// placement new copy constructor.
		#pragma warning ( pop )
		++size;
		++nAlloc;
	}


	void PushFront( const T& t ) {
		EnsureCap( size+1 );
		for( int i=size; i>=0; --i ) {
			mem[i] = mem[i-1];
		}
		mem[0] = t;
		++size;
		++nAlloc;
	}

	T* PushArr( int count ) {
		EnsureCap( size+count );
		T* result = &mem[size];
		for( int i=0; i<count; ++i ) {
			#pragma warning ( push )
			#pragma warning ( disable : 4345 )	// PODs will get constructors generated for them. Yes I know.
			new (result+i) T();	// placement new constructor
			#pragma warning ( pop )
		}
		size += count;
		nAlloc += count;
		return result;
	}

	T Pop() {
		GLASSERT( size > 0 );
		--size;
		--nAlloc;

		T temp = mem[size];
		SEM::DoRemove( mem[size] );
		mem[size].~T();
		return temp;
	}

	void Remove( int i ) {
		GLASSERT( i < (int)size );
		// Copy down.
		for( int j=i; j<size-1; ++j ) {
			mem[j] = mem[j+1];
		}
		// Get rid of the end:
		Pop();
	}

	void SwapRemove( int i ) {
		GLASSERT( i<(int)size );
		GLASSERT( size > 0 );
		
		mem[i] = mem[size-1];
		Pop();
	}

	int Find( const T& t ) {
		for( int i=0; i<size; ++i ) {
			if ( mem[i] == t )
				return i;
		}
		return -1;
	}

	int Size() const		{ return size; }
	
	void Clear()			{ 
		while( !Empty() ) 
			Pop();
		GLASSERT( nAlloc == 0 );
		GLASSERT( size == 0 );
	}
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return mem; }
	T* Mem()				{ return mem; }

	void EnsureCap( int count ) {
		if ( count > capacity ) {
			capacity = Max( CeilPowerOf2( count ), (U32) 16 );
			if ( mem == reinterpret_cast<T*>(cache) ) {
				mem = (T*)Malloc( capacity*sizeof(T) );
			}
			else {
				mem = (T*)Realloc( mem, capacity*sizeof(T) );
			}
		}
	}

	// Binary Search: array must be sorted!
	int BSearch( const T& t ) const {
		int low = 0;
		int high = Size();

		while (low < high) {
			int mid = low + (high - low) / 2;
			if ( mem[mid] < t )
				low = mid + 1; 
			else
				high = mid; 
		}
		if ((low < Size() ) && ( mem[low] == t))
			return low;

		return -1;
	}

private:
	CDynArray( const CDynArray<T>& );	// not allowed. Add a missing '&' in the code.

	T* mem;
	int size;
	int capacity;
	int nAlloc;
	enum { 
		CACHE_SIZE = (CACHE*sizeof(T)+sizeof(int)-1)/sizeof(int)
	};
	int cache[CACHE_SIZE];
};


/* A fixed array class for any type.
   Supports copy construction, proper destruction, etc.
   Does keep the objects around, until entire CArray is destroyed,

 */
template < class T, int CAPACITY >
class CArray
{
public:
	// construction
	CArray() : size( 0 )	{}
	~CArray()				{}

	// operations
	T& operator[]( int i )				{ GLASSERT( i>=0 && i<(int)size ); return mem[i]; }
	const T& operator[]( int i ) const	{ GLASSERT( i>=0 && i<(int)size ); return mem[i]; }

	// Push on
	void Push( const T& t ) {
		GLASSERT( size < CAPACITY );
		mem[size++] = t;
	}

	// Returns space to uninitialized objects.
	T* PushArr( int n ) {
		GLASSERT( size+n <= CAPACITY );
		T* rst = &mem[size];
		size += n;
		return rst;
	}

	T Pop() {
		GLASSERT( size > 0 );
		return mem[--size];
	}

	T PopFront() {
		GLASSERT( size > 0 );
		T temp = mem[0];
		for( int i=0; i<size-1; ++i ) {
			mem[i] = mem[i+1];
		}
		--size;
		return temp;
	}

	int Find( const T& key ) {
		for( int i=0; i<size; ++i ) {
			if ( mem[i] == key )
				return i;
		}
		return -1;
	}

	int Size() const		{ return size; }
	int Capacity() const	{ return CAPACITY; }
	bool HasCap() const		{ return size < CAPACITY; }
	
	void Clear()	{ 
		size = 0; 
	}
	bool Empty() const		{ return size==0; }
	T*		 Mem() 			{ return mem; }
	const T* Mem() const	{ return mem; }

	void SwapRemove( int i ) {
		GLASSERT( i >= 0 && i < (int)size );
		mem[i] = mem[size-1];
		Pop();
	}

private:
	T mem[CAPACITY];
	int size;
};


class CompValue {
public:
	// Hash table:
	template <class T>
	static U32 Hash( const T& v)					{ return (U32)(v); }
	template <class T>
	static bool Equal( const T& v0, const T& v1 )	{ return v0 == v1; }

	// Sort:
	template <class T>
	static bool Less( const T& v0, const T& v1 )	{ return v0 < v1; }
};


class CompCharPtr {
public:
	template <class T>
	static U32 Hash( T& _p) {
		const char* p = _p;
		U32 hash = 2166136261UL;
		for( ; *p; ++p ) {
			hash ^= *p;
			hash *= 16777619;
		}
		return hash;
	}
	template <class T>
	static bool Equal( T& v0, T& v1 ) { return StrEqual( v0, v1 ); }
};


template <class K, class V, class KCOMPARE=CompValue, class SEM=ValueSem >
class HashTable
{
public:
	HashTable() : nAdds(0), nItems(0), nBuckets(0), buckets(0), reallocating(false) {}
	~HashTable() 
	{ 
		RemoveAll();
		delete [] buckets;
	}

	// Adds a key/value pair. What about duplicates? Duplicate
	// keys aren't allowed. An old value will be deleted and
	// replaced.
	void Add( const K& key, const V& value ) 
	{
		values.Clear();
		EnsureCap();

		// Existing value?
		int index = FindIndex( key );
		if ( index >= 0 ) {
			// Replace!
			SEM::DoRemove( buckets[index].value );
			buckets[index].value = value;
		}
		else {
			U32 hash = KCOMPARE::Hash(key);
			while( true ) {
				hash = hash & (nBuckets-1);
				if ( buckets[hash].state == UNUSED || buckets[hash].state == DELETED ) {
					buckets[hash].state = IN_USE;
					buckets[hash].key   = key;
					buckets[hash].value = value;
					++nAdds;
					++nItems;
					break;
				}
				++hash;
			}
		}
	}

	V Remove( K key ) {
		int index = FindIndex( key );
		GLASSERT( index >= 0 );
		buckets[index].state = DELETED;
		--nItems;
		SEM::DoRemove( buckets[index].value );
		values.Clear();
		return buckets[index].value;
	}

	void RemoveAll() {
		for( U32 i=0; i<nBuckets; ++i ) {
			if ( buckets[i].state == IN_USE ) {
				SEM::DoRemove( buckets[i].value );
				--nItems;
			}
			buckets[i].state = UNUSED;
		}
		values.Clear();
		GLASSERT( nItems == 0 );
	}


	V Get( const K& key ) const {
		int index = FindIndex( key );
		GLASSERT( index >= 0 );
		return buckets[index].value;
	}

	bool Query( const K& key, V* value ) const {
		int index = FindIndex( key );
		if ( index >= 0 ) {
			if ( value ) {
				*value = buckets[index].value;
			}
			return true;
		}
		return false;
	}

	bool Empty() const		{ return nItems == 0; }
	int NumValues() const	{ return nItems; }

	V* GetValues() {
		// Create a cache of the values, so they can be a true array.
		if ( values.Empty() ) {
			for( U32 i=0; i<nBuckets; ++i ) {
				if ( buckets[i].state == IN_USE ) {
					values.Push( buckets[i].value );
				}
			}	
		}
		GLASSERT( values.Size() == nItems );
		return values.Mem();
	}

private:
	void EnsureCap() {
		if ( nAdds >= nBuckets*1/2 ) {
			GLASSERT( !reallocating );
			reallocating = true;
			Bucket* oldBuckets = buckets;
			U32 oldNBuckets = nBuckets;
			U32 oldNAdds = nAdds;
			U32 oldNItems = nItems;

			nBuckets = CeilPowerOf2( Max(	(U32)(nItems*4), 
											nAdds*2, 
											(U32) 128 ));
			buckets = new Bucket[nBuckets];

			nAdds = 0;
			nItems = 0;
			for( U32 i=0; i<oldNBuckets; ++i ) {
				if ( oldBuckets[i].state == IN_USE ) {
					Add( oldBuckets[i].key, oldBuckets[i].value );
				}
			}
			delete [] oldBuckets;
			values.Clear();
			reallocating = false;
		}
	}

	int FindIndex( const K& key ) const
	{
		if ( nBuckets > 0 ) {
			U32 hash = KCOMPARE::Hash( key );
			while( true ) {
				hash = hash & (nBuckets-1);
				if ( buckets[hash].state == IN_USE ) {
					const K& bkey = buckets[hash].key;
					if ( KCOMPARE::Equal( bkey, key )) {
						return hash;
					}
				}
				else if ( buckets[hash].state == UNUSED ) {
					return -1;
				}
				++hash;
			}
		}
		return -1;
	}

	U32 nAdds;
	int nItems;
	U32 nBuckets;
	bool reallocating;

	enum {
		UNUSED,
		IN_USE,
		DELETED
	};

	struct Bucket
	{
		Bucket() : state( UNUSED ) {}
		char	state;
		K		key;
		V		value;
	};
	Bucket *buckets;

	CDynArray< V > values;
};


// I'm completely intriqued by CombSort. QuickSort is
// fast, but an intricate algorithm that is easy to
// implement an "almost working" solution. CombSort
// is very simple, and almost as fast.
// good description: http://yagni.com/combsort/

inline int CombSortGap( int gap ) 
{
	GLASSERT( (gap & 0xff000000) == 0 );
	// shrink: 1.247
	// 9 or 10 go to 11 (awesome)
	//							   0  1  2  3  4  5  6  7  8  9 10 11  12  13  14  15
	static const int table[16] = { 1, 1, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 11, 11, 11, 12 };
	if ( gap < 16 ) {
		gap = table[gap];
	}
	else {
		gap = (gap * 103)>>7;
	}
	GLASSERT( gap  > 0 );
	return gap;
}

template <class T, class KCOMPARE >
inline void Sort( T* mem, int size )
{
	int gap = size;
	for (;;) {
		gap = CombSortGap(gap);
		bool swapped = false;
		const int end = size - gap;
		for (int i = 0; i < end; i++) {
			int j = i + gap;
			if ( KCOMPARE::Less(mem[j],mem[i]) ) {
				Swap(mem+i, mem+j);
				swapped = true;
			}
		}
		if (gap == 1 && !swapped) {
			break;
		}
	}
}


}	// namespace grinliz
#endif
