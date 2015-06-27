#include "glstreamer.h"

#define ENCODE_FLOATS

using namespace grinliz;


template < class T > bool IsZeroArray( T* mem, int n ) {
	for( int i=0; i<n; ++i ) {
		if ( mem[i] ) {
			return false;
		}
	}
	return true;
}

XStream::XStream()
{
}


XStream::~XStream()
{
}


StreamWriter::StreamWriter( FILE* p_fp, int version ) 
	:	XStream(), 
		file( p_fp ), 
		depth( 0 ),
		nCompInt( 0 ),
		nInt( 0 ),
		nStr( 0 ),
		nNumber( 0 )
{
	// Write the header:
	idPool = 0;
	WriteInt( version );
	Flush();
}


StreamWriter::~StreamWriter()
{
	//Code may have already closed stream. Flush is in CloseElement()
	//Flush();
	GLOUTPUT(( "StreamWriter Bytes. nCompInt=%d (nInt=%d) nStr=%d nNumber=%d\n",
		       nCompInt, nInt, nStr, nNumber ));
	GLOUTPUT(( "             KB.    nCompInt=%d (nInt=%d) nStr=%d nNumber=%d\n",
		       nCompInt/1024, nInt/1024, nStr/1024, nNumber/1024 ));
}

void StreamWriter::WriteInt( int value )
{
	// Very simple compression. Lead 4 bits
	// saves the sign & #bytes needed.
	//
	// 1: sign (1:positive, 0:negative)
	// 3: 0-4 number of bytes that follow
	U8 lead=0;
	if ( value >= 0 ) {
		lead |= (1<<7);
	}
	else {
		value *= -1;
	}

	int nBytes = NumBytesFollow(value);
	lead |= nBytes << 4;

	lead |= value & 0xf;
	//fputc( lead, fp );
	buffer.Push(lead);
	value >>= 4;

	for( int i=0; i<nBytes; ++i ) {
		//fputc( value & 0xff, fp );
		buffer.Push(value & 0xff);
		value >>= 8;
	}

	nCompInt += nBytes + 1;
	nInt += 4;
}


int StreamReader::ReadInt()
{
	int value = 0;
	int lead = getc( fp );
	int sign = (lead & (1<<7)) ? 1 : -1;
	int nBytes = (lead & 0x70)>>4;

	value = lead & 0xf;
	for( int i=0; i<nBytes; ++i ) {
		value |= (getc(fp) << (4+i*8));
	}
	return value * sign;
}


int StreamReader::PeekByte()
{
	int value = 0;
	int lead = getc( fp );
	int sign = (lead & (1<<7)) ? 1 : -1;
	int nBytes = (lead & 0x70)>>4;
	GLASSERT( nBytes == 0 );
	(void)nBytes;

	value = lead & 0xf;
	ungetc( lead, fp );
	return value * sign;
}


void StreamWriter::WriteString( const char* str )
{
	// Write the name.
	// Has it been written before? Re-use, positive index.
	// Else, negative index, then add string.

	// Special case empty string.
	if ( str == 0 || *str == 0 ) {
		WriteInt( 0 );
		return;
	}

	int index=0;
	if ( strToIndex.Query( str, &index )) {
		WriteInt( index );
	}
	else {
		idPool++;
		WriteInt( -idPool );
		int len = strlen( str );
		WriteInt( len );
		//fwrite( str, len, 1, fp );
		WriteBuffer(str, len);

		nStr += len + 1;

		IString istr = StringPool::Intern( str );
		indexToStr.Add( idPool, istr.c_str() );
		strToIndex.Add( istr.c_str(), idPool );
	}
}


const char* StreamReader::ReadString()
{
	int id = ReadInt();

	if ( id == 0 ) {
		return "";
	}

	const char* r = 0;

	if ( id < 0 ) {
		int len = ReadInt();
		strBuf.Clear();
		char* p = strBuf.PushArr( len );
		size_t didRead = fread( p, len, 1, fp );
		GLASSERT(didRead == len);
		(void)didRead;
		strBuf.Push(0);

		IString istr = StringPool::Intern( strBuf.Mem() );
		indexToStr.Add( -id, istr.c_str() );
		r = istr.c_str();
	}
	else {
		r = indexToStr.Get( id );
	}
	return r;
}


