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
		ATTRIB_INT_ARRAY		= 0x08,
		ATTRIB_DOUBLE_ARRAY		= 0x10,
	};

	virtual StreamWriter* Saving() { return 0; }
	virtual StreamReader* Loading() { return 0; }

protected:
	grinliz::CDynArray< char >		names;
	grinliz::CDynArray< int >		intData;
};


class StreamWriter : public XStream {
public:
	StreamWriter( FILE* p_fp );
	~StreamWriter()					{}

	virtual StreamWriter* Saving() { return this; }

	void OpenElement( const char* name );
	void CloseElement();
	void Set( const char* key, int value );

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

		union {
			int intValue;
		};

		int Int() const { GLASSERT( type == ATTRIB_INT ); return intValue; }

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


inline void XarcGet( XStream* stream, const char* key, int &value )		{ 
	GLASSERT( stream->Loading() );
	const StreamReader::Attribute* attr = stream->Loading()->Get( key );
	value = attr->Int();
}


inline void XarcSet( XStream* stream, const char* key, int value ) {
	GLASSERT( stream->Saving() );
	stream->Saving()->Set( key, value );
}

#define XARC_SER( stream, name ) { if ( stream->Saving() ) XarcSet( stream, #name, name ); else XarcGet( stream, #name, name ); }


#endif // GRINLIZ_STREAMER_INCLUDED
