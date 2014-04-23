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

	void Set( const char* key, int value );
	void Set( const char* key, float value );
	void Set( const char* key, const IString& value );

	void Set( const IString& key, int value );
	void Set( const IString& key, float value );
	void Set( const IString& key, const IString& value );

	int Get( const char* key, int* value ) const;
	int Get( const char* key, float* value ) const;
	int Get( const char* key, IString* value ) const;

	int Get( const IString& key, int* value ) const;
	int Get( const IString& key, float* value ) const;
	int Get( const IString& key, IString* value ) const;

	IString GetIString( const char* key ) const;
	IString GetIString( const IString& key ) const;

	// Convenience: fetch a key, increment by 1, set key
	void Increment( const char* key );
	// Fetch a value; add to it, return the result
	int  Add( const char* key, int value );

	void Serialize( XStream* xs, const char* name );

	enum {
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_ISTRING
	};

private:

	struct Entry {
		void Serialize( XStream* xs );
		grinliz::IString	key;
		int					type;
		grinliz::IString	value;		// special: _FLOAT_, _INT_
		union {
			float		floatVal;
			int			intVal;
		};
	};

	Entry* FindOrCreate( const IString& key );
	const Entry* Find( const IString& key ) const;

	CDynArray< Entry > dataArr;
};


class MicroDBIterator
{
public:
	MicroDBIterator( const MicroDB& _db ) : db(_db), index(0) {}

		bool Done() const				{ return index >= db.dataArr.Size(); }
		void Next()						{ ++index; }

		const char* Key() const         { GLASSERT( !Done() ); return db.dataArr[index].key.c_str(); }
		int			Type() const		{ GLASSERT( !Done() ); return db.dataArr[index].type; }
		int			IntValue() const	{ GLASSERT( !Done() ); GLASSERT( db.dataArr[index].type == MicroDB::TYPE_INT ); return db.dataArr[index].intVal; }
		float		FloatValue() const	{ GLASSERT( !Done() ); GLASSERT( db.dataArr[index].type == MicroDB::TYPE_FLOAT ); return db.dataArr[index].floatVal; }
		const char* StrValue() const	{ GLASSERT( !Done() ); GLASSERT( db.dataArr[index].type == MicroDB::TYPE_ISTRING ); return db.dataArr[index].value.c_str(); }

private:
        const MicroDB&	db;
		int				index;
};

};

#endif //  GL_MICRO_DB
