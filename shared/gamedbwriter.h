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

#ifndef GRINLIZ_GAME_DB_WRITER_INCLUDED
#define GRINLIZ_GAME_DB_WRITER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmemorypool.h"

#include <stdio.h>

namespace gamedb
{
class Writer;

/**
	A Write-Item. Used when writing to build a tree to be placed in the database.
*/
class WItem
{
	friend class Writer;
public:
	/// Construct a WItem. The name must be unique amongst its siblings.
	WItem();
	~WItem();

	void Init( const char* name, WItem* parent, Writer* writer );

	/// Query the name of this item.
	const char* Name() const		{ return itemName.c_str(); }
	grinliz::IString IName() const	{ return itemName; }

	/// The parent link
	const WItem* Parent() const { return parent; }

	/// Create a child item by name.
	WItem* CreateChild( const char* name );
	/** Create a child item by number, not this simple creates the string value of the name.
		CreateChild( 1 ) and CreateChild( "1" ) are exactly the same. 
	*/
	WItem* CreateChild( int id );
		
	/// Get a Child if it exists, create it if the child does not.
	WItem* FetchChild( const char* name );

	/// Add/Set binary data (or long/unique string). Data is compressed by default.
	void SetData( const char* name, const void* data, int nData, bool compress=true );
	/// Add/Set a integer attribute.
	void SetInt( const char* name, int value );
	/// Add/Set a floating point attribute
	void SetFloat( const char* name, float value );
	/// Add/Set a string attribute. Placed in the string pool.
	void SetString( const char* name, const char* str );
	/// Add/Set a boolean attribute.
	void SetBool( const char* name, bool value );

	struct MemSize {
		const void* mem;
		int size;
		bool compressData;
	};
	void Save(	FILE* fp, 
				const grinliz::CDynArray< grinliz::IString >& stringPool, 
				grinliz::CDynArray< MemSize >* dataPool );

	// internal
	int offset;	// valid after save

private:

	struct Attrib
	{
		void Clear()	{ type=0; data=0; dataSize=0; intVal=0; floatVal=0; stringVal = grinliz::IString(); next=0; }
		void Free();

		grinliz::IString name;
		int type;				// ATTRIBUTE_INT, etc.
		
		void* data;
		int dataSize;
		bool compressData;

		int intVal;
		float floatVal;
		grinliz::IString stringVal;

		Attrib* next;
	};

	class CompAttribPtr {
	public:
		// Sort:
		static bool Less( const Attrib* v0, const Attrib* v1 )	{ return v0->name < v1->name; }
	};

	int FindString( const grinliz::IString& str, const grinliz::CDynArray< grinliz::IString >& stringPoolVec );

	grinliz::IString itemName;
	const WItem*	parent;
	Writer*			writer;
	WItem*			child;
	WItem*			sibling;
	Attrib*			attrib;
};


class CompWItemPtr {
public:
	// Sort:
	static bool Less( const WItem* v0, const WItem* v1 )	{ return v0->IName() < v1->IName(); }
};


/** Utility class to create a gamedb database. Memory aggressive; meant for tooling
    not for runtime. Basic use:

	# Construct the class
	# Call Root() to get the top node
	# Add you data by addig WItems to the Root(), and WItems to other WItems
	# When all the WItems are set up, call Save()
	# Delete the class. Deleting the class will delete all the WItems attached to the root.
*/
class Writer
{
	friend class WItem;
public:
	Writer();		///<
	~Writer();

	/// Write the database.
	void Save( const char* filename );

	/** Access the root element to be writted. All application Items
	    are children (or sub-children) of the Root.
	*/
	WItem* Root()		{ return root; }

private:

	WItem*									root;
	grinliz::StringPool*					stringPool;
	grinliz::MemoryPoolT< WItem::Attrib >	attribMem; 
	grinliz::MemoryPoolT< WItem >			witemMem; 
	grinliz::CDynArray< WItem::Attrib* >	attribArr;		// cache. (Some trick doesn't work for child items, where multiple caches would be needed.)
};


};	// gamedb
#endif	// GRINLIZ_GAME_DB_WRITER_INCLUDED