/*
 Copyright (c) 2010 Lee Thomason

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include <stdlib.h>
#include <string.h>

#include "gamedbreader.h"
#include "../grinliz/glstringutil.h"	// FIXME: only used for child index snprintf. Can and should be removed.
#include "../FastLZ/fastlz.h"

#ifdef _MSC_VER
#pragma warning ( disable : 4996 )
#endif
using namespace gamedb;

struct CompStringID
{
	const char* key;
	const void* mem;
	CompStringID( const char* k, const void* r ) : key( k ), mem( r )	{}

	int operator()( U32 offset ) const {
		const char* str = (const char*)mem + offset;
		GLASSERT( str );
		return strcmp( str, key );
	}
};


struct CompChild
{
	int key;
	const void* mem;
	CompChild( int k, const void* r ) : key( k ), mem( r )	{}

	int operator()( U32 a ) const {
		const ItemStruct* istruct = (const ItemStruct*)((const char*)mem + a);
		return istruct->nameID - key;
	}
};


struct CompAttribute
{
	int key;
	CompAttribute( int k ) : key( k )	{}

	int operator()( const AttribStruct& attrib ) const {
		return int(attrib.Key()) - key;
	}
};


template< typename T, typename C > 
class BinarySearch
{
public:
	int Search( const T* arr, int N, const C& compareFunc ) 
	{
		unsigned low = 0;
		unsigned high = N;

		while (low < high) {
			unsigned mid = low + (high - low) / 2;

			int compare = compareFunc( arr[mid] );
			if ( compare < 0 )
				low = mid + 1; 
			else
				high = mid; 
		}
		if ( ( low < (unsigned)N ) && ( compareFunc( arr[low] ) == 0 ) )
			return low;
		return -1;
	}
};



/* static */ Reader* Reader::readerRoot = 0;

/* static */ const Reader* Reader::GetContext( const Item* item )
{
	GLASSERT( item );
	const void* m = item;

	for( Reader* r=readerRoot; r; r=r->next ) {
		if ( m >= r->mem && m < r->endMem ) {
			return r;
		}
	}
	GLASSERT( 0 );
	return 0;
}

Reader::Reader()
{
	mem = 0;
	memSize = 0;
	fp = 0;

	// Add to linked list.
	next = readerRoot;
	readerRoot = this;
	buffer = 0;
	bufferSize = 0;
	access = 0;
	accessSize = 0;
}


Reader::~Reader()
{
	// unlink from the global list.
	Reader* r = readerRoot;
	Reader* prev = 0;
	while( r && r != this ) {
		prev = r;
		r = r->next;
	}
	GLASSERT( r );

	if ( prev ) {
		prev->next = r->next;
	}
	else {
		readerRoot = r->next;
	}

	if ( mem )
		Free( mem );
	if ( buffer )
		Free( buffer );
	if ( access )
		Free( access );
	if ( fp ) {
		fclose( fp );
	}
}


bool Reader::Init( int id, const char* filename, int _offset )
{
	databaseID = id;
	fp = fopen( filename, "rb" );
	if ( !fp ) {
		GLOUTPUT_REL(("Reader::Init failed on '%s'\n", filename ? filename : ""));
		GLASSERT( 0 );
		return false;
	}

	offset = _offset;	
	fseek( fp, 0, SEEK_END );
	int dbSize = (int)ftell( fp ) - (int)offset;
	fseek( fp, offset, SEEK_SET );

	// Read in the data structers. Leave the "data" compressed and in the file.
	HeaderStruct header;
	int size = fread( &header, sizeof(header), 1, fp );
	if ( size != 1 ) {
		GLOUTPUT(( "CORRUPT DATABASE\n" ));
		fclose( fp );
		return false;
	}

	fseek( fp, offset, SEEK_SET );
	memSize = header.offsetToData;

	// FIXME: Should add a checksum...
	if (    (int)header.offsetToItems > memSize 
		 || (int)header.offsetToDataDesc > memSize
		 || memSize > dbSize ) 
	{
		GLOUTPUT(( "CORRUPT DATABASE\n" ));
		fclose( fp );
		return false;
	}

	mem = Malloc( memSize );
	endMem = (const char*)mem + memSize;

	GLOUTPUT(( "Reading '%s' from offset=%d\n", filename, offset ));
	size_t  didRead = fread( mem, memSize, 1, fp );
	GLASSERT(didRead == 1);
	(void)didRead;

	root = (const Item*)( (U8*)mem + header.offsetToItems );

#if 0		// Dump string pool

	{
		int totalLen = 0;
		for( unsigned i=0; i<header.nString; ++i ) {
			const char* str = GetString( i );

			int len = strlen( str );
			if ( totalLen != 0 && totalLen + len > 60 ) {
				printf( "\n" );
				totalLen = 0;
			}
			totalLen += len;
			printf( "%s ", str );
		}
		printf( "\n" );
	}
#endif

	return true;
}


