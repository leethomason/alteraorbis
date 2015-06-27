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

#include "gamedbwriter.h"
#include "../grinliz/glstringutil.h"
#include "../FastLZ/fastlz.h"
#include "gamedb.h"

#if defined( _MSC_VER )
#pragma warning ( disable : 4996 )
#endif

using namespace gamedb;
using namespace grinliz;

void WriteU32( FILE* fp, U32 val )
{
	fwrite( &val, sizeof(U32), 1, fp );
}


Writer::Writer()
{
	stringPool = new StringPool();
	root = new WItem();
	root->Init( "root", 0, this );
}

Writer::~Writer()
{
	delete root;
	delete stringPool;
}


void Writer::Save(const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	GLASSERT(fp);
	if (!fp) {
		return;
	}

	// Get all the strings used for anything to build a string pool.
	CDynArray< IString > poolVec;
	stringPool->GetAllStrings(&poolVec);

	poolVec.Sort();

	// --- Header --- //
	HeaderStruct headerStruct;	// come back later and fill in.
	fwrite(&headerStruct, sizeof(HeaderStruct), 1, fp);

	// --- String Pool --- //
	headerStruct.nString = poolVec.Size();
	U32 mark = ftell(fp) + 4 * poolVec.Size();		// position of first string.
	for (int i = 0; i < poolVec.Size(); ++i) {
		WriteU32(fp, mark);
		mark += poolVec[i].size() + 1;				// size of string and null terminator.
	}
	for (int i = 0; i < poolVec.Size(); ++i) {
		fwrite(poolVec[i].c_str(), poolVec[i].size() + 1, 1, fp);
	}
	// Move to a 32-bit boundary.
	while (ftell(fp) % 4) {
		fputc(0, fp);
	}

	// --- Items --- //
	headerStruct.offsetToItems = ftell(fp);
	grinliz::CDynArray< WItem::MemSize > dataPool;
	grinliz::CDynArray< U8 > buffer;

	root->Save(fp, poolVec, &dataPool);

	// --- Data Description --- //
	headerStruct.offsetToDataDesc = ftell(fp);

	// reserve space for offsets
	grinliz::CDynArray< DataDescStruct > ddsVec;
	ddsVec.PushArr(dataPool.Size());
	fwrite(ddsVec.Mem(), sizeof(DataDescStruct)*ddsVec.Size(), 1, fp);

	// --- Data --- //
	headerStruct.offsetToData = ftell(fp);
	headerStruct.nData = dataPool.Size();

	for (int i = 0; i < dataPool.Size(); ++i) {
		WItem::MemSize* m = &dataPool[i];

		ddsVec[i].offset = ftell(fp);
		ddsVec[i].size = m->size;

		int compressed = 0;
		int compressedSize = 0;

		if (m->size > 20 && dataPool[i].compressData) {
			buffer.Clear();
			buffer.PushArr(fastlz_compress_buffer_size(m->size));

			compressedSize = fastlz_compress(m->mem, m->size, &buffer[0]);
			GLASSERT(compressedSize > 0);

			if (compressedSize < m->size) {
				compressed = 1;
			}
		}

		if (compressed) {
			fwrite(&buffer[0], compressedSize, 1, fp);
			ddsVec[i].compressedSize = compressedSize;
		}
		else {
			fwrite(m->mem, m->size, 1, fp);
			ddsVec[i].compressedSize = m->size;
		}
	}
	unsigned totalSize = ftell(fp);
	(void)totalSize;

	// --- Data Description, revisited --- //
	fseek(fp, headerStruct.offsetToDataDesc, SEEK_SET);
	fwrite(ddsVec.Mem(), sizeof(DataDescStruct)*ddsVec.Size(), 1, fp);

	// Go back and patch header:
	fseek(fp, 0, SEEK_SET);
	fwrite(&headerStruct, sizeof(headerStruct), 1, fp);
	fclose(fp);

	GLOUTPUT(("Database write complete. size=%dk stringPool=%dk tree=%dk data=%dk\n",
		totalSize / 1024,
		headerStruct.offsetToItems / 1024,
		(headerStruct.offsetToData - headerStruct.offsetToItems) / 1024,
		(totalSize - headerStruct.offsetToData) / 1024));
}


