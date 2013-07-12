/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFOATTACK_SERIALIZE_INCLUDED
#define UFOATTACK_SERIALIZE_INCLUDED

/*	WARNING everything assumes little endian. Nead to rework
	save/load if this changes.
*/

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glmatrix.h"

#include "../shared/gamedbreader.h"
#include "../tinyxml2/tinyxml2.h"
#include "enginelimits.h"

struct SDL_RWops;


struct ModelGroup
{
	grinliz::CStr<EL_FILE_STRING_LEN> textureName;
	U16						nVertex;
	U16						nIndex;

	void Set( const char* textureName, int nVertex, int nIndex );
	void Load( const gamedb::Item* item );
};


// Count the non-zero section of an array.
template< class T >
int CountArray( const T* arr, int n ) {
	int count = 0;
	for( int i=0; i<n; ++i ) {
		if ( arr[i] ) {
			count = i+1;
		}
	}
	return count;
}

void LoadVector( const tinyxml2::XMLElement* element, grinliz::Vector3F* vec );
void LoadColor( const tinyxml2::XMLElement* element, grinliz::Color3F* color );
void LoadColor( const tinyxml2::XMLElement* element, grinliz::Color4F* color );
void LoadColor( const tinyxml2::XMLElement* element, grinliz::Vector4F* color );

//void XEArchive( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, 
//				U32& value );
void XEArchive( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, 
				grinliz::Vector2I& vec );
void XEArchive( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, 
				grinliz::Vector3F& vec );
void XEArchive( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, 
				grinliz::Vector4F& vec, const grinliz::Vector4F* defaultValue=0 );
void XEArchive( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, 
				grinliz::Quaternion& vec );
void XEArchive( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, 
				grinliz::Matrix4& vec );

template< class T >
void XEArchiveT( tinyxml2::XMLPrinter* printer, const tinyxml2::XMLElement* element, const char* name, T& value )
{
	GLASSERT( !(printer && element));
	GLASSERT( printer || element );
	GLASSERT( name && *name );

	if ( printer ) 
		printer->PushAttribute( name, value );

	if ( element ) {
		value = 0;
		element->QueryAttribute( name, &value );
	}
}


#define XE_ARCHIVE( name ) XEArchiveT( prn, ele, #name, name );

#endif // UFOATTACK_SERIALIZE_INCLUDED
