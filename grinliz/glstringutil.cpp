/*
Copyright (c) 2000-2005 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include "glstringutil.h"
#include "glrandom.h"
#include "glcontainer.h"

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

using namespace grinliz;

/* static */ const char* GLString::nullBuf = "";

void GLString::init( const GLString& rhs ) 
{
	if ( m_buf != nullBuf ) delete [] m_buf;
	m_buf = (char*)nullBuf;
	m_allocated = 0;
	m_size = 0;

	ensureSize( rhs.size() );
	m_size = rhs.size();
	if ( m_size )
		memcpy( m_buf, rhs.m_buf, m_size );
	m_buf[m_size] = 0;
	validate();
}


void GLString::init( const char* rhs ) 
{
	if ( m_buf != nullBuf ) delete [] m_buf;
	m_buf = (char*)nullBuf;
	m_allocated = 0;
	m_size = 0;

	unsigned strSize = strlen( rhs );
	ensureSize( strSize );
	if ( strSize )
		memcpy( m_buf, rhs, strSize );
	m_size = strSize;
	m_buf[m_size] = 0;
	validate();
}


void GLString::append( const GLString& str )
{
	ensureSize( str.size() + size() );
	if ( str.size() )
		memcpy( m_buf+size(), str.m_buf, str.size() );
	m_size += str.size();
	m_buf[m_size] = 0;
	validate();
}


void GLString::append( const char* str )
{
	int strSize = strlen( str );
	ensureSize( strSize + size() );
	if ( strSize )
		memcpy( m_buf+size(), str, strSize );
	m_size += strSize;
	m_buf[m_size] = 0;
	validate();
}


void GLString::append( const char* str, int n )
{
	int strSize = strlen( str );
	if ( strSize > n )
		strSize = n;

	ensureSize( strSize + size() );
	if ( strSize )
		memcpy( m_buf+size(), str, strSize );
	m_size += strSize;
	m_buf[m_size] = 0;
	validate();
}


GLString GLString::substr( unsigned pos, unsigned n )
{
	GLString str;
	if ( pos < size()-1 ) {
		if ( pos + n > size() ) {
			n = size() - pos;
		}
		str.ensureSize(n);
		if ( n )
			memcpy( str.m_buf, m_buf+pos, n );
		str.m_buf[n] = 0;
		str.m_size = n;
	}
	validate();
	return str;
}