void Reader::RecWalk( const Item* item, int depth )
{
	for( int i=0; i<depth; ++i )
		printf( "  " );
	printf( "%s", item->Name() );
	
	printf( " [" );
	int n = item->NumAttributes();
	for( int i=0; i<n; ++i ) {
		const char* name = item->AttributeName( i );
		printf( " %s=", name );
		switch( item->AttributeType( i ) ) {
			case ATTRIBUTE_DATA:
				printf( "data(#%d %d)", item->GetDataID( name ), item->GetDataSize( name ) );
				this->AccessData(item, name);
				break;
			case ATTRIBUTE_INT_ARRAY:
				{
					int n = item->GetArrayLen( name );
					if ( n <= 8 ) {
						int array[8];
						item->GetIntArray( name, n, array );
						printf( "(" );
						for( int i=0; i<n; ++i ) {
							printf( "%d ", array[i] );
						}
						printf( ")" );
					}
					else {
						printf( "intArray(#%d %d)", item->GetDataID( name ), item->GetArrayLen( name ) );
					}
				}
				break;
			case ATTRIBUTE_FLOAT_ARRAY:
				{
					int n = item->GetArrayLen( name );
					if ( n <= 8 ) {
						float array[8];
						item->GetFloatArray( name, n, array );
						printf( "(" );
						for( int i=0; i<n; ++i ) {
							printf( "%f ", array[i] );
						}
						printf( ")" );
					}
					else {
						printf( "floatArray(#%d %d)", item->GetDataID( name ), item->GetArrayLen( name ) );
					}
				}
				break;
			case ATTRIBUTE_INT:
				printf( "%d", item->GetInt( name ) );
				break;
			case ATTRIBUTE_FLOAT:
				printf( "%f", item->GetFloat( name ) );
				break;
			case ATTRIBUTE_STRING:
				printf( "'%s'", item->GetString( name ) );
				break;
			case ATTRIBUTE_BOOL:
				printf( "%s", item->GetBool( name ) ? "TRUE" : "FALSE" );
				break;
			default:
				GLASSERT( 0 );
		}
	}
	printf( "]\n" );

	int nChildren = item->NumChildren();
	for( int i=0; i<nChildren; ++i ) {
		const Item* child = item->ChildAt( i );
		RecWalk( child, depth + 1 );
	}
}


const char* Reader::GetString( int id ) const
{
#ifdef DEBUG
	const HeaderStruct* header = (const HeaderStruct*)mem;
	GLASSERT( id >= 0 && id < (int)header->nString );
#endif

	const U32* stringOffset = (const U32*)((const char*)mem + sizeof(HeaderStruct));
	const char* str = (const char*)mem + stringOffset[id];

	GLASSERT( str > mem && str < endMem );
	return str;
}


int Reader::GetStringID( const char* str ) const
{
	// Trickier and more common case. Do a binary seach through the string table.
	const HeaderStruct* header = (const HeaderStruct*)mem;

	CompStringID comp( str, mem );
	const U32* stringOffset = (const U32*)((const char*)mem + sizeof(HeaderStruct));
	BinarySearch<U32, CompStringID> bsearch;
	return bsearch.Search( stringOffset, (int)header->nString, comp );
}


int Reader::GetDataSize( int dataID ) const
{
	const HeaderStruct* header = (const HeaderStruct*)mem;
	GLASSERT( header->offsetToDataDesc % 4 == 0 );

	const DataDescStruct* dataDesc = (const DataDescStruct*)((const U8*)mem + header->offsetToDataDesc);
	GLASSERT( dataID >= 0 && dataID < (int)header->nData );

	return dataDesc[dataID].size;
}


