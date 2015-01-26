#ifndef GRINLIZ_STREAMER_INCLUDED
#define GRINLIZ_STREAMER_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glutil.h"

// adaptors
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glmatrix.h"

class StreamReader;
class StreamWriter;

class XStream {
public:
	XStream();
	virtual ~XStream();

	//	version:int
	//	BeginNode [Attributes]
	//		BeginNode [Attributes]
	//		EndNode
	//		BeginNode
	//			BeginNode
	//			EndNode
	//		EndNode
	//	EndNode

	// chunks
	enum {
		READER_EOF = 0,
		BEGIN_ELEMENT,		// name:String,
		END_ELEMENT,
		ATTRIB_INT,
		ATTRIB_FLOAT,
		ATTRIB_DOUBLE,
		ATTRIB_STRING,

		ATTRIB_START = ATTRIB_INT,
		ATTRIB_END = ATTRIB_STRING+1
	};


	// Attrib:
	//		type: int
	//		name: string
	//		count: int
	//		values[count] of type
	virtual StreamWriter* Saving() { return 0; }
	virtual StreamReader* Loading() { return 0; }

protected:
	enum {
		ENC_0 = ATTRIB_END+1,
		ENC_1,
		ENC_INT,
		ENC_INT2,
		ENC_FLOAT,
		ENC_DOUBLE,
	};

	grinliz::HashTable< int, const char* >							indexToStr;
	// The string is not interned on lookup. Need the CompCharPtr
	grinliz::HashTable< const char*, int, grinliz::CompCharPtr >	strToIndex;
};


class StreamWriter : public XStream {
public:
	StreamWriter( FILE* p_fp, int version );
	~StreamWriter();

	virtual StreamWriter* Saving() { return this; }

	void OpenElement( const char* name );
	void CloseElement();

	void Set( const char* key, int value )			{ SetArr( key, &value, 1 ); }
	void Set( const char* key, float value )		{ SetArr( key, &value, 1 ); }
	void Set( const char* key, double value )		{ SetArr( key, &value, 1 ); }
	void Set( const char* key, const char* str )	{ SetArr( key, &str, 1 ); }

	void SetArr( const char* key, const int* value, int n );
	void SetArr( const char* key, const U8* value, int n );
	void SetArr( const char* key, const float* value, int n );
	void SetArr( const char* key, const double* value, int n );
	void SetArr( const char* key, const char* value[], int n );
	void SetArr( const char* key, const grinliz::IString* value, int n );

private:
	int  NumBytesFollow(int value) {
		int nBits = grinliz::LogBase2(value) + 1;
		int nBytes = (nBits + 3) / 8;
		return nBytes;
	}
	void WriteInt( int value );
	void WriteString( const char* str );
	void WriteFloat( float value );
	void WriteDouble( double value );
	void WriteReal(double value, bool isDouble);

	void WriteBuffer(const void* data, int size) {
		U8* p = buffer.PushArr(size);
		memcpy(p, data, size);
	}
	void Flush();
	grinliz::CDynArray<U8> buffer;

	FILE* file;
	int idPool;
	int depth;

	// Integers include integer values, markers in the stream, and string IDs
	int nCompInt;	
	int nInt;
	// Number of string bytes. Does not include integer reference.
	int nStr;
	// Raw number data.
	int nNumber;
};


class StreamReader : public XStream {
public:
	StreamReader( FILE* fp );
	~StreamReader()	{}

	int Version() const { return version; }
	virtual StreamReader* Loading() { return this; }

	struct Attribute {
		int type;
		const char* key;
		int n;
		int offset;

		bool operator==( const Attribute& a ) const { return strcmp( key, a.key ) == 0; }
		bool operator<( const Attribute& a ) const	{ return strcmp( key, a.key ) < 0; }
	};

	void Value( const Attribute* a, int* value, int size, int offset=0 ) const;
	void Value( const Attribute* a, float* value, int size, int offset=0 ) const;
	void Value( const Attribute* a, double* value, int size, int offset=0 ) const;
	void Value( const Attribute* a, U8* value, int size, int offset=0 ) const;
	void Value( const Attribute* a, grinliz::IString* value, int size, int offset=0 ) const;

