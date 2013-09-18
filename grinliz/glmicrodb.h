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
*/
class MicroDB
{
public:
	MicroDB()	{}
	~MicroDB()	{}

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
	int Fetch( const char* key, const char* fmt, ... );

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

	Entry* Set( const IString& key, int nSubKey, int* error );
	Entry* AppendEntry( const IString& key, int nSubKey );

	CDynArray< U8 > dataArr;
};


};

#endif //  GL_MICRO_DB
