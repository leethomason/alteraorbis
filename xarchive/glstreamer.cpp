#include "glstreamer.h"
#include "squisher.h"

using namespace grinliz;


XStream::XStream()
{
	squisher = 0;
	//squisher = new Squisher();
}


XStream::~XStream()
{
	delete squisher;
}


StreamWriter::StreamWriter( FILE* p_fp ) : fp( p_fp ), depth( 0 ) 
{
	// Write the header:
	int version = 1;
	WriteInt( version );
}


StreamWriter::~StreamWriter()
{
	if ( squisher ) {
		GLOUTPUT(( "Squisher: in:%d out:%d ratio=%.2f\n", squisher->totalIn, squisher->totalOut, squisher->Ratio() ));
	}
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
	U16 v = value;
	fwrite( &v, 2, 1, fp );
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
		int ncomp = names.Size();
		const U8* encoded = (const U8*)names.Mem();
		if ( squisher ) {
			encoded = squisher->Encode( (const U8*)names.Mem(), names.Size(), &ncomp );
		}
		WriteU16( names.Size() );	// uncompressed
		WriteU16( ncomp );			// compressed
		fwrite( encoded, ncomp, 1, fp );

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
			if ( a.type != ATTRIB_ZERO ) {
				WriteByte( a.index );
			}
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


void StreamWriter::Set( const char* key, const char* value )
{
	Attrib* a = attribs.PushArr(1);
	a->type = ATTRIB_STRING;

	a->keyIndex = names.Size();
	int len = strlen( key );
	char* name = names.PushArr( len+1 );
	memcpy( name, key, len+1 );

	a->index = names.Size() - a->keyIndex;
	len = strlen( value );
	name = names.PushArr( len+1 );
	memcpy( name, value, len+1 );

	a->size = 1;
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
	SetArr( key, &value, 1 );
}


void StreamWriter::Set( const char* key, float value )
{
	SetArr( key, &value, 1 );
}


void StreamWriter::Set( const char* key, double value )
{
	SetArr( key, &value, 1 );
}


void StreamWriter::Set( const char* key, U8 value )
{
	SetArr( key, &value, 1 );
}


void StreamWriter::SetArr( const char* key, const int* value, int n )
{
	int type = CheckZero( value, n ) ? ATTRIB_ZERO : ATTRIB_INT;
	Attrib* a = LowerSet( key, type, n );

	if ( type == ATTRIB_INT ) {
		a->index = intData.Size();
		int* mem = intData.PushArr( n );
		memcpy( mem, value, n*sizeof(int) );
	}
}


void StreamWriter::SetArr( const char* key, const float* value, int n )
{
	int type = CheckZero( value, n ) ? ATTRIB_ZERO : ATTRIB_FLOAT;
	Attrib* a = LowerSet( key, type, n );

	if ( type == ATTRIB_FLOAT ) {
		a->index = floatData.Size();
		float* mem = floatData.PushArr( n );
		memcpy( mem, value, n*sizeof(float) );
	}
}


void StreamWriter::SetArr( const char* key, const double* value, int n )
{
	int type = CheckZero( value, n ) ? ATTRIB_ZERO : ATTRIB_DOUBLE;
	Attrib* a = LowerSet( key, type, n );

	if ( type == ATTRIB_DOUBLE ) {
		a->index = doubleData.Size();
		double* mem = doubleData.PushArr( n );
		memcpy( mem, value, n*sizeof(double) );
	}
}


void StreamWriter::SetArr( const char* key, const U8* value, int n )
{
	int type = CheckZero( value, n ) ? ATTRIB_ZERO : ATTRIB_BYTE;
	Attrib* a = LowerSet( key, type, n );

	if ( type == ATTRIB_BYTE ) {
		a->index = byteData.Size();
		U8* mem = byteData.PushArr( n );
		memcpy( mem, value, n );
	}
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
	U16 i=0;
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
	floatData.Clear();
	doubleData.Clear();
	byteData.Clear();

	// Attributes:
	U8 b = PeekByte();
	if ( b == BEGIN_ATTRIBUTES ) {
		ReadByte();
		int types = ReadByte();

		// String Pool
		char* stringPool = 0;
		{
			int size = ReadU16();
			int compSize = ReadU16();
			stringPool = names.PushArr( compSize );
			fread( stringPool, compSize, 1, fp );

			if ( squisher ) {
				int decomp = compSize;
				const U8* str = squisher->Decode( (const U8*)stringPool, size, &decomp );
				GLASSERT( decomp == compSize );

				names.Clear();
				stringPool = names.PushArr( size );
				memcpy( stringPool, str, size );
			}
		}
		// pools
		if ( types & ATTRIB_INT ) {
			int size = ReadByte();
			int* data = intData.PushArr( size );
			fread( data, sizeof(int), size, fp );
		}
		if ( types & ATTRIB_FLOAT ) {
			int size = ReadByte();
			float* data = floatData.PushArr( size );
			fread( data, sizeof(float), size, fp );
		}
		if ( types & ATTRIB_DOUBLE ) {
			int size = ReadByte();
			double* data = doubleData.PushArr( size );
			fread( data, sizeof(double), size, fp );
		}
		if ( types & ATTRIB_BYTE ) {
			int size = ReadByte();
			U8* data = byteData.PushArr( size );
			fread( data, sizeof(U8), size, fp );
		}

		// Attributes
		int nAttrib = ReadByte();
		for( int i=0; i<nAttrib; ++i ) {
			Attribute* a = attributes.PushArr(1);

			int keyIndex = ReadU16();
			a->key = &stringPool[ keyIndex ];
			a->type = ReadByte();
			
			switch ( a->type ) {

			case (ATTRIB_INT):
				a->intArr = &intData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case (ATTRIB_FLOAT):
				a->floatArr = &floatData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case (ATTRIB_DOUBLE):
				a->doubleArr = &doubleData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case (ATTRIB_BYTE):
				a->byteArr = &byteData[ ReadByte() ];
				a->n = ReadByte();
				break;

			case ATTRIB_STRING:
				a->str = &stringPool[ keyIndex + ReadByte() ];
				a->n = ReadByte();
				GLASSERT( a->n == 1 );
				break;

			case ATTRIB_ZERO:
				a->intArr = 0;
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


void StreamReader::Attribute::Value( int* value ) const	
{ 
	GLASSERT( type == ATTRIB_INT || type == ATTRIB_ZERO );	
	GLASSERT( n == 1 );	
	*value = (type==ATTRIB_INT) ? intArr[0] : 0; 
}


void StreamReader::Attribute::Value( float* value ) const	
{ 
	GLASSERT( type == ATTRIB_FLOAT || type == ATTRIB_ZERO );	
	GLASSERT( n == 1 );	
	*value = (type == ATTRIB_FLOAT ) ? floatArr[0] : 0; 
}


void StreamReader::Attribute::Value( double* value ) const	
{ 
	GLASSERT( type == ATTRIB_DOUBLE || type == ATTRIB_ZERO );
	GLASSERT( n == 1 ); 
	*value = ( type == ATTRIB_DOUBLE ) ? doubleArr[0] : 0; 
}

void StreamReader::Attribute::Value( U8* value ) const		
{ 
	GLASSERT( type == ATTRIB_BYTE || type == ATTRIB_ZERO );	
	GLASSERT( n == 1 ); 
	*value = (type == ATTRIB_BYTE) ? byteArr[0] : 0; 
}

const char* StreamReader::Attribute::Str() const
{ 
	GLASSERT( type == ATTRIB_STRING ); 
	return str; 
}

void StreamReader::Attribute::Value( int* value, int size ) const	{	
	GLASSERT( type == ATTRIB_INT || type == ATTRIB_ZERO );
	GLASSERT( n == size );		
	if ( type == ATTRIB_INT )
		memcpy( value, intArr, n*sizeof(int) ); 
	else
		memset( value, 0, sizeof(*value)*size );
}


void StreamReader::Attribute::Value( float* value, int size ) const
{	
	GLASSERT( type == ATTRIB_FLOAT || type == ATTRIB_ZERO ); 
	GLASSERT( n == size );		
	if ( type == ATTRIB_FLOAT ) {
		memcpy( value, floatArr, n*sizeof(float) ); 
	}
	else {
		for( int i=0; i<size; ++i )
			*value = 0;
	}
}

void StreamReader::Attribute::Value( double* value, int size ) const
{	
	GLASSERT( type == ATTRIB_DOUBLE || type == ATTRIB_ZERO ); 
	GLASSERT( n == size );		
	if ( type == ATTRIB_DOUBLE ) {
		memcpy( value, doubleArr, n*sizeof(double) ); 
	}
	else {
		for( int i=0; i<size; ++i )
			*value = 0;
	}
}


void StreamReader::Attribute::Value( U8* value, int size ) const
{	
	GLASSERT( type == ATTRIB_BYTE || type == ATTRIB_ZERO ); 
	GLASSERT( n == size );		
	if ( type == ATTRIB_BYTE ) {
		memcpy( value, byteArr, n*sizeof(U8) ); 
	}
	else {
		for( int i=0; i<size; ++i )
			*value = 0;
	}
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
			if ( attr->n == 1 ) {
				printf( "%s=i:%d ", attr->key, attr->intArr[0] );
			}
			else {
				printf( "%s=(i:", attr->key );
				for( int i=0; i<attr->n; ++i ) {
					printf( "%d ", *(attr->intArr+i) );
				}
				printf( ") " );
			}
			break;
		
		case XStream::ATTRIB_FLOAT:
			if ( attr->n == 1 ) {
				printf( "%s=f:%f ", attr->key, attr->floatArr[0] );
			}
			else {
				printf( "%s=(f:", attr->key );
				for( int i=0; i<attr->n; ++i ) {
					printf( "%f ", *(attr->floatArr+i) );
				}
				printf( ") " );
			}
			break;
		
		case XStream::ATTRIB_DOUBLE:
			if ( attr->n == 1 ) {
				printf( "%s=d:%f ", attr->key, attr->doubleArr[0] );
			}
			else {
				printf( "%s=(d:", attr->key );
				for( int i=0; i<attr->n; ++i ) {
					printf( "%f ", *(attr->doubleArr+i) );
				}
				printf( ") " );
			}
			break;

		case XStream::ATTRIB_BYTE:
			if ( attr->n == 1 ) {
				printf( "%s=b:%d ", attr->key, attr->byteArr[0] );
			}
			else {
				printf( "%s=(b:", attr->key );
				for( int i=0; i<attr->n; ++i ) {
					printf( "%d ", *(attr->byteArr+i) );
				}
				printf( ") " );
			}
			break;

		case XStream::ATTRIB_ZERO:
			printf( "%s=z[%d] ", attr->key, attr->n );
			break;

		case XStream::ATTRIB_STRING:
			printf( "%s=%s ", attr->key, attr->str );
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


bool XarcGet( XStream* xs, const char* key, grinliz::IString& i )
{
	GLASSERT( xs->Loading() );
	const StreamReader::Attribute* attr = xs->Loading()->Get( key );
	if ( attr ) {
		i = StringPool::Intern( attr->Str() );
		return true;
	}
	else {
		i = IString();
		return false;
	}
}

void XarcSet( XStream* xs, const char* key, const grinliz::IString& i )
{
	GLASSERT( xs->Saving() );
	xs->Saving()->Set( key, i.empty() ? "" : i.c_str() );
}
