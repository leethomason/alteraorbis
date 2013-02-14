#ifndef GRINLIZ_STREAMER_INCLUDED
#define GRINLIZ_STREAMER_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"


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

	enum {
		STR_LEN = 16
	};

	virtual void OpenElement( grinliz::CStr<STR_LEN>& name ) = 0;
	virtual void CloseElement() = 0;
};


typedef grinliz::CStr< XStream::STR_LEN > XCStr;

class StreamWriter : public XStream {
public:
	StreamWriter( FILE* p_fp );
	~StreamWriter()					{}

	virtual void OpenElement( grinliz::CStr<STR_LEN>& name );
	virtual void CloseElement();
	//void SetAttribute( const char* name, int value );
	//void SetAttribute( const char* name, const char* value );

private:
	void WriteByte( int byte );
	void WriteInt( int value );
	void WriteString( const char* str );

	int  version;

	FILE* fp;
	int depth;
};


class StreamReader : public XStream {
public:
	StreamReader( FILE* fp );
	~StreamReader()	{}

	int Version() const { return version; }

	virtual void OpenElement( grinliz::CStr<STR_LEN>& name );
	bool		 HasChild();
	virtual void CloseElement();

private:
	int ReadByte();
	int ReadInt();
	void ReadString( grinliz::CStr<STR_LEN>* str );

	FILE* fp;
	int depth;
	int version;
};

void DumpStream( StreamReader* reader, int depth );

#endif // GRINLIZ_STREAMER_INCLUDED
