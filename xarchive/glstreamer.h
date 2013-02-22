#ifndef GRINLIZ_STREAMER_INCLUDED
#define GRINLIZ_STREAMER_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

// adaptors
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glmatrix.h"

class StreamReader;
class StreamWriter;
class Squisher;

class XStream {
public:
	XStream( bool squish );
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
		BEGIN_ATTRIBUTES,
		END_ATTRIBUTES,
		END_ELEMENT
	};

	// Attribute:
	//		BEGIN_ATTRIBUTES
	//		typeflags:byte
	//
	//		key characters:U16
	//		char array of keys
	//
	//		if ( INT ) size:byte, int array
	//		if ( FLOAT ) size:byte, float array
	//		numAttributes:byte
	//			[]	keyIndex:
	//				type:byte
	//				index:U16
	//				(optional) size:byte
	enum {
		ATTRIB_INT				= 0x01,
		ATTRIB_FLOAT			= 0x02,
		ATTRIB_DOUBLE			= 0x04,
		ATTRIB_BYTE				= 0x08,
		ATTRIB_STRING			= 0x10,
		ATTRIB_ZERO				= 0x20,	 // a numerical value that is zeros
	};

	virtual StreamWriter* Saving() { return 0; }
	virtual StreamReader* Loading() { return 0; }

protected:
	Squisher* squisher;
	grinliz::CDynArray< char >		names;
	grinliz::CDynArray< U8 >		byteData;
	grinliz::CDynArray< int >		intData;
	grinliz::CDynArray< float >		floatData;
	grinliz::CDynArray< double >	doubleData;
};


class StreamWriter : public XStream {
public:
	StreamWriter( FILE* p_fp );
	~StreamWriter();

	virtual StreamWriter* Saving() { return this; }

	void OpenElement( const char* name );
	void CloseElement();

	void Set( const char* key, int value );
	void Set( const char* key, float value );
	void Set( const char* key, double value );
	void Set( const char* key, U8 value );
	void Set( const char* key, const char* str );

	void SetArr( const char* key, const int* value, int n );
	void SetArr( const char* key, const float* value, int n );
	void SetArr( const char* key, const double* value, int n );
	void SetArr( const char* key, const U8* value, int n );

private:
	void WriteByte( int byte );
	void WriteU16( int value );
	void WriteInt( int value );
	void WriteString( const char* str );
	void FlushAttributes();

	template< class T >
	bool CheckZero( const T* data, int n ) {
		for( int i=0; i<n; ++i ) {
			if ( data[i] != 0 ) 
				return false;
		}
		return true;
	}

	struct Attrib {
		int type;
		int keyIndex;
		int index;
		int size;
	};

	struct CompAttrib {
		static const char* data;
		static bool Less( const Attrib& v0, const Attrib& v1 )	{ return strcmp( data+v0.keyIndex, data+v1.keyIndex ) < 0; }
	};

	Attrib* LowerSet( const char* key, int type, int n );
	grinliz::CDynArray< Attrib > attribs;

	FILE* fp;
	int depth;
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

		union {
			const int*		intArr;
			const float*	floatArr;
			const double*	doubleArr;
			const U8*		byteArr;
			const char*		str;
		};

		void Value( int* value ) const;
		void Value( float* value ) const;
		void Value( double* value ) const;
		void Value( U8* value ) const;
		const char* Str() const;

		void Value( int* value, int size ) const;
		void Value( float* value, int size ) const;
		void Value( double* value, int size ) const;
		void Value( U8* value, int size ) const;

		bool operator==( const Attribute& a ) const { return strcmp( key, a.key ) == 0; }
		bool operator<( const Attribute& a ) const	{ return strcmp( key, a.key ) < 0; }
	};

	const char*			OpenElement();
	const Attribute*	Attributes() const			{ return attributes.Mem(); }
	int					NumAttributes() const		{ return attributes.Size(); }
	bool				HasChild();
	virtual void		CloseElement();

	const Attribute*	Get( const char* key );

private:
	int ReadByte();
	int ReadInt();
	int ReadU16();
	int PeekByte();
	void ReadString( grinliz::CDynArray<char>* target );

	grinliz::CDynArray<char> elementName;
	grinliz::CDynArray< Attribute >	attributes;

	FILE* fp;
	int depth;
	int version;
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
		attr->Value( &value );
		return true;
	}
	return false;
}

template< class T >
inline bool XarcGetArr( XStream* stream, const char* key, T* value, int n )		{ 
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	if ( attr ) {
		attr->Value( value, n );
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
inline bool XarcGet( XStream* xs, const char* key, grinliz::Matrix4 &v )			{ return XarcGetArr( xs, key, v.x, 16 );}
inline void XarcSet( XStream* xs, const char* key, const grinliz::Matrix4& v )		{ XarcSetArr( xs, key, v.x, 16 );}

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

#define XARC_SER_ARR( stream, name, n ) {		\
	if ( (stream)->Saving() )						\
		XarcSetArr( stream, #name, name, n );	\
	else										\
		XarcGetArr( stream, #name, name, n );		\
}


#endif // GRINLIZ_STREAMER_INCLUDED