	const char* Value( const Attribute* a, int index ) const;

	// returns the name of the element; next call must be OpenElement()
	// Allows for name = PeekElement(); switch(name)... OpenElement()
	const char*			PeekElement();		
	const char*			OpenElement();

	const Attribute*	Attributes() const			{ return attributes.Mem(); }
	int					NumAttributes() const		{ return attributes.Size(); }
	bool				HasChild();
	virtual void		CloseElement();

	const Attribute*	Get( const char* key );

private:
	int ReadInt();
	float ReadFloat();
	double ReadDouble();
	const char* ReadString();
	int PeekByte();
	double ReadReal();

	grinliz::CDynArray< char > strBuf;
	grinliz::CDynArray< int > intData;
	grinliz::CDynArray< float > floatData;
	grinliz::CDynArray< double > doubleData;
	grinliz::CDynArray< const char* > stringData;	// they are interned. can use const char*
	grinliz::CDynArray< Attribute > attributes;

	FILE* fp;
	int depth;
	int version;
	const char* peekElementName;
};

void DumpStream( StreamReader* reader, int depth );


inline void XarcOpen( XStream* stream, const char* name ) {
	if ( stream->Saving() ) {
		stream->Saving()->OpenElement( name );
	}
	else {
		const char* n = stream->Loading()->OpenElement();
		GLASSERT( grinliz::StrEqual( name, n ));
	}
}


inline void XarcClose( XStream* stream) {
	if ( stream->Saving() ) stream->Saving()->CloseElement();
	else stream->Loading()->CloseElement();
}


template< class T >
inline bool XarcGet( XStream* stream, const char* key, T &value )		{ 
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	if ( attr ) {
		stream->Loading()->Value( attr, &value, 1 );
		return true;
	}
	return false;
}

template< class T >
inline bool XarcGetArr( XStream* stream, const char* key, T* value, int n )		{ 
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	if ( attr ) {
		stream->Loading()->Value( attr, value, n );
		return true;
	}
	return false;
}

template< class T >
inline bool XarcGetArr( XStream* stream, const char* key, grinliz::Vector2<T>* value, int n ) {
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	if ( attr ) {
		stream->Loading()->Value( attr, &value->x, n*2 );
		return true;
	}
	return false;
}


template< class T >
void XarcSet( XStream* stream, const char* key, const T& value ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->Set( key, value );
}

template< class T >
void XarcSetArr( XStream* stream, const char* key, const T* value, int n ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->SetArr( key, value, n );
}


inline void XarcSetArr( XStream* stream, const char* key, const grinliz::IString* value, int n ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->SetArr( key, value, n );
}

template< class T >
inline void XarcSetArr( XStream* stream, const char* key, const grinliz::Vector2<T>* value, int n ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->SetArr( key, &value->x, n*2 );
}

// Adaptors.
// Basic types.
inline bool XarcGet( XStream* stream, const char* key, bool &value ) {
	U8 b = 0;
	if ( XarcGet( stream, key, b ) ) {
		value = b ? true : false;
		return true;
	}
	return false;
}

inline void XarcSet( XStream* stream, const char* key, bool value ) {
	U8 b = (value) ? 1 : 0;
	XarcSet( stream, key, b );
}

inline bool XarcGet( XStream* stream, const char* key, U32 &value ) {
	int v = 0;
	if ( XarcGet( stream, key, v )) {
		value = (U32)v;
		return true;
	}
	return false;
}

inline void XarcSet( XStream* stream, const char* key, U32 value ) {
	XarcSet( stream, key, (int)value );
}

// Strings
// Can serialize a null string.
bool XarcGet( XStream* xs, const char* key, grinliz::IString& i );
void XarcSet( XStream* xs, const char* key, const grinliz::IString& i );