void StreamWriter::WriteReal(double value, bool isDouble)
{
	// Went down the road of pulling apart floating point numbers,
	// and re-encoding as float8, float16, etc.
	// Fun to do. But tricky code. Going with the hacky approach for now.
	if (value == 0) {
		nNumber += 1;
		//fputc(ENC_0, fp);
		buffer.Push(ENC_0);
		return;
	}
	if (value == 1) {
		nNumber += 1;
		//fputc(ENC_1, fp);
		buffer.Push(ENC_1);
		return;
	}

	int realBytes = (isDouble && (value != float(value))) ? 8 : 4;

	if (value == int(value)) {
		int nBytes = NumBytesFollow(abs(int(value))) + 1;
		if (nBytes < realBytes) {
			nNumber += 1;
			//fputc(ENC_INT, fp);
			buffer.Push(ENC_INT);
			WriteInt(int(value));
			return;
		}
	}
	if (value*2.0f == int(value * 2)) {
		// The 0.5 case is very common (in altera.) Do
		// a special check for that.
		int nBytes = NumBytesFollow(abs(int(value * 2))) + 1;
		if (nBytes < realBytes) {
			nNumber += 1;
			//fputc(ENC_INT2, fp);
			buffer.Push(ENC_INT2);
			WriteInt(int(value * 2));
			return;
		}
	}
	if ( realBytes == 4 ) {
		//fputc(ENC_FLOAT, fp);
		buffer.Push(ENC_FLOAT);
		nNumber += 1 + sizeof(float);
		float f = float(value);
		//fwrite(&f, sizeof(float), 1, fp);
		WriteBuffer(&f, sizeof(float));
		return;
	}

	//fputc(ENC_DOUBLE, fp);
	buffer.Push(ENC_DOUBLE);
	nNumber += 1 + sizeof(double);
	//fwrite(&value, sizeof(double), 1, fp);
	WriteBuffer(&value, sizeof(double));
}

void StreamWriter::WriteFloat( float value )
{
#ifdef ENCODE_FLOATS
	WriteReal(value, false);
#else
	nNumber += sizeof(value);
	fwrite( &value, sizeof(value), 1, fp );
#endif
}


double StreamReader::ReadReal()
{
	int enc = fgetc(fp);
	if (enc == ENC_0) {
		return 0;
	}
	else if (enc == ENC_1) {
		return 1;
	}
	else if (enc == ENC_INT) {
		int v = ReadInt();
		return float(v);
	}
	else if (enc == ENC_INT2) {
		int v = ReadInt();
		return float(v)*0.5f;
	}
	else if (enc == ENC_FLOAT) {
		float v = 0;
		size_t didRead = fread(&v, sizeof(v), 1, fp);
		GLASSERT(didRead == sizeof(v));
		(void)didRead;
		return v;
	}
	// ENC_DOUBLE
	double v = 0;
	size_t didRead = fread(&v, sizeof(v), 1, fp);
	GLASSERT(didRead == sizeof(v));
	(void)didRead;
	return v;
}


float StreamReader::ReadFloat()
{
#ifdef ENCODE_FLOATS
	return (float)ReadReal();
#else
	float v = 0;
	fread( &v, sizeof(v), 1, fp );
	return v;
#endif
}


void StreamWriter::WriteDouble( double value )
{
#ifdef ENCODE_FLOATS
	WriteReal(value, true);
#else
	nNumber += sizeof(value);
	fwrite( &value, sizeof(value), 1, fp );
#endif
}


double StreamReader::ReadDouble()
{
#ifdef ENCODE_FLOATS
	return ReadReal();
#else
	double v=0;
	fread( &v, sizeof(v), 1, fp );
	return v;
#endif
}


void StreamWriter::Flush()
{
	if (buffer.Size()) {
		fwrite(buffer.Mem(), buffer.Size(), 1, file);
		buffer.Clear();
	}
}


void StreamWriter::OpenElement( const char* str ) 
{
	++depth;
	WriteInt( BEGIN_ELEMENT );
	WriteString( str );
}


void StreamWriter::CloseElement()
{
	GLASSERT( depth > 0 );
	--depth;
	WriteInt( END_ELEMENT );

	if (depth == 0 || buffer.Size() > 32*1000) {
		Flush();
	}
}


