#include "glmicrodb.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

int MicroDB::Set( const char* key,  const char* fmt, ... )
{
	GLASSERT( fmt && *fmt );
	GLASSERT( key && *key );
	IString ikey = StringPool::Intern( key );

	// Find the key;

	int nData = strlen( fmt );

	// Handle the first node.
	if ( dataArr.Empty() ) {
		Entry e;
		e.key = ikey;
		U8* p = dataArr.PushArr( sizeof(Entry) );
		memcpy( p, &e, sizeof(Entry) );

		// Space for the fmt
		p = dataArr.PushArr( nData+1 );
		memcpy( p, fmt, nData+1 );

		// Space for the data 
		dataArr.PushArr( nData*4 );
	}

	int pos = 0;
	U8* dataBlock = 0;

	while( true ) {
		Entry* node = (Entry*) &dataArr[pos];
		if ( node->key == ikey ) {
			if ( strcmp( fmt, (const char*) &dataArr[pos+sizeof(Entry)] )) {
				GLASSERT( 0 );
				return WRONG_FORMAT;
			}
			dataBlock = &dataArr[pos+sizeof(Entry)+nData+1];
			break;
		}
		int compare = strcmp( node->key.c_str(), key );
		GLASSERT( compare != 0 );	// should have been caught above
		int bias = compare < 0 ? 0 : 1;
		if ( node->next[bias] ) {
			pos = node->next[bias];
		}
		else {
			node->next[bias] = dataArr.Size();

			Entry e;
			e.key = ikey;
			U8* p = dataArr.PushArr( sizeof(Entry) );
			memcpy( p, &e, sizeof(Entry) );

			// Space for the fmt
			p = dataArr.PushArr( nData+1 );
			memcpy( p, fmt, nData+1 );

			// Space for the data 
			dataBlock = dataArr.PushArr( nData*4 );

			break;
		}
	}

	const char* p = fmt;	// don't use the one in the array; it can change.
	GLASSERT( dataBlock );

	va_list vl;
	va_start( vl, fmt );

	while ( *p ) {
		switch ( *p ) { 
		case 'S':
			{
				GLASSERT( sizeof( IString ) == 4 );
				IString s = va_arg( vl, IString );
				memcpy( dataBlock, &s, 4 );
			}
			break;

		case 'd':
			{
				int val = va_arg( vl, int );
				memcpy( dataBlock, &val, 4 );
			}
			break;

		case 'f':
			{
				float val = va_arg( vl, float );
				memcpy( dataBlock, &val, 4 );
			}
			break;

		default:
			GLASSERT( 0 );
			break;

		}
		dataBlock += 4;
		++p;
	}

	va_end( vl );
	return 0;
}


int MicroDB::Fetch( const char* key, const char* fmt, ... )
{

	GLASSERT( fmt && *fmt );
	GLASSERT( key && *key );
	if ( dataArr.Empty() )
		return KEY_NOT_FOUND;

	IString ikey = StringPool::Intern( key );
	const Entry* pe = (const Entry*) &dataArr[0];

	while( true ) {
		if ( pe->key == ikey ) {
			break;
		}
		int compare = strcmp( pe->key.c_str(), key );
		GLASSERT( compare != 0 );	// should have been caught above
		int bias = compare < 0 ? 0 : 1;

		if ( !pe->next[bias] ) {
			return KEY_NOT_FOUND;
		}

		pe = (const Entry*) &dataArr[pe->next[bias]];
	}

	const char* storedFMT = (const char*)(pe + 1);
	GLASSERT( strcmp( storedFMT, fmt ) == 0 );
	if ( strcmp( storedFMT, fmt )) {
		return WRONG_FORMAT;
	}

	va_list vl;
	va_start( vl, fmt );

	const char* p = fmt;
	const U8* dataBlock = (const U8*)(storedFMT + strlen(storedFMT) + 1);

	while ( *p ) {
		switch ( *p ) { 
		case 'S':
			{
				IString* s = va_arg( vl, IString* );
				memcpy( s, dataBlock, 4 );
			}
			break;

		case 'd':
			{
				int* val = va_arg( vl, int* );
				memcpy( val, dataBlock, 4 );
			}
			break;

		case 'f':
			{
				float* val = va_arg( vl, float* );
				memcpy( val, dataBlock, 4 );
			}
			break;

		default:
			GLASSERT( 0 );
			break;

		}
		dataBlock += 4;
		++p;
	}

	va_end( vl );
	return 0;
}

/*
void MicroDB::Serialize( XStream* xs, const char* name )
{
	CStr<16> str;
	XarcOpen( xs, name );
	
	if ( xs->Saving() ) {
		const Entry* pe = (const Entry*) dataArr.Mem();
		while( pe < dataArr.Mem() + dataArr.Size() ) {

			XarcOpen( xs, pe->key.c_str() );

			int count = 0;
			const char* fmt = (const char*)(pe + 1 );
			const U8* data = (const U8*)(fmt + strlen(fmt) + 1);

			while ( *fmt ) {
				str.Format( "%05d", count );
				switch( *fmt ) {
				case 'S':	xs->Saving()->Set( str.c_str(), 

			XarcClose( xs );
		}
	}	

	XarcClose( xs );
}
*/