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

#ifndef UFO_BUILDER_INCLUDED
#define UFO_BUILDER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml2/tinyxml2.h"

struct SDL_Surface;

SDL_Surface* LoadImage( const char* pathname );

grinliz::Color4U8 GetPixel( const SDL_Surface *surface, int x, int y);
void PutPixel(SDL_Surface *surface, int x, int y, const grinliz::Color4U8& pixel);

void ParseNames(	const tinyxml2::XMLElement* element, 
					grinliz::GLString* _assetName, 
					grinliz::GLString* _fullPath, 
					grinliz::GLString* _extension, 
					grinliz::GLString* _fullPath2=0 );

void ExitError( const char* tag, 
				const char* pathName,
				const char* assetName,
				const char* message );

void StringToVector( const char* str, grinliz::Vector3F* vec );
void StringToVector( const char* str, grinliz::Vector4F* vec );


#endif // UFO_BUILDER_INCLUDED