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
	mode = USES_GRID;
}


void MapSpatialComponent::SetMapPosition( int x, int y )
{
	GLASSERT( parentChit == 0 );
	SetPosition( (float)x + 0.5f, 0.0f, (float)y + 0.5f );
}


void MapSpatialComponent::SetMode( int newMode ) 
{
	GLASSERT( worldMap );
	GLASSERT( newMode == USES_GRID || newMode == BLOCKS_GRID );
	GLASSERT( mode == USES_GRID || mode == BLOCKS_GRID );
	Vector2I pos = MapPosition();

	if ( parentChit && ( newMode != mode )) {
		GLASSERT( worldMap->InUse( pos.x, pos.y ));
		if ( newMode == USES_GRID ) {
			worldMap->ClearBlocked( pos.x, pos.y );
		}
		else if ( newMode == BLOCKS_GRID ) {
			worldMap->SetBlocked( pos.x, pos.y );
		}
	}

	mode = newMode;
	if ( parentChit ) {
		GLASSERT( (mode != BLOCKS_GRID) || worldMap->IsBlocked( pos.x, pos.y ));
		GLASSERT( (mode == BLOCKS_GRID) || !worldMap->IsBlocked( pos.x, pos.y ));
		GLASSERT( worldMap->InUse( pos.x, pos.y ));
	}
}


void MapSpatialComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );

	Vector2I pos = MapPosition();
	const WorldGrid& wg = worldMap->GetWorldGrid( pos.x, pos.y );

	// Messy - since this interacts with the map (which
	// is saved as a straight block of data.) Needs the "just loaded"
	// flag.
	// Fragile, nasty code.
	if ( !worldMap->InUse( pos.x, pos.y ) ) {
		worldMap->SetInUse( pos.x, pos.y, true );
	}
	if ( mode == BLOCKS_GRID ) {
		if ( !worldMap->IsBlocked( pos.x, pos.y ) ) {
			worldMap->SetBlocked( pos.x, pos.y );
		}
	}
	else {
		if ( worldMap->IsBlocked( pos.x, pos.y ) ) {
			worldMap->ClearBlocked( pos.x, pos.y );
		}
	}
}


void MapSpatialComponent::OnRemove()
{
	Vector2I pos = MapPosition();
	const WorldGrid& wg = worldMap->GetWorldGrid( pos.x, pos.y );

	worldMap->SetInUse( pos.x, pos.y, false );
	if ( mode == BLOCKS_GRID ) {
		worldMap->ClearBlocked( pos.x, pos.y );
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