WItem::WItem()
{
	offset = 0;

	parent = 0;
	writer = 0;
	child  = 0;
	sibling = 0;
	attrib = 0;
}


WItem::~WItem()
{
	{
		while ( child ) {
			WItem* w = child->sibling;
			delete child;
			child = w;
		}
	}
	{
		// Free the copy of the memory.
		while ( attrib ) {
			Attrib* a = attrib->next;
			delete attrib;
			attrib = a;
		}
	}
	// Everything else cleaned by destructors.
}


void WItem::Init( const char* name, WItem* parent, Writer* writer )
{
	this->itemName = writer->stringPool->Get( name );
	this->parent = parent;
	this->writer = writer;
}


WItem* WItem::FetchChild( const char* name )
{
	GLASSERT( name && *name );
	//IString iname = writer->stringPool->Get( name );
	for( WItem* c = child; c; c=c->sibling ) {
		if ( c->itemName == name )
			return c;
	}
	return CreateChild( name );
}


WItem* WItem::CreateChild( const char* name )
{
	GLASSERT( name && *name );
	//IString iname = writer->stringPool->Get( name );

	WItem* witem = new WItem();
	witem->Init( name, this, writer );

	witem->sibling = this->child;
	this->child = witem;
	return witem;
}


WItem* WItem::FetchChild( int id )
{
	char buffer[32];
	GLASSERT(id >= 0 && id < 10000);
	grinliz::SNPrintf( buffer, 32, "%04d", id );
	return FetchChild( buffer );
}


void WItem::Attrib::Free()
{
	if ( type == ATTRIBUTE_DATA ) {
		free( data );
	}
	Clear();
}


void WItem::SetData( const char* name, const void* d, int nData, bool useCompression )
{
	GLASSERT( name && *name );
	GLASSERT( d && nData );

	void* mem = malloc( nData );
	memcpy( mem, d, nData );

	Attrib* a = new Attrib();
	a->Clear();
	a->name = writer->stringPool->Get( name );
	a->type = ATTRIBUTE_DATA;
	a->data = mem;
	a->dataSize = nData;
	a->compressData = useCompression;

	a->next = attrib;
	attrib = a;
}


void WItem::SetIntArray( const char* name, const int* value, int nItems )
{
	SetData( name, value, nItems*sizeof(int), true );
	// Sleazy trick. The value we just changed should be root:
	GLASSERT( attrib->name == name );
	attrib->type = ATTRIBUTE_INT_ARRAY;	// DATA in every repsect except how it is interpreted.
}


void WItem::SetFloatArray( const char* name, const float* value, int nItems )
{
	SetData( name, value, nItems*sizeof(float), true );
	// Sleazy trick. The value we just changed should be root:
	GLASSERT( attrib->name == name );
	attrib->type = ATTRIBUTE_FLOAT_ARRAY;	// DATA in every repsect except how it is interpreted.
}


void WItem::SetInt( const char* name, int value )
{
	GLASSERT( name && *name );

	Attrib* a = new Attrib();
	a->Clear();
	a->name = writer->stringPool->Get( name );
	a->type = ATTRIBUTE_INT;
	a->intVal = value;

	a->next = attrib;
	attrib = a;
}


void WItem::SetFloat( const char* name, float value )
{
	GLASSERT( name && *name );

	Attrib* a = new Attrib();
	a->Clear();
	a->name = writer->stringPool->Get( name );
	a->type = ATTRIBUTE_FLOAT;
	a->floatVal = value;

	a->next = attrib;
	attrib = a;
}


