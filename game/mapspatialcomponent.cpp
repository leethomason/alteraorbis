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

#include "mapspatialcomponent.h"
#include "worldmap.h"

using namespace grinliz;

MapSpatialComponent::MapSpatialComponent( int width, int height, WorldMap* _map ) : SpatialComponent()
{
	map = _map;
	mapPos.Set( -1, -1 );
	mapSize.Set( width, height );
	mapRotation = 0;
}


void MapSpatialComponent::SetMapPosition( int x, int y, int r )
{
	if ( x != mapPos.x || y != mapPos.y || r != mapRotation ) {

		if ( mapPos.x >= 0 ) {
			Rectangle2I rb;
			rb.Set( mapPos.x, mapPos.y, mapPos.x+mapSize.x-1, mapPos.y+mapSize.y-1 );
			map->ClearBlocked( rb );
		}

		float wx, wz;
		GLASSERT( r == 0 || r == 90 || r == 180 || r == 270  );
		mapRotation = r;

		Rectangle2I rb;
		rb.min.Set( x, y );

		if ( mapRotation == 0 || mapRotation == 180 ) {
			wx = (float)x + (float)mapSize.x*0.5f;
			wz = (float)y + (float)mapSize.y*0.5f;
			rb.max.Set( x+mapSize.x-1, y+mapSize.y-1 );
		}
		else {
			wx = (float)x + (float)mapSize.y*0.5f;
			wz = (float)y + (float)mapSize.x*0.5f;
			rb.max.Set( x+mapSize.y-1, y+mapSize.x-1 );
		}		
		map->SetBlocked( rb );

		SetPosition( wx, 0, wz );
		SetYRotation( (float)mapRotation );
	}
}