void Reader::GetData( int dataID, void* target, int memSize ) const
{
	const HeaderStruct* header = (const HeaderStruct*)mem;
	GLASSERT( header->offsetToDataDesc % 4 == 0 );

	const DataDescStruct* dataDescPtr = (const DataDescStruct*)((const U8*)mem + header->offsetToDataDesc);
	GLASSERT( dataID >= 0 && dataID < (int)header->nData );
	const DataDescStruct& dataDesc = dataDescPtr[dataID];
	
	fseek( fp, offset+dataDesc.offset, SEEK_SET );

	if ( dataDesc.compressedSize == dataDesc.size ) {
		// no compression.
		GLASSERT( dataDesc.size == (U32)memSize );
		size_t didRead = fread( target, memSize, 1, fp );
		GLASSERT(didRead == 1);
		(void)didRead;
	}
	else {
		if ( bufferSize < (int)dataDesc.compressedSize ) {
			bufferSize = (int)dataDesc.compressedSize*5/4;
			if ( bufferSize < 1000 )
				bufferSize = 1000;
			if ( buffer )
				buffer = Realloc( buffer, bufferSize );
			else 
				buffer = Malloc( bufferSize );
		}
		size_t didRead = fread( buffer, dataDesc.compressedSize, 1, fp );
		GLASSERT(didRead == 1);
		(void)didRead;

		int resultSize = fastlz_decompress(buffer, dataDesc.compressedSize, target, dataDesc.size);

		if (resultSize != int(dataDesc.size)) {
			GLOUTPUT_REL(("Reader::GetData uncompress returned size %d. dataID=%d size=%d compressedSize=%d memSize=%d\n", resultSize, dataID, dataDesc.size, dataDesc.compressedSize, memSize));
		}
		GLASSERT( dataDesc.size == (U32)memSize );
	}
}


const void* Reader::AccessData( const Item* item, const char* name, int* p_size ) const
{
	GLASSERT( ItemInReader( item ));

	int size = 0;
	if ( p_size ) *p_size = 0;

	int type = item->AttributeType( name ); 
	if ( !IsDataType( type ) ) {
		return 0;
	}
	size = item->GetDataSize( name );
	// Make sure we have enough space, including appended null terminator.
	if ( (size+1) > accessSize ) {
		accessSize = (size+1);
		if ( access )
			access = Realloc( access, accessSize );
		else
			access = Malloc( accessSize );
	}
	item->GetData( name, access, size );
	*((char*)access + size) = 0;	// null terminate for text assets.

	if ( p_size )
		*p_size = size;
	return access;
}


const char* Item::Name() const
{
	const Reader* context = Reader::GetContext( this );
	const ItemStruct* istruct = (const ItemStruct*)this;

	return context->GetString( istruct->nameID );	
}


const Item* Item::Parent() const
{
	const Reader* context = Reader::GetContext( this );
	const U8* mem = (const U8*)context->BaseMem();
	const ItemStruct* istruct = (const ItemStruct*)this;
	if ( istruct->offsetToParent )
		return (const Item*)(mem + istruct->offsetToParent);
	return 0;
}


const Item* Item::ChildAt( int i ) const
{
	const Reader* context = Reader::GetContext( this );
	const U8* mem = (const U8*)context->BaseMem();

#ifdef DEBUG
	const ItemStruct* istruct = (const ItemStruct*)this;
	GLASSERT( i >= 0 && i < (int)istruct->nChildren );
#endif

	const U32* childOffset = (const U32*)((U8*)this + sizeof( ItemStruct ));
	return (const Item*)( mem + childOffset[i]);
}


const Item* Item::Child( int i ) const
{
	char buffer[32];
	GLASSERT(i >= 0 && i < 10000);
	grinliz::SNPrintf( buffer, 32, "%04d", i );
	return Child( buffer );
}


const Item* Item::Child( const char* name ) const
{
	const ItemStruct* istruct = (const ItemStruct*)this;
	const U32* childOffset = (const U32*)((U8*)this + sizeof( ItemStruct ));
	const Reader* context = Reader::GetContext( this );
	const U8* mem = (const U8*)context->BaseMem();
	int nameID = context->GetStringID( name );

	if ( nameID >= 0 ) {
		CompChild comp( nameID, mem );
		BinarySearch<U32, CompChild> search;
		int result = search.Search( childOffset, istruct->nChildren, comp );
		if ( result >= 0 )
			return ChildAt( result );
	}
	return 0;
}


int Item::NumAttributes() const
{
	const ItemStruct* istruct = (const ItemStruct*)this;
	return (int)istruct->nAttrib;
}


