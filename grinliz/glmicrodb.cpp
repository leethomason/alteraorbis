#include "glmicrodb.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

// Sorting didn't help much. Generally very small arrays.
// Basic
// grinliz::MicroDB::Find	0.29s	0.70s	2.17%	5.30%	lumos	c:\src\alteraorbis\grinliz\glmicrodb.cpp	8	0xf0ae5c
// Use pointers
// grinliz::MicroDB::Find	0.23s	0.74s	1.62%	5.14%	lumos	c:\src\alteraorbis\grinliz\glmicrodb.cpp	11	0x3daeb1
// Split into key and value arrays?


const MicroDB::Entry* MicroDB::Find( const IString& key ) const
{
	for (const Entry* e = dataArr.Mem(); e < dataArr.End(); ++e) {
		if (e->key == key) {
			return e;
		}
	}
	return 0;	
}


MicroDB::Entry* MicroDB::FindOrCreate(const IString& key)
{
	for (int i = 0; i < dataArr.Size(); ++i) {
		if (dataArr[i].key == key) {
			return &dataArr[i];
		}
	}
	Entry* e = dataArr.PushArr(1);
	e->key = key;
	return e;
}

	
void MicroDB::Set( const char* key, int value )
{
	GLASSERT( key && *key );
	IString ikey = StringPool::Intern( key );
	Set( ikey, value );
}


void MicroDB::Set( const IString& key, int value )
{
	Entry* e = FindOrCreate( key );
	e->type = TYPE_INT;
	e->intVal = value;
}


void MicroDB::Set( const char* key, float value )
{
	GLASSERT( key && *key );
	IString ikey = StringPool::Intern( key );
	Set( ikey, value );
}


void MicroDB::Set( const IString& key, float value )
{
	Entry* e = FindOrCreate( key );
	e->type = TYPE_FLOAT;
	e->floatVal = value;
}


void MicroDB::Set( const char* key, const IString& value )
{
	GLASSERT( key && *key );
	IString ikey = StringPool::Intern( key );
	Set( ikey, value );
}


void MicroDB::Set( const IString& key, const IString& value )
{
	Entry* e = FindOrCreate( key );
	e->type = TYPE_ISTRING;
	e->value= value;
}


bool MicroDB::Has(const char* key) const
{
	GLASSERT(key && *key);
	return Has(StringPool::Intern(key));
}


bool MicroDB::Has(const IString& key) const
{
	const Entry* e = Find(key);
	return e != 0;
}


int MicroDB::Get( const char* key, int* value ) const
{
	GLASSERT( key && *key );
	return Get( StringPool::Intern( key ), value );
}


int MicroDB::Get( const IString& key, int* value ) const
{
	const Entry* e = Find( key );
	if ( !e ) return KEY_NOT_FOUND;
	if ( e->type != TYPE_INT ) return WRONG_FORMAT;
	*value = e->intVal;
	return 0;
}


int MicroDB::Get( const char* key, float* value ) const
{
	GLASSERT( key && *key );
	return Get( StringPool::Intern( key ), value );
}


int MicroDB::Get( const IString& key, float* value ) const
{
	const Entry* e = Find( key );
	if ( !e ) return KEY_NOT_FOUND;
	if ( e->type != TYPE_FLOAT ) return WRONG_FORMAT;
	*value = e->floatVal;
	return 0;
}


int MicroDB::Get( const char* key, IString* value ) const
{
	GLASSERT( key && *key );
	return Get( StringPool::Intern( key ), value );
}


int MicroDB::Get( const IString& key, IString* value ) const
{
	const Entry* e = Find( key );
	if ( !e ) return KEY_NOT_FOUND;
	if ( e->type != TYPE_ISTRING ) return WRONG_FORMAT;
	*value = e->value;
	return 0;
}


IString MicroDB::GetIString( const char* key ) const
{
	IString str;
	Get( key, &str );
	return str;
}


IString MicroDB::GetIString( const IString& key ) const
{
	IString str;
	Get( key, &str );
	return str;
}


void MicroDB::Increment( const char* key )
{
	int value = 0;
	Get( key, &value );
	Set( key, value + 1 );
}


int MicroDB::Add( const char* key, int add )
{
	int value = 0;
	Get( key, &value );
	Set( key, value + add );
	return value + add;
}


void MicroDB::Entry::Serialize( XStream* xs )
{
	XarcOpen( xs, "Entry" );

	XARC_SER( xs, key );
	XARC_SER( xs, type );
	XARC_SER(xs, value);
	if ( type == TYPE_INT ) {
		XARC_SER( xs, intVal );
	}
	else if ( type == TYPE_FLOAT ) {
		XARC_SER( xs, floatVal );
	}
	XarcClose( xs );
}


void MicroDB::Serialize( XStream* xs, const char* name )
{
	// The stream contains raw pointers. It needs
	//   to be pulled apart to serialize.
	XarcOpen( xs, name );
	XARC_SER_CARRAY( xs, dataArr );
	XarcClose( xs );
}