void StreamWriter::SetArr( const char* key, const char* value[], int n )
{
	WriteInt( ATTRIB_STRING );
	WriteString( key );

	bool zero = IsZeroArray( value, n );

	if ( zero ) {
		WriteInt( -n );
	}
	else {
		WriteInt( n );
		for( int i=0; i<n; ++i ) {
			WriteString( value[i] );
		}
	}
}


void StreamWriter::SetArr( const char* key, const grinliz::IString* value, int n )
{
	WriteInt( ATTRIB_STRING );
	WriteString( key );

	bool zero = true;
	for( int i=0; i<n; ++i ) {
		if ( !value[i].empty() ) {
			zero = false;
			break;
		}
	}

	if ( zero ) {
		WriteInt( -n );
	}
	else {
		WriteInt( n );
		for( int i=0; i<n; ++i ) {
			WriteString( value[i].c_str() );
		}
	}
}


void StreamWriter::SetArr( const char* key, const U8* value, int n )
{
	WriteInt( ATTRIB_INT );
	WriteString( key );

	bool zero = IsZeroArray( value, n );

	if ( zero ) {
		WriteInt( -n );
	}
	else {
		WriteInt( n );
		for( int i=0; i<n; ++i ) {
			WriteInt( value[i] );
		}
	}
}


void StreamWriter::SetArr( const char* key, const int* value, int n )
{
	WriteInt( ATTRIB_INT );
	WriteString( key );

	bool zero = IsZeroArray( value, n );

	if ( zero ) {
		WriteInt( -n );
	}
	else {
		WriteInt( n );
		for( int i=0; i<n; ++i ) {
			WriteInt( value[i] );
		}
	}
}


void StreamWriter::SetArr( const char* key, const float* value, int n )
{
	WriteInt( ATTRIB_FLOAT );
	WriteString( key );

	bool zero = IsZeroArray( value, n );

	if ( zero ) {
		WriteInt( -n );
	}
	else {
		WriteInt( n );
		for( int i=0; i<n; ++i ) {
			WriteFloat( value[i] );
		}
	}
}


void StreamWriter::SetArr( const char* key, const double* value, int n )
{
	WriteInt( ATTRIB_DOUBLE );
	WriteString( key );

	bool zero = IsZeroArray( value, n );

	if ( zero ) {
		WriteInt( -n );
	}
	else {
		WriteInt( n );
		for( int i=0; i<n; ++i ) {
			WriteDouble( value[i] );
		}
	}
}

StreamReader::StreamReader(FILE* p_fp) : XStream(), fp(p_fp), depth(0), version(0), peekElementName(0)
{
	version = ReadInt();
}

const char* StreamReader::PeekElement()
{
	GLASSERT(peekElementName == 0);
	peekElementName = OpenElement();
	return peekElementName;
}


const char* StreamReader::OpenElement()
{
	if (peekElementName) {
		const char* r = peekElementName;
		peekElementName = 0;
		return r;
	}

	int node = ReadInt();
	GLASSERT( node == BEGIN_ELEMENT );
	(void)node;
	const char* elementName = ReadString();

	attributes.Clear();
	intData.Clear();
	floatData.Clear();
	doubleData.Clear();

	// Attributes:
	while ( true ) {
		int b = PeekByte();
		if ( b >= ATTRIB_START && b < ATTRIB_END ) {
			Attribute a;
			a.type = ReadInt();
			a.key  = ReadString();
			a.n    = ReadInt();

			bool zero = false;
			if ( a.n < 0 ) { 
				a.n = -a.n;
				zero = true;
			}

			switch ( a.type ) {
			case ATTRIB_INT:
				a.offset = intData.Size();
				for( int i=0; i<a.n; ++i ) {
					intData.Push( zero ? 0 : ReadInt() );
				}
				break;

			case ATTRIB_FLOAT:
				a.offset = floatData.Size();
				for( int i=0; i<a.n; ++i ) {
					floatData.Push( zero ? 0 : ReadFloat() );
				}
				break;

			case ATTRIB_DOUBLE:
				a.offset = doubleData.Size();
				for( int i=0; i<a.n; ++i ) {
					doubleData.Push( zero ? 0 : ReadDouble() );
				}
				break;

			case ATTRIB_STRING:
				a.offset = stringData.Size();
				for( int i=0; i<a.n; ++i ) {
					stringData.Push( zero ? "" : ReadString() );
				}
				break;
		
			default:
				GLASSERT( 0 );
				break;
			}
			attributes.Push( a );
		}
		else {
			break;
		}
	}
	attributes.Sort();
	return elementName;
}


