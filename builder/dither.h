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

#ifndef UFO_ATTACK_DITHER_INCLUDED
#define UFO_ATTACK_DITHER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../engine/texturetype.h"
#include "../libs/SDL2/include/SDL.h"

void OrderedDitherTo16( const SDL_Surface* surface, TextureType format, bool invert, U16* target );


#endif  // UFO_ATTACK_DITHER_INCLUDED