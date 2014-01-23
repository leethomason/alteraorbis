#ifndef GL_MICRO_DB
#define GL_MICRO_DB

#include "gltypes.h"
#include "gldebug.h"
#include "glstringutil.h"

class XStream;

namespace grinliz {

/*	A key-value data store. (DataBase is stretching
	its definition.) Stores arbitrary keyed data.
	Uses very little memory if not used at all,
	and modest memory per key.

	Supports serialization via XStreams.

	When iterated, the keys are walked in the order added.
*/
class MicroDB
{
	friend class MicroDBIterator;
public:
	MicroDB()	{}
	MicroDB( const MicroDB& rhs ) {
		for( int i=0; i<rhs.dataArr.Size(); ++i ) {
			dataArr.Push( rhs.dataArr[i] );
		}
	}

	~MicroDB()	{}

	void operator=( const MicroDB& rhs ) {
		dataArr.Clear();
		for( int i=0; i<rhs.dataArr.Size(); ++i ) {
			dataArr.Push( rhs.dataArr[i] );
		}
	}
	void Clear() { dataArr.Clear(); }

	enum {
		NO_ERROR,
		WRONG_FORMAT,
		KEY_NOT_FOUND
	};

	/*
		S:	IString
		d:	int
		f:	float
	*/
	int Set(   const char* key, const char* fmt, ... );
	int Fetch( const char* key, const char* fmt, ... ) const;

	// convenience, for simple key-value pairs
	grinliz::IString	GetIString( const char* key ) const;
	int GetFloat( const char* key, float* value ) const;	///< doesn't write if not found.
	int GetInt( const char* key, int* value ) const;

	// Convenience: fetch a key, increment by 1, set key
	void Increment( const char* key );

	void Serialize( XStream* xs, const char* name );

private:
	struct Entry {
		Entry() { next[0] = next[1] = 0; }

		const char*	key;	// pointer to the IString pool
		int		nSub;		
		int		next[2];
	};

	struct SubEntry {
		char type;
		union {
			const char* str;
			float		floatVal;
			int			intVal;
		};
	};

	int Find( const IString& key, Entry** entry ) const;
	Entry* Set( const IString& key, int nSubKey, int* error );
	Entry* AppendEntry( const IString& key, int nSubKey );

	CDynArray< U8 > dataArr;
};


class MicroDBIterator
{
public:
	MicroDBIterator( const MicroDB& _db );

	bool Done()	const				{ return entry == 0; }
	void Next();

	const char* Key()				{ GLASSERT( entry ); return entry->key; }
	int NumSub() const				{ GLASSERT( entry ); return entry->nSub; }
	char SubType( int i ) const		{ GLASSERT( i<entry->nSub ); return subStart[i].type; }
	int Int( int i ) const			{ GLASSERT( i<entry->nSub ); GLASSERT( subStart[i].type == 'd' ); return subStart[i].intVal; }
	float Float( int i ) const		{ GLASSERT( i<entry->nSub ); GLASSERT( subStart[i].type == 'f' ); return subStart[i].floatVal; }
	const char* Str( int i ) const	{ GLASSERT( i<entry->nSub ); GLASSERT( subStart[i].type == 'S' ); return subStart[i].str; }

private:
	const MicroDB& db;
	const MicroDB::Entry*		entry;
	const MicroDB::SubEntry*	subStart;
};

};

#endif //  GL_MICRO_DB