bool StreamReader::HasChild()
{
	int c = PeekByte();
	return c == BEGIN_ELEMENT;
}


void StreamReader::CloseElement()
{
	int node = ReadInt();
	GLASSERT( node == END_ELEMENT );
	(void)node;
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


void StreamReader::Value( const Attribute* a, int* value, int size, int offset ) const	
{	
	GLASSERT( a->type == ATTRIB_INT );
	for( int i=0; i<size; ++i ) {
		value[i] = intData[a->offset+i+offset];
	}
}


void StreamReader::Value( const Attribute* a, float* value, int size, int offset ) const
{	
	GLASSERT( a->type == ATTRIB_FLOAT );
	for( int i=0; i<size; ++i ) {
		value[i] = floatData[a->offset+i+offset];
	}
}


void StreamReader::Value( const Attribute* a, double* value, int size, int offset ) const
{	
	GLASSERT( a->type == ATTRIB_DOUBLE );
	for( int i=0; i<size; ++i ) {
		value[i] = doubleData[a->offset+i+offset];
	}
}


void StreamReader::Value( const Attribute* a, U8* value, int size, int offset ) const	
{	
	GLASSERT( a->type == ATTRIB_INT );
	for( int i=0; i<size; ++i ) {
		int v = intData[a->offset+i+offset];
		GLASSERT( v >= 0 && v < 256 );
		value[i] = (U8)v;
	}
}


void StreamReader::Value( const Attribute* a, IString* value, int size, int offset ) const	
{	
	GLASSERT( a->type == ATTRIB_STRING );
	for( int i=0; i<size; ++i ) {
		const char* s = stringData[a->offset+i+offset];
		value[i] = StringPool::Intern( s );
	}
}

const char* StreamReader::Value( const Attribute* a, int index ) const
{
	GLASSERT( a->type == ATTRIB_STRING );
	GLASSERT( index < a->n );
	const char* s = stringData[a->offset+index];
	return s;
}


void DumpStream( StreamReader* reader, int depth )
{
	const char* name = reader->OpenElement();

	for( int i=0; i<depth; ++i ) 
		printf( "    " );
	printf( "%s [", name );

	const StreamReader::Attribute* attrs = reader->Attributes();
	int nAttr = reader->NumAttributes();

	float fv;
	double dv;
	int iv;

	for( int i=0; i<nAttr; ++i ) {
		const StreamReader::Attribute* attr = attrs + i;
		switch( attr->type ) {
		case XStream::ATTRIB_INT:
			printf( "%s=(i:", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				reader->Value( attr, &iv, 1, i );
				printf( "%d ", iv );
			}
			printf( ") " );
			break;
		
		case XStream::ATTRIB_FLOAT:
			printf( "%s=(f:", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				reader->Value( attr, &fv, 1, i );
				printf( "%f ", fv );
			}
			printf( ") " );
			break;
		
		case XStream::ATTRIB_DOUBLE:
			printf( "%s=(d:", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				reader->Value( attr, &dv, 1, i );
				printf( "%f ", dv );
			}
			printf( ") " );
			break;

		case XStream::ATTRIB_STRING:
			printf( "%s=(s:", attr->key );
			for( int i=0; i<attr->n; ++i ) {
				printf( "%s ", reader->Value( attr, i ));
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


bool XarcGet( XStream* xs, const char* key, grinliz::IString& i )
{
	GLASSERT( xs->Loading() );
	const StreamReader::Attribute* attr = xs->Loading()->Get( key );
	if ( attr ) {
		i = StringPool::Intern( xs->Loading()->Value( attr, 0 ));
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


bool XarcGet( XStream* xs, const char* key, grinliz::Matrix4 &v )			
{ 
	GLASSERT( xs->Loading() );
	const StreamReader::Attribute* attr = xs->Loading()->Get( key );
	if ( attr->n == 1 ) {
		v.SetIdentity();
		return true;
	}
	return XarcGetArr( xs, key, v.Mem(), 16 );
}


void XarcSet( XStream* xs, const char* key, const grinliz::Matrix4& v )		
{ 
	float identity[1] = { 1 };
	if ( v.IsIdentity() ) {
		XarcSetArr( xs, key, identity, 1 );
	}
	else {
		XarcSetArr( xs, key, v.Mem(), 16 );
	}
}