const AttribStruct* Item::AttributePtr( int i ) const
{
	GLASSERT( i>=0 && i<NumAttributes() );
	const ItemStruct* istruct = (const ItemStruct*)this;
	const AttribStruct* attrib = (const AttribStruct*)((U8*)this + sizeof( ItemStruct ) + istruct->nChildren*sizeof(U32) );

	return attrib + i;
}


const char* Item::AttributeName( int i ) const
{
	const AttribStruct* attrib = AttributePtr( i );
	// note only uses lower 24 bits
	U32 key = attrib->Key();

	const Reader* context = Reader::GetContext( this );
	return context->GetString( key );
}


int Item::AttributeType( int i ) const
{
	const AttribStruct* attrib = AttributePtr( i );
	U32 type = attrib->Type();
	return (int)type;
}


int Item::AttributeType( const char* name ) const
{
	int i = AttributeIndex( name );
	if ( i >= 0 ) {
		return AttributeType( i );
	}
	return ATTRIBUTE_INVALID;
}



int Item::AttributeIndex( const char* name ) const
{

	//const ItemStruct* istruct = (const ItemStruct*)this;

	const Reader* context = Reader::GetContext( this );
	int id = context->GetStringID( name );
	int n = NumAttributes();
	int result = -1;

	if ( id >= 0 && n ) {
		const AttribStruct* attrib = AttributePtr( 0 );
		CompAttribute comp( id );
		BinarySearch<AttribStruct, CompAttribute> search;
		result = search.Search( attrib, n, comp );
	}
	return result;
}


int	Item::GetDataSize( int i ) const
{
	int type = AttributeType( i ); 
	GLASSERT( IsDataType( type ) );
	if ( IsDataType( type ) ) {	// also range checks
		const AttribStruct* attrib = AttributePtr( i );
		const Reader* context = Reader::GetContext( this );
		return context->GetDataSize( attrib->dataID );
	}
	return -1;
}


int	Item::GetDataSize( const char* name ) const
{
	int i = AttributeIndex( name );
	if ( i >= 0 ) {
		return GetDataSize( i );
	}
	return -1;
}


void Item::GetDataInfo( int i, int* p_offset, int* size, bool* compressed ) const
{
	GLASSERT( IsDataType( AttributeType( i )));
	const AttribStruct* attrib = AttributePtr( i );
	const Reader* context = Reader::GetContext( this );

	const HeaderStruct* header = (const HeaderStruct*)context->BaseMem();
	GLASSERT( header->offsetToDataDesc % 4 == 0 );

	const DataDescStruct* dataDescPtr = (const DataDescStruct*)((const U8*)context->BaseMem() + header->offsetToDataDesc);
	GLASSERT( attrib->dataID >= 0 && attrib->dataID < header->nData );
	const DataDescStruct& dataDesc = dataDescPtr[attrib->dataID];

	*p_offset = context->OffsetFromStart()+ dataDesc.offset;
	*size = dataDesc.size;
	*compressed = ( dataDesc.size != dataDesc.compressedSize );
}


void Item::GetDataInfo( const char* name, int* offset, int* size, bool* compressed ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0 );
	GetDataInfo( i, offset, size, compressed );
}


void Item::GetData( int i, void* mem, int memSize ) const
{
	GLASSERT( IsDataType( AttributeType( i )));
	GLASSERT( GetDataSize( i ) == memSize );

	const Reader* context = Reader::GetContext( this );

	const AttribStruct* attrib = AttributePtr( i );
	context->GetData( attrib->dataID, mem, memSize );
}


void Item::GetData( const char* name, void* mem, int memSize ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0 );
	return GetData( i, mem, memSize );
}


int	Item::GetDataID( int i ) const
{
	GLASSERT( IsDataType( AttributeType( i )));
	const AttribStruct* attrib = AttributePtr( i );
	return attrib->dataID;
}


int	Item::GetDataID( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0 );
	return GetDataID( i );
}


int Item::GetInt( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_INT );
	if ( AttributeType( i ) == ATTRIBUTE_INT ) {
		const AttribStruct* attrib = AttributePtr( i );
		return attrib->intVal;
	}
	return 0;
}


int Item::GetInt( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetInt( i );
	}
	return 0;
}


int Item::GetArrayLen( const char* name ) const
{
	int type = AttributeType( name ); 
	GLASSERT( type == ATTRIBUTE_INT_ARRAY || type == ATTRIBUTE_FLOAT_ARRAY );
	GLASSERT( sizeof(int) == sizeof(float) );
	if ( type == ATTRIBUTE_INT_ARRAY || type == ATTRIBUTE_FLOAT_ARRAY ) {
		int n = GetDataSize( name );
		GLASSERT( (n % sizeof(int) ) == 0 );
		return n / sizeof(int);
	}
	return 0;
};


