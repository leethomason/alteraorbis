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

		int types = 0;
		for( int i=0; i<attribs.Size(); ++i ) {
			types |= attribs[i].type;
		}

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
		if ( types & ATTRIB_FLOAT ) {
			WriteByte( floatData.Size() );
			fwrite( floatData.Mem(), floatData.Size(), sizeof(float), fp );
		}
		if ( types & ATTRIB_DOUBLE ) {
			WriteByte( doubleData.Size() );
			fwrite( doubleData.Mem(), doubleData.Size(), sizeof(double), fp );
		}
		if ( types & ATTRIB_BYTE ) {
			WriteByte( byteData.Size() );
			fwrite( byteData.Mem(), byteData.Size(), 1, fp );
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
	floatData.Clear();
	doubleData.Clear();
	byteData.Clear();
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


StreamWriter::Attrib* StreamWriter::LowerSet( const char* key, int type, int n )
{
	Attrib* a = attribs.PushArr(1);
	a->type = type;
	a->keyIndex = names.Size();

	int len = strlen( key );
	char* name = names.PushArr( len+1 );
	memcpy( name, key, len+1 );

	a->size = n;
	return a;
}


void StreamWriter::Set( const char* key, int value )
{
	Attrib* a = LowerSet( key, ATTRIB_INT, 1 );

	a->index = intData.Size();
	intData.Push( value );
}


void StreamWriter::Set( const char* key, float value )
{
	Attrib* a = LowerSet( key, ATTRIB_FLOAT, 1 );

	a->index = floatData.Size();
	floatData.Push( value );
}


void StreamWriter::Set( const char* key, double value )
{
	Attrib* a = LowerSet( key, ATTRIB_DOUBLE, 1 );

	a->index = doubleData.Size();
	doubleData.Push( value );
}


void StreamWriter::Set( const char* key, U8 value )
{
	Attrib* a = LowerSet( key, ATTRIB_BYTE, 1 );

	a->index = byteData.Size();
	byteData.Push( value );
}

void StreamWriter::SetArr( const char* key, const int* value, int n )
{
	Attrib* a = LowerSet( key, ATTRIB_INT | ATTRIB_ARRAY, n );

	a->index = intData.Size();
	int* mem = intData.PushArr( n );
	memcpy( mem, value, n*sizeof(int) );
}


void StreamWriter::SetArr( const char* key, const float* value, int n )
{
	Attrib* a = LowerSet( key, ATTRIB_FLOAT | ATTRIB_ARRAY, n );

	a->index = floatData.Size();
	float* mem = floatData.PushArr( n );
	memcpy( mem, value, n*sizeof(float) );
}


void StreamWriter::SetArr( const char* key, const double* value, int n )
{
	Attrib* a = LowerSet( key, ATTRIB_DOUBLE | ATTRIB_ARRAY, n );

	a->index = doubleData.Size();
	double* mem = doubleData.PushArr( n );
	memcpy( mem, value, n*sizeof(double) );
}


void StreamWriter::SetArr( const char* key, const U8* value, int n )
{
	Attrib* a = LowerSet( key, ATTRIB_BYTE | ATTRIB_ARRAY, n );

	a->index = byteData.Size();
	U8* mem = byteData.PushArr( n );
	memcpy( mem, value, n );
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
				a->n = ReadByte();	// size
				GLASSERT( a->n == 1 );
				break;
			
			case ATTRIB_FLOAT:
				a->floatValue = floatData[ ReadByte() ];
				a->n = ReadByte();	// size
				GLASSERT( a->n == 1 );
				break;

			case ATTRIB_DOUBLE:
				a->doubleValue = doubleData[ ReadByte() ];
				a->n = ReadByte();	// size
				GLASSERT( a->n == 1 );
				break;

			case ATTRIB_BYTE:
				a->byteValue = byteData[ ReadByte() ];
				a->n = ReadByte();	// size
				GLASSERT( a->n == 1 );
				break;

			case (ATTRIB_INT | ATTRIB_ARRAY):
				a->intArr = &intData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case (ATTRIB_FLOAT | ATTRIB_ARRAY):
				a->floatArr = &floatData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case (ATTRIB_DOUBLE | ATTRIB_ARRAY):
				a->doubleArr = &doubleData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case (ATTRIB_BYTE | ATTRIB_ARRAY):
				a->byteArr = &byteData[ ReadByte() ];
				a->n = ReadByte();
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
		case XStream::ATTRIB_FLOAT:
			printf( "%s=%f ", attr->key, attr->floatValue );
			break;
		case XStream::ATTRIB_DOUBLE:
			printf( "%s=%f ", attr->key, attr->doubleValue );
			break;
		case XStream::ATTRIB_BYTE:
			printf( "%s=%d ", attr->key, attr->byteValue );
			break;

		case (XStream::ATTRIB_INT | XStream::ATTRIB_ARRAY):
			printf( "%s=(", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				printf( "%d ", *(attr->intArr+i) );
			}
			printf( ") " );
			break;
		case (XStream::ATTRIB_FLOAT | XStream::ATTRIB_ARRAY):
			printf( "%s=(", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				printf( "%f ", *(attr->floatArr+i) );
			}
			printf( ") " );
			break;
		case (XStream::ATTRIB_DOUBLE | XStream::ATTRIB_ARRAY):
			printf( "%s=(", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				printf( "%f ", *(attr->doubleArr+i) );
			}
			printf( ") " );
			break;
		case (XStream::ATTRIB_BYTE | XStream::ATTRIB_ARRAY):
			printf( "%s=(", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				printf( "%d ", *(attr->byteArr+i) );
			}
			printf( ") " );
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

