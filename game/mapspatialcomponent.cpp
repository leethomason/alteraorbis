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
	building = false;
}


void MapSpatialComponent::SetMapPosition( int x, int y, int cx, int cy )
{
	GLASSERT( parentChit == 0 );
	bounds.Set( x, y, x+cx-1, y+cy-1 );
	GLASSERT( cx <= MAX_BUILDING_SIZE );
	GLASSERT( cy <= MAX_BUILDING_SIZE );
	
	float px = (float)x + (float)cx*0.5f;
	float py = (float)y + (float)cy*0.5f;

	SetPosition( px, 0, py );
}


void MapSpatialComponent::SetMode( int newMode ) 
{
	GLASSERT( worldMap );
	GLASSERT( newMode == GRID_IN_USE || newMode == GRID_BLOCKED );

	if ( newMode != mode ) {
		mode = newMode;

		for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
			for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
				worldMap->UpdateBlock( x, y );
			}
		}
	}
}


void MapSpatialComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );

	if ( mode == GRID_BLOCKED ) {
		Vector2I pos = MapPosition();
		for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
			for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
				worldMap->UpdateBlock( x, y );
			}
		}
	}
}


void MapSpatialComponent::OnRemove()
{
	if ( mode == GRID_BLOCKED ) {
		Vector2I pos = MapPosition();
		for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
			for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
				worldMap->UpdateBlock( x, y );
			}
		}
	}
	super::OnRemove();
}


void MapSpatialComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, "MapSpatialComponent" );
	XARC_SER( xs, mode );
	XARC_SER( xs, building );
	XARC_SER( xs, bounds );
	super::Serialize( xs );
	this->EndSerialize( xs );
}


Vector2I MapSpatialComponent::PorchPos() const
{
	Vector2I v = bounds.min;

	int r = LRintf( this->GetYRotation() / 90.0f );
	// FIXME: only works for 1x1
	GLASSERT( r == 0 );

	switch (r) {
	case 0:		v.y = bounds.max.y + 1;	break;
	case 1:		v.x += 1;	break;
	case 2:		v.y -= 1;	break;
	case 3:		v.x -= 1;	break;
	default:	GLASSERT(0);	break;
	}
	return v;
}