void WItem::SetString( const char* name, const char* str )
{
	GLASSERT( name && *name );

	Attrib *a = new Attrib();
	a->Clear();
	a->name = writer->stringPool->Get( name );
	a->type = ATTRIBUTE_STRING;
	a->stringVal = writer->stringPool->Get( str );

	a->next = attrib;
	attrib = a;
}


void WItem::SetBool( const char* name, bool value )
{
	GLASSERT( name && *name );
	
	Attrib* a = new Attrib();
	a->Clear();
	a->name = writer->stringPool->Get( name );
	a->type = ATTRIBUTE_BOOL;
	a->intVal = value ? 1 : 0;

	a->next = attrib;
	attrib = a;
}


int WItem::FindString( const IString& str, const CDynArray< IString >& stringPool )
{
	int index = stringPool.BSearch( str );
	GLASSERT( index >= 0 );	// should always succeed!
	GLASSERT( index < stringPool.Size() );
	GLASSERT(StrEqual(stringPool[index].c_str(), str.c_str()));
	return index;
}


void WItem::Save(	FILE* fp, 
					const CDynArray< IString >& stringPool, 
					CDynArray< MemSize >* dataPool )
{
	offset = ftell( fp );

	// Attributes:
	// Pull out the linked list and sort it.
	CDynArray< Attrib* >& attribArr = writer->attribArr;
	attribArr.Clear();
	for( Attrib* a = attrib; a; a=a->next ) {
		attribArr.Push( a );
	}

	attribArr.Sort([](const Attrib* v0, const Attrib* v1) {
		return v0->name < v1->name;
	});

	// Child items:
	// Pull out the linked list and sort it.
	CDynArray< WItem* > childArr;
	childArr.Clear();
	for( WItem* c = child; c; c=c->sibling ) {
		childArr.Push( c );
	}
	childArr.Sort([](const WItem* a, const WItem* b){
		return a->IName() < b->IName();
	});

	ItemStruct itemStruct;
	itemStruct.nameID = FindString( itemName, stringPool );
	itemStruct.offsetToParent = Parent() ? Parent()->offset : 0;
	itemStruct.nAttrib = attribArr.Size();
	itemStruct.nChildren = childArr.Size();

	fwrite( &itemStruct, sizeof(itemStruct), 1, fp );

	// Reserve the child locations.
	int markChildren = ftell( fp );
	for( int i=0; i<childArr.Size(); ++i )
		WriteU32( fp, 0 );
	
	// And now write the attributes:
	for( int i=0; i<attribArr.Size(); ++i ) {
		Attrib* a = attribArr[i];
		AttribStruct aStruct;
		aStruct.SetKeyType( FindString( a->name, stringPool ), a->type );

		switch ( a->type ) {
			case ATTRIBUTE_DATA:
			case ATTRIBUTE_INT_ARRAY:
			case ATTRIBUTE_FLOAT_ARRAY:
				{
					int id = dataPool->Size();
					MemSize m = { a->data, a->dataSize, a->compressData ? true : false };
					dataPool->Push( m );
					aStruct.dataID = id;
				}
				break;

			case ATTRIBUTE_INT:
				aStruct.intVal = a->intVal;
				break;

			case ATTRIBUTE_FLOAT:
				aStruct.floatVal = a->floatVal;
				break;

			case ATTRIBUTE_STRING:
				aStruct.stringID = FindString( a->stringVal, stringPool );
				break;

			case ATTRIBUTE_BOOL:
				aStruct.intVal = a->intVal;
				break;

			default:
				GLASSERT( 0 );
		}
		fwrite( &aStruct, sizeof(aStruct), 1, fp );
	}

	// Save the children
	for( int i=0; i<childArr.Size(); ++i ) {
		// Back up, write the current position to the child offset.
		int current = ftell(fp);
		fseek( fp, markChildren+i*4, SEEK_SET );
		WriteU32( fp, current );
		fseek( fp, current, SEEK_SET );

		// save
		childArr[i]->Save( fp, stringPool, dataPool );
	}
}