void GLString::Format( const char* format, ...) 
{
	static const int size = 200;
	char str[size] = { 0 };	// sleazy, yes.
    va_list     va;

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    int result = vsnprintf_s( str, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    int result = vsnprintf( str, size, format, va );
	str[size-1] = 0;
#endif
    va_end( va );
	this->append( str );
}


void GLString::ensureSize( unsigned s )
{
	unsigned ensure = s+1;
	if ( m_allocated < ensure ) {
		unsigned newAllocated = ensure * 3/2 + 16;
		char* newBuf = new char[newAllocated];

		if ( m_size )
			memcpy( newBuf, m_buf, m_size );

		if ( m_buf != nullBuf ) delete [] m_buf;
		m_buf = newBuf;
		m_allocated = newAllocated;
		newBuf[m_size] = 0;
	}
	validate();
}


#ifdef DEBUG
void GLString::validate()
{
	if ( m_buf != nullBuf ) {
		GLASSERT( m_allocated );
		GLASSERT( m_allocated > m_size );	// strictly greater: need room for a null terminator!
		GLASSERT( m_buf[m_size] == 0 );
		GLASSERT( m_size == strlen( m_buf ) );
	}
	else {
		GLASSERT( m_allocated == 0 );
		GLASSERT( m_size == 0 );
	}
}
#endif


void grinliz::StrSplitFilename(	const GLString& fullPath, 
								GLString* base,
								GLString* name,
								GLString* extension )
{
	GLString path = fullPath;
	if ( base )
		*base = "";
	if ( name )
		*name = "";
	if ( extension )
		*extension = "";

	unsigned dotPos = path.rfind( '.' );
	if ( dotPos < path.size() ) {
		if ( extension )
			*extension = path.substr( dotPos, path.size() );
		path = path.substr( 0, dotPos );
	}

	unsigned slashPos = path.rfind( '/' );
	unsigned backPos = path.rfind( '\\' );
	unsigned colonPos = path.rfind( ':' );

	unsigned pos = 0;
	bool found = false;
	if ( slashPos < path.size() && slashPos > pos ) {
		pos = slashPos;
		found = true;
	}
	if ( backPos < path.size() && backPos > pos ) {
		pos = backPos;
		found = true;
	}
	if ( colonPos < path.size() && colonPos > pos ) {
		pos = colonPos;
		found = true;
	}
	if ( found && pos < path.size() ) {
		if ( base )
			*base = path.substr( 0, pos+1 );
		path = path.substr( pos+1, path.size() );
	}
	if ( name )
		*name = path;
}


int grinliz::SNPrintf(char *str, size_t size, const char *format, ...)
{
    va_list     va;

    //
    //  format and output the message..
    //
    va_start( va, format );
#ifdef _MSC_VER
    int result = vsnprintf_s( str, size, _TRUNCATE, format, va );
#else
	// Reading the spec, the size does seem correct. The man pages
	// say it will aways be null terminated (whereas the strcpy is not.)
	// Pretty nervous about the implementation, so force a null after.
    int result = vsnprintf( str, size, format, va );
	str[size-1] = 0;
#endif
    va_end( va );

	return result;
}


void grinliz::StrNCpy( char* dst, const char* src, size_t len )
{
	GLASSERT( len > 0 );
	if ( len > 0 && dst && src ) {
		const char* p = src;
		char* q = dst;
		int n = len-1;

		while( *p && n ) {
			*q++ = *p++;
			--n;
		}
		*q = 0;
		dst[len-1] = 0;
		GLASSERT( strlen( dst ) < len );
	}
}


void grinliz::StrFillBuffer( const GLString& str, char* buffer, int bufferSize )
{
	StrNCpy( buffer, str.c_str(), bufferSize );

	if ( bufferSize - 1 - (int)str.size() > 0 ) {
		memset( buffer + str.size() + 1, 0, bufferSize - str.size() - 1 );
	}
	GLASSERT( buffer[bufferSize-1] == 0 );
}


void grinliz::StrFillBuffer( const char* str, char* buffer, int bufferSize )
{
	StrNCpy( buffer, str, bufferSize );

	int size = strlen( str );

	if ( bufferSize - 1 - size > 0 ) {
		memset( buffer + size + 1, 0, bufferSize - size - 1 );
	}
	GLASSERT( buffer[bufferSize-1] == 0 );

}


static const char* SkipWhiteSpace( const char* p ) 
{
	while( p && *p && isspace( *p ) ) {
		++p;
	}
	return p;
}

static const char* FindWhiteSpace( const char* p ) 
{
	while( p && *p && !isspace( *p ) ) {
		++p;
	}
	return p;
}


void grinliz::StrTokenize( const GLString& in, int maxTokens, StrToken *tokens, int* nTokens )
{
	const char* p = in.c_str();
	p = SkipWhiteSpace( p );
	const char* q = FindWhiteSpace( p );

	*nTokens = 0;

	while ( p && *p && q && (*nTokens < maxTokens )) {
		StrToken token;

		GLString str;
		str.append( p, q-p );

		if ( isalpha( *p ) ) {
			token.InitString( str );
		}
		else if (    isdigit( *p ) 
			      || *p == '-' 
				  || *p == '+' ) {
			token.InitNumber( atof( str.c_str() ) );
		}
		tokens[*nTokens] = token;
		*nTokens += 1;

		p = SkipWhiteSpace( q );
		q = FindWhiteSpace( p );
	}
}


StringPool* StringPool::instance = 0;


StringPool* StringPool::Instance() {
	if ( !instance ) {
		instance = new StringPool();
	}
	return instance;
}


StringPool::StringPool()
{
	treeSize = 1+2+4+8+16;
	treeDepth = 5;
	tree = (Node*) calloc( treeSize, sizeof(Node) );
	root = 0;

	nStrings = 0;
	nStored = 0;
	nBlocks = 0;
}


StringPool::~StringPool()
{
	GLOUTPUT(( "StringPool destructor. nStrings=%d nStored=%d treeDepth=%d mem=%d blocks * %d = %d\n",
			   nStrings,
			   nStored,
			   treeDepth,
			   nBlocks,
			   BLOCK_SIZE,
			   BLOCK_SIZE * nBlocks ));

	if ( tree )
		free( tree );
	while( root ) {
		Block* b = root->next;
		free( root );
		root = b;
	}
}


/*
		     0
	    1          2
	  3   4     5     6
	 7 8 9 10 11 12 13 14
*/

IString StringPool::Get( const char* str, bool strIsStaticMem )
{
	if ( !str || !*str ) {
		return IString();
	}
	GLASSERT( strlen( str ) < BLOCK_SIZE-1 );

	U32 hash = Random::Hash( str, U32(-1) ); 
	
	int depth=0; 
	int offset=0;
	Node* node = tree;

	while( depth < treeDepth ) {
		GLASSERT( node < tree + treeSize );
		if ( node->str == 0 ) {
			break;	// empty
		}
		if ( node->hash == hash && StrEqual( node->str, str ) ) {
			return IString( node->str );
		}
		else if ( hash < node->hash ) {
			++depth;
			offset = offset*2 + 1;
			node = tree + offset;
		}
		else {
			++depth;
			offset = offset*2+2;
			node = tree + offset;
		}
	}
	// Didn't find. Do we need to extend the tree?
	if ( offset >= treeSize ) {
		int oldTreeSize = treeSize;
		treeSize += (1<<treeDepth);
		++treeDepth;
		tree = (Node*) realloc( tree, treeSize*sizeof(Node) );
		memset( tree + oldTreeSize, 0, (treeSize-oldTreeSize)*sizeof(Node) );
	}

	node = tree + offset;
	GLASSERT( node->hash == 0 );
	GLASSERT( node->str == 0 );
	node->hash = hash;
	++nStrings;
	if ( strIsStaticMem ) {
		node->str = str;
	}
	else {
		node->str = Add( str );
		++nStored;
	}
	return node->str;
}


const char* StringPool::Add( const char* str )
{
	int len = strlen( str );
	int nBytes = len+1;

	for( Block* b = root; b; b=b->next ) {
		if ( b->nBytes + nBytes < BLOCK_SIZE ) {
			memcpy( b->mem + b->nBytes, str, nBytes );
			const char* s = b->mem + b->nBytes;
			b->nBytes += nBytes;
			return s;
		}
	}
	++nBlocks;
	Block* b = (Block*)malloc( sizeof(Block) );
	b->nBytes = nBytes;
	b->next = root;
	root = b;

	memcpy( b->mem, str, nBytes );
	return b->mem;
}
