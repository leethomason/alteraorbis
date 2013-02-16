#ifndef GRINLIZ_STREAMER_INCLUDED
#define GRINLIZ_STREAMER_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"


class StreamReader;
class StreamWriter;

class XStream {
public:
	XStream() {}
	virtual ~XStream() {}

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
	};

	virtual StreamWriter* Saving() { return 0; }
	virtual StreamReader* Loading() { return 0; }

protected:
	grinliz::CDynArray< char >		names;
	grinliz::CDynArray< U8 >		byteData;
	grinliz::CDynArray< int >		intData;
	grinliz::CDynArray< float >		floatData;
	grinliz::CDynArray< double >	doubleData;
};


class StreamWriter : public XStream {
public:
	StreamWriter( FILE* p_fp );
	~StreamWriter()					{}

	virtual StreamWriter* Saving() { return this; }

	void OpenElement( const char* name );
	void CloseElement();

	void Set( const char* key, int value );
	void Set( const char* key, float value );
	void Set( const char* key, double value );
	void Set( const char* key, U8 value );
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
			//int intValue;
			//float floatValue;
			//double doubleValue;
			//U8 byteValue;

			const int* intArr;
			const float* floatArr;
			const double* doubleArr;
			const U8* byteArr;
		};

		void Value( int* value ) const		{ GLASSERT( type == ATTRIB_INT );	GLASSERT( n == 1 );	*value = intArr[0]; }
		void Value( float* value ) const	{ GLASSERT( type == ATTRIB_FLOAT );	GLASSERT( n == 1 );	*value = floatArr[0]; }
		void Value( double* value ) const	{ GLASSERT( type == ATTRIB_DOUBLE );GLASSERT( n == 1 ); *value = doubleArr[0]; }
		void Value( U8* value ) const		{ GLASSERT( type == ATTRIB_BYTE );	GLASSERT( n == 1 ); *value = byteArr[0]; }

		void Value( int* value, int size ) const	{	GLASSERT( type == (ATTRIB_INT) ); 
														GLASSERT( n == size );		
														memcpy( value, intArr, n*sizeof(int) ); }
		void Value( float* value, int size ) const	{	GLASSERT( type == (ATTRIB_FLOAT) ); 
														GLASSERT( n == size );		
														memcpy( value, floatArr, n*sizeof(float) ); }
		void Value( double* value, int size ) const	{	GLASSERT( type == (ATTRIB_DOUBLE) ); 
														GLASSERT( n == size );		
														memcpy( value, doubleArr, n*sizeof(double) ); }
		void Value( U8* value, int size ) const		{	GLASSERT( type == (ATTRIB_BYTE) ); 
														GLASSERT( n == size );		
														memcpy( value, byteArr, n ); }

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
inline void XarcGet( XStream* stream, const char* key, T &value )		{ 
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	attr->Value( &value );
}

template< class T >
inline void XarcGetArr( XStream* stream, const char* key, T* value, int n )		{ 
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	attr->Value( value, n );
}

template< class T >
void XarcSet( XStream* stream, const char* key, T value ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->Set( key, value );
}

template< class T >
void XarcSetArr( XStream* stream, const char* key, const T* value, int n ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->SetArr( key, value, n );
}

#define XARC_SER( stream, name ) {			\
	if ( stream->Saving() )					\
		XarcSet( stream, #name, name );		\
	else									\
		XarcGet( stream, #name, name );		\
}

#define XARC_SER_ARR( stream, name, n ) {		\
	if ( stream->Saving() )						\
		XarcSetArr( stream, #name, name, n );	\
	else										\
		XarcGetArr( stream, #name, name, n );		\
}


#endif // GRINLIZ_STREAMER_INCLUDED