// Vector
inline bool XarcGet( XStream* xs, const char* key, grinliz::Vector2I& v )			{ return XarcGetArr( xs, key, &v.x, 2  ); }
inline void XarcSet( XStream* xs, const char* key, const grinliz::Vector2I& v )		{ XarcSetArr( xs, key, &v.x, 2  ); }
inline bool XarcGet( XStream* xs, const char* key, grinliz::Vector2F& v )			{ return XarcGetArr( xs, key, &v.x, 2  ); }
inline void XarcSet( XStream* xs, const char* key, const grinliz::Vector2F& v )		{ XarcSetArr( xs, key, &v.x, 2  ); }
inline bool XarcGet( XStream* xs, const char* key, grinliz::Vector3F& v )			{ return XarcGetArr( xs, key, &v.x, 3  ); }
inline void XarcSet( XStream* xs, const char* key, const grinliz::Vector3F& v )		{ XarcSetArr( xs, key, &v.x, 3  ); }
inline bool XarcGet( XStream* xs, const char* key, grinliz::Vector4F& v )			{ return XarcGetArr( xs, key, &v.x, 4  ); }
inline void XarcSet( XStream* xs, const char* key, const grinliz::Vector4F& v )		{ XarcSetArr( xs, key, &v.x, 4  ); }
inline bool XarcGet( XStream* xs, const char* key, grinliz::Quaternion& v )			{ return XarcGetArr( xs, key, &v.x, 4  ); }
inline void XarcSet( XStream* xs, const char* key, const grinliz::Quaternion& v )	{ XarcSetArr( xs, key, &v.x, 4  ); }

// Rectangle2I
inline bool XarcGet( XStream* xs, const char* key, grinliz::Rectangle2I &v )		{ return XarcGetArr( xs, key, &v.min.x, 4 );}
inline void XarcSet( XStream* xs, const char* key, const grinliz::Rectangle2I& v )	{ XarcSetArr( xs, key, &v.min.x, 4 );}

// Matrix
bool XarcGet( XStream* xs, const char* key, grinliz::Matrix4 &v );
void XarcSet( XStream* xs, const char* key, const grinliz::Matrix4& v );

#define XARC_SER( stream, name ) {			\
	if ( (stream)->Saving() )				\
		XarcSet( stream, #name, name );		\
	else									\
		XarcGet( stream, #name, name );		\
}

#define XARC_SER_DEF( stream, name, defaultVal ) {	\
	if ( (stream)->Saving() ) {						\
		if ( name != defaultVal )					\
			XarcSet( stream, #name, name );			\
	}												\
	else {											\
		name = defaultVal;							\
		XarcGet( stream, #name, name );				\
	}												\
}

#define XARC_SER_KEY( stream, key, name ) {		\
	if ( (stream)->Saving() )					\
		XarcSet( stream, key, name );			\
	else										\
		XarcGet( stream, key, name );			\
}

#define XARC_SER_ARR( stream, name, n ) {			\
	if ( (stream)->Saving() )						\
		XarcSetArr( stream, #name, name, n );		\
	else											\
		XarcGetArr( stream, #name, name, n );		\
}

// Serialize a CArray or CDynArray with structures 
// with Serialize methods.
#define XARC_SER_CARRAY( stream, arr ) {					\
	if ( (stream)->Saving() ) {								\
		(stream)->Saving()->Set( #arr ".size", arr.Size() );\
	}														\
	else {													\
		int _size = 0;										\
		XarcGet( stream, #arr ".size", _size );				\
		arr.Clear();										\
		arr.PushArr( _size );								\
	}														\
	for( int _i=0; _i<arr.Size(); ++_i ) {					\
		arr[_i].Serialize( stream );						\
	}														\
}

// Serialize a CArray or CDynArray of primitives.
#define XARC_SER_VAL_CARRAY( stream, arr ) {				\
	if ( (stream)->Saving() ) {								\
		(stream)->Saving()->Set( #arr ".size", arr.Size()); \
		XarcSetArr(stream, #arr, arr.Mem(), arr.Size());	\
	}														\
	else {													\
		int _size = 0;										\
		XarcGet( stream, #arr ".size", _size );				\
		arr.Clear();										\
		arr.PushArr( _size );								\
		XarcGetArr(stream, #arr, arr.Mem(), arr.Size());	\
	}														\
}

#endif // GRINLIZ_STREAMER_INCLUDED