int Item::GetArrayLen( int i ) const
{
	int type = AttributeType( i ); 
	GLASSERT( type == ATTRIBUTE_INT_ARRAY || type == ATTRIBUTE_FLOAT_ARRAY );
	GLASSERT( sizeof(int) == sizeof(float) );
	if ( type == ATTRIBUTE_INT_ARRAY || type == ATTRIBUTE_FLOAT_ARRAY ) {
		int n = GetDataSize( i );
		GLASSERT( (n % sizeof(int) ) == 0 );
		return n / sizeof(int);
	}
	return 0;
};


void Item::GetIntArray( const char* name, int nItems, int* array ) const
{
	GLASSERT( AttributeType( name ) == ATTRIBUTE_INT_ARRAY );
	if ( AttributeType( name ) == ATTRIBUTE_INT_ARRAY ) {	
		GetData( name, array, nItems*sizeof(int) );
	}
}


void Item::GetIntArray( int i, int nItems, int* array ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_INT_ARRAY );
	if ( AttributeType( i ) == ATTRIBUTE_INT_ARRAY ) {	
		GetData( i, array, nItems*sizeof(int) );
	}
}


void Item::GetFloatArray( const char* name, int nItems, float* array ) const
{
	GLASSERT( AttributeType( name ) == ATTRIBUTE_FLOAT_ARRAY );
	if ( AttributeType( name ) == ATTRIBUTE_FLOAT_ARRAY ) {	
		GetData( name, array, nItems*sizeof(float) );
	}
}


void Item::GetFloatArray( int i, int nItems, float* array ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_FLOAT_ARRAY );
	if ( AttributeType( i ) == ATTRIBUTE_FLOAT_ARRAY ) {	
		GetData( i, array, nItems*sizeof(float) );
	}
}


float Item::GetFloat( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_FLOAT );
	if ( AttributeType( i ) == ATTRIBUTE_FLOAT ) {
		const AttribStruct* attrib = AttributePtr( i );
		return attrib->floatVal;
	}
	return 0.0f;
}


float Item::GetFloat( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetFloat( i );
	}
	return 0.0f;
}


const char*	Item::GetString( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_STRING );
	if ( AttributeType( i ) == ATTRIBUTE_STRING ) {
		const AttribStruct* attrib = AttributePtr( i );
		const Reader* context = Reader::GetContext( this );
		return context->GetString( attrib->stringID );
	}
	return 0;
}


const char* Item::GetString( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetString( i );
	}
	return 0;
}


bool Item::GetBool( int i ) const
{
	GLASSERT( AttributeType( i ) == ATTRIBUTE_BOOL );
	if ( AttributeType( i ) == ATTRIBUTE_BOOL ) {
		const AttribStruct* attrib = AttributePtr( i );
		return attrib->intVal ? true : false;
	}
	return false;
}


bool Item::GetBool( const char* name ) const
{
	int i = AttributeIndex( name );
	GLASSERT( i >= 0);
	if ( i >= 0 ) {
		return GetBool( i );
	}
	return false;
}


void Reader::Manifest(int maxDepth) const
{
	grinliz::CDynArray<ManifestItem> items;
	ManifestRec(Root(), 1, maxDepth, &items);
	items.Sort();

	printf("\n");
	for (int i = 0; i < items.Size(); ++i) {
		printf("%16s %20s %dk\n", items[i].group.safe_str(), items[i].name.safe_str(), items[i].uncompressedDataSize/1024);
	}

}

void Reader::ManifestRec(const gamedb::Item* item, int depth, int maxDepth, grinliz::CDynArray<ManifestItem>* arr ) const
{
	for (int i = 0; i < item->NumChildren(); ++i) {
		const gamedb::Item* subItem = item->ChildAt(i);
		if (depth == maxDepth) {
			int size = 0;
			for (int j = 0; j < subItem->NumAttributes(); ++j) {
				if (subItem->AttributeType(j) == gamedb::ATTRIBUTE_DATA) {
					int s = subItem->GetDataSize(j);
					if (s > 0)
						size += s;
				}
			}

			ManifestItem mi = { subItem->Name(), item->Name(), size };
			arr->Push(mi);
		}
		else {
			ManifestRec(subItem, depth + 1, maxDepth, arr);
		}
	}
}

