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
#include "../engine/serialize.h"

using namespace grinliz;

MapSpatialComponent::MapSpatialComponent( WorldMap* _map ) : SpatialComponent()
{
	worldMap = _map;
	mode = GRID_IN_USE;
}


void MapSpatialComponent::SetMapPosition( int x, int y )
{
	GLASSERT( parentChit == 0 );
	SetPosition( (float)x + 0.5f, 0.0f, (float)y + 0.5f );
}


void MapSpatialComponent::SetMode( int newMode ) 
{
	GLASSERT( worldMap );
	GLASSERT( newMode == GRID_IN_USE || newMode == GRID_BLOCKED );

	if ( newMode != mode ) {
		mode = newMode;
		Vector2I pos = MapPosition();
		worldMap->ResetPather( pos.x, pos.y );
	}
}


void MapSpatialComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );

	if ( mode == GRID_BLOCKED ) {
		Vector2I pos = MapPosition();
		worldMap->ResetPather( pos.x, pos.y );
	}
}


void MapSpatialComponent::OnRemove()
{
	if ( mode == GRID_BLOCKED ) {
		Vector2I pos = MapPosition();
		worldMap->ResetPather( pos.x, pos.y );
	}
	super::OnRemove();
}


void MapSpatialComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, "MapSpatialComponent" );
	XARC_SER( xs, mode );
	super::Serialize( xs );
	this->EndSerialize( xs );
}

