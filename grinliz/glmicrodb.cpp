#include "glmicrodb.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;


MicroDB::Entry* MicroDB::AppendEntry( const IString& key, int nSubKey ) 
{
	int pos = dataArr.Size();
	dataArr.PushArr( sizeof(Entry) );
	U8* p = dataArr.PushArr( sizeof(SubEntry)*nSubKey );
	memset( p, 0, sizeof(SubEntry)*nSubKey );

	Entry* e = (Entry*) &dataArr[pos];
	e->key = key.c_str();	// fixed memory
	e->next[0] = 0;
	e->next[1] = 0;
	e->nSub = nSubKey;

	return e;
}

MicroDB::Entry* MicroDB::Set( const IString& key, int nSubKey, int* error )
{
	// Handle the first node.
	if ( dataArr.Empty() ) {
		return AppendEntry( key, nSubKey );
	}

	int pos = 0;
	while( true ) {
		Entry* node = (Entry*) &dataArr[pos];
		if ( node->key == key.c_str() ) {	// compare by address is valid
			if ( node->nSub != nSubKey ) {
				GLASSERT( 0 );
				*error = WRONG_FORMAT;
				return 0;
			}
			return node;
		}
		int compare = strcmp( node->key, key.c_str() );
		GLASSERT( compare != 0 );	// should have been caught above
		int bias = compare < 0 ? 0 : 1;
		if ( node->next[bias] ) {
			pos = node->next[bias];
		}
		else {
			node->next[bias] = dataArr.Size();
			return AppendEntry( key, nSubKey );
		}
	}
	GLASSERT( 0 );
	return 0;
}


int MicroDB::Set( const char* key,  const char* fmt, ... )
{
	GLASSERT( fmt && *fmt );
	GLASSERT( key && *key );
	IString ikey = StringPool::Intern( key );

	// Find the key;
	int nData = strlen( fmt );
	GLASSERT( nData < 256 );
	
	int error=0;
	Entry* e = Set( StringPool::Intern( key ), nData, &error );
	if ( !e ) return error;

	const char* p = fmt;	// don't use the one in the array; it can change.

	SubEntry* sub = (SubEntry*)(e+1);

	va_list vl;
	va_start( vl, fmt );

	while ( *p ) {
		switch ( *p ) { 
		case 'S':	sub->type = 'S';	sub->str = va_arg( vl, IString ).c_str();	break;
		case 'd':	sub->type = 'd';	sub->intVal = va_arg( vl, int );			break;
		case 'f':	sub->type = 'f';	sub->floatVal = (float)va_arg( vl, double );		break;
		default:
			GLASSERT( 0 );
			return WRONG_FORMAT;
		}
		++p;
		++sub;
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
		if ( pe->key == ikey.c_str() ) {
			break;
		}
		int compare = strcmp( pe->key, key );
		GLASSERT( compare != 0 );	// should have been caught above
		int bias = compare < 0 ? 0 : 1;

		if ( !pe->next[bias] ) {
			return KEY_NOT_FOUND;
		}

		pe = (const Entry*) &dataArr[pe->next[bias]];
	}

	SubEntry* sub = (SubEntry*)(pe+1);

	va_list vl;
	va_start( vl, fmt );

	const char* p = fmt;

	while ( *p ) {
		GLASSERT( *p == sub->type );
		if ( *p != sub->type ) return WRONG_FORMAT;

		switch ( *p ) { 
		case 'S':	
			{
				IString* s = va_arg( vl, IString* );
				*s = StringPool::Intern( sub->str );
			}
			break;

		case 'd':
			{
				int* val = va_arg( vl, int* );
				*val = sub->intVal;
			}
			break;

		case 'f':
			{
				float* val = va_arg( vl, float* );
				*val = sub->floatVal;
			}
			break;

		default:
			GLASSERT( 0 );
			break;

		}
		++sub;
		++p;
	}

	va_end( vl );
	return 0;
}


void MicroDB::Serialize( XStream* xs, const char* name )
{
	// The stream contains raw pointers. It needs
	//   to be pulled apart to serialize.
	CStr<16> str;
	XarcOpen( xs, name );
	
	if ( xs->Saving() ) {
		const Entry* pe = (const Entry*) dataArr.Mem();
		while( (const U8*)pe < dataArr.Mem() + dataArr.Size()) {

			XarcOpen( xs, pe->key );
			XarcSet( xs, "nSub", pe->nSub );

			SubEntry* sub = (SubEntry*)(pe+1);
			for( int i=0; i<pe->nSub; ++i ) {

				str.Format( "%d", i );
				switch ( sub->type ) {
				case 'S':	xs->Saving()->Set( str.c_str(), sub->str );			break;
				case 'd':	xs->Saving()->Set( str.c_str(), sub->intVal );		break;
				case 'f':	xs->Saving()->Set( str.c_str(), sub->floatVal );	break;
				default: GLASSERT( 0 );
				}

				++sub;
			}
			XarcClose( xs );
			pe = (const Entry*)sub;
		}
		XarcOpen( xs, "#microdb" );
		XarcClose( xs );
	}
	else {
		dataArr.Clear();
		const char* key = xs->Loading()->OpenElement();
		while ( !StrEqual( key, "#microdb" )) {

			int nSub = 0;
			int error = 0;
			XarcGet( xs, "nSub", nSub );
			Entry* entry = Set( StringPool::Intern( key ), nSub, &error );
			GLASSERT( error == 0 );

			SubEntry* sub = (SubEntry*)(entry+1);
			for( int i=0; i<nSub; ++i, ++sub ) {
				str.Format( "%d", i );
				const StreamReader::Attribute* attribute = xs->Loading()->Get( str.c_str() );
				switch ( attribute->type ) {
				case XStream::ATTRIB_STRING:	
					{
						sub->type = 'S';
						IString istr;
						xs->Loading()->Value( attribute, &istr, 1, 0 );
						sub->str = istr.c_str();
					}
					break;
				case XStream::ATTRIB_INT:
					{ 
						sub->type = 'd';
						xs->Loading()->Value( attribute, &sub->intVal, 1, 0 );
					}
					break;

				case XStream::ATTRIB_FLOAT:
					{
						sub->type = 'f';
						xs->Loading()->Value( attribute, &sub->floatVal, 1, 0 );
					}
					break;
				default:
					GLASSERT( 0 );
					break;
				};
			}
			xs->Loading()->CloseElement();
			key = xs->Loading()->OpenElement();
		}
		xs->Loading()->CloseElement();
	}

	XarcClose( xs );
}

