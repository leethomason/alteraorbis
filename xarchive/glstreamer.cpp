#include "glstreamer.h"

using namespace grinliz;

StreamWriter::StreamWriter( FILE* p_fp ) : fp( p_fp ), depth( 0 ) 
{
	// Write the header:
	int version = 1;
	WriteInt( version );
}


void StreamWriter::WriteByte( int value ) 
{
	U8 b = (U8)value;
	fwrite( &b, 1, 1, fp );
}


void StreamWriter::WriteInt( int value )
{
	fwrite( &value, 4, 1, fp );
}


void StreamWriter::WriteString( const char* str )
{
	if ( !str ) {
		WriteByte( 0 );
	}
	else {
		int len = strlen( str );
		GLASSERT( len <= STR_LEN );
		WriteByte( len );
		fwrite( str, len, 1, fp );
	}
}


void StreamWriter::OpenElement( const char* name ) 
{
	++depth;
	WriteByte( BEGIN_ELEMENT );
	WriteString( name );
}


void StreamWriter::CloseElement()
{
	GLASSERT( depth > 0 );
	--depth;
	WriteByte( END_ELEMENT );
}


StreamReader::StreamReader( FILE* p_fp ) : fp( p_fp ), depth(0), version(0)
{
	version = ReadInt();
}


int StreamReader::ReadByte()
{
	U8 b = 0;
	fread( &b, 1, 1, fp );
	return b;
}


int StreamReader::ReadInt()
{
	int i = 0;
	fread( &i, 4, 1, fp );
	return i;
}


void StreamReader::ReadString( grinliz::CStr<STR_LEN>* str )
{
	str->Clear();
	int len = ReadByte();
	GLASSERT( len <= STR_LEN );
	char buf[STR_LEN+1];
	fread( buf, len, 1, fp );
	buf[len] = 0;
	*str = buf;
}


bool StreamReader::OpenElement( CStr<STR_LEN>* name )
{
	int node = ReadByte();
	if ( node == BEGIN_ELEMENT ) {
		ReadString( name );
		return true;
	}
	GLASSERT( node == END_ELEMENT || node == READER_EOF );
	name->Clear();
	return false;
}


void StreamReader::CloseElement()
{

}




void DumpStream( StreamReader* reader, int depth )
{
	CStr<XStream::STR_LEN> name;
	
	while ( true ) {
		bool isChild = reader->OpenElement( &name );
		if ( !isChild ) 
			break;

		for( int i=0; i<depth; ++i ) 
			printf( "    " );
		printf( "%s []\n", name.c_str() );

		DumpStream( reader, depth+1 );
	}
}

