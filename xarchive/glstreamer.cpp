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
	GLASSERT( value < 256 );
	U8 b = (U8)value;
	fwrite( &b, 1, 1, fp );
}


void StreamWriter::WriteInt( int value )
{
	fwrite( &value, 4, 1, fp );
}


void StreamWriter::WriteU16( int value )
{
	GLASSERT( value >= 0 && value < 0x10000 );
	fwrite( &value, 2, 1, fp );
}


void StreamWriter::WriteString( const char* str )
{
	if ( !str ) {
		WriteByte( 0 );
	}
	else {
		int len = strlen( str );
		GLASSERT( len < 256 );
		WriteByte( len );
		fwrite( str, len, 1, fp );
	}
}

const char* StreamWriter::CompAttrib::data = 0;

void StreamWriter::FlushAttributes()
{
	if ( attribs.Size() ) {
		CompAttrib::data = names.Mem();
		Sort< Attrib, CompAttrib >( attribs.Mem(), attribs.Size() );

		int types = ATTRIB_INT;

		// Write the header and type of stuff.
		WriteByte( BEGIN_ATTRIBUTES );
		WriteByte( types );
		GLASSERT( attribs.Size() < 256 );

		// Write the string pool.
		GLASSERT( names.Size() < 0x10000 );
		WriteU16( names.Size() );
		fwrite( names.Mem(), names.Size(), 1, fp );

		// Write the int pool
		if ( types & ATTRIB_INT ) {
			WriteByte( intData.Size() );
			fwrite( intData.Mem(), intData.Size(), sizeof(int), fp );
		}

		// The attribute array
		WriteByte( attribs.Size() );
		for( int i=0; i<attribs.Size(); ++i ) {
			const Attrib& a = attribs[i];
			WriteU16( a.keyIndex );
			WriteByte( a.type );
			WriteByte( a.index );
			WriteByte( a.size );
		}
	}
	attribs.Clear();
	intData.Clear();
	names.Clear();
}


void StreamWriter::OpenElement( const char* str ) 
{
	FlushAttributes();

	++depth;
	WriteByte( BEGIN_ELEMENT );
	WriteString( str );
}


void StreamWriter::CloseElement()
{
	FlushAttributes();

	GLASSERT( depth > 0 );
	--depth;
	WriteByte( END_ELEMENT );
}


void StreamWriter::Set( const char* key, int value )
{
	Attrib* a = attribs.PushArr(1);
	a->type = ATTRIB_INT;

	a->keyIndex = names.Size();
	int len = strlen( key );
	char* name = names.PushArr( len+1 );
	memcpy( name, key, len+1 );

	a->index = intData.Size();
	intData.Push( value );

	a->size = 1;
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


int StreamReader::ReadU16()
{
	int i=0;
	fread( &i, 2, 1, fp );
	return i;
}


int StreamReader::PeekByte()
{
	int c = getc( fp );
	ungetc( c, fp );
	return c;
}


void StreamReader::ReadString( grinliz::CDynArray<char>* target )
{
	target->Clear();

	int len = ReadByte();
	char* p = target->PushArr( len+1 );
	fread( p, len, 1, fp );
	p[len] = 0;
}


const char* StreamReader::OpenElement()
{
	int node = ReadByte();
	GLASSERT( node == BEGIN_ELEMENT );
	ReadString( &elementName );

	attributes.Clear();
	names.Clear();
	intData.Clear();

	// Attributes:
	U8 b = PeekByte();
	if ( b == BEGIN_ATTRIBUTES ) {
		ReadByte();
		int types = ReadByte();

		// String Pool
		int stringPoolSize = ReadU16();
		char* stringPool = names.PushArr( stringPoolSize );
		fread( stringPool, stringPoolSize, 1, fp );

		// Int pool
		if ( types & ATTRIB_INT ) {
			int size = ReadByte();
			int* data = intData.PushArr( size );
			fread( data, sizeof(int), size, fp );
		}

		// Attributes
		int nAttrib = ReadByte();
		for( int i=0; i<nAttrib; ++i ) {
			Attribute* a = attributes.PushArr(1);
			a->key = &stringPool[ ReadU16() ];
			a->type = ReadByte();
			switch ( a->type ) {
			case ATTRIB_INT:
				a->intValue = intData[ ReadByte() ];
				ReadByte();	// size
				break;

			default:
				GLASSERT(0);
			}
		}
	}
	return elementName.Mem();
}


bool StreamReader::HasChild()
{
	int c = PeekByte();
	return c == BEGIN_ELEMENT;
}


void StreamReader::CloseElement()
{
	int node = ReadByte();
	GLASSERT( node == END_ELEMENT );
}


const StreamReader::Attribute* StreamReader::Get( const char* key )
{
	Attribute a = { 0, key };
	int index  = attributes.BSearch( a );
	if ( index >= 0 ) {
		return &attributes[index];
	}
	return 0;
}


void DumpStream( StreamReader* reader, int depth )
{
	const char* name = reader->OpenElement();

	for( int i=0; i<depth; ++i ) 
		printf( "    " );
	printf( "%s [", name );

	const StreamReader::Attribute* attrs = reader->Attributes();
	int nAttr = reader->NumAttributes();

	for( int i=0; i<nAttr; ++i ) {
		const StreamReader::Attribute* attr = attrs + i;
		switch( attr->type ) {
		case XStream::ATTRIB_INT:
			printf( "%s=%d ", attr->key, attr->intValue );
			break;
		default:
			GLASSERT( 0 );
		}
	}
	printf( "]\n" );

	while ( reader->HasChild() ) {
		DumpStream( reader, depth+1 );
	}

	reader->CloseElement();
}

