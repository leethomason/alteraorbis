#ifndef GL_MICRO_DB
#define GL_MICRO_DB

#include "gltypes.h"
#include "gldebug.h"
#include "glstringutil.h"

class XStream;

namespace grinliz {

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
	int Set( const char* key,  const char* fmt, ... );
	int Fetch( const char* key, const char* fmt, ... );

	void Serialize( XStream* xs, const char* name );

private:
	struct Entry {
		Entry() { next[0] = next[1] = 0; }

		IString	key;
		int		next[2];
	};
	// followed by c-str 'format'
	// followed by data (int, float, IString)

	CDynArray< U8 > dataArr;
};


class MicroDBIterator
{
public:
	MicroDBIterator( MicroDB* db );

	bool Done() const;

	const IString& Key();
	
};

};

#endif //  GL_MICRO_DB
