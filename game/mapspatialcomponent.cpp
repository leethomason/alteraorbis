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
#include "lumoschitbag.h"
#include "../engine/serialize.h"
#include "../xegame/itemcomponent.h"

using namespace grinliz;

MapSpatialComponent::MapSpatialComponent( WorldMap* _map, LumosChitBag* bag ) : SpatialComponent()
{
	worldMap = _map;
	chitBag = bag;
	mode = GRID_IN_USE;
	building = false;
	nextBuilding = 0;
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


void MapSpatialComponent::SetBuilding( bool b )
{
	GLASSERT( !parentChit );
	building = b;
}


void MapSpatialComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	if ( building ) {
		chitBag->AddToBuildingHash( this, bounds.min.x, bounds.min.y ); 
	}

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
	if ( building ) {
		Vector2I pos = GetPosition2DI();
		chitBag->RemoveFromBuildingHash( this, bounds.min.x, bounds.min.y ); 
	}
	super::OnRemove();
	if ( mode == GRID_BLOCKED ) {
		Vector2I pos = MapPosition();
		for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
			for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
				worldMap->UpdateBlock( x, y );
			}
		}
	}
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


Vector2I MapSpatialComponent::PorchPos( int id ) const
{
	Rectangle2I r = PorchPos();
	Vector2I v = { 0, 0 };
	if ( r.Width() == 0 ) {
		return v;
	}

	v.Set(	r.min.x + (r.Width()-1)  * (id & 1),
			r.min.y + (r.Height()-1) * (id & 1) );
	return v;
}


Rectangle2I MapSpatialComponent::PorchPos() const
{
	Rectangle2I v;
	v.Set( 0, 0, 0, 0 );

	GameItem* item = parentChit->GetItem();
	if ( !item ) return v;
	if ( item->keyValues.GetIString( "porch" ).empty() ) return v;

	// picks up the size, so we only need to 
	// adjust one coordinate for the porch below
	v.min = bounds.min;
	v.max = bounds.max;

	int r = LRintf( this->GetYRotation() / 90.0f );

	switch (r) {
	case 0:		v.min.y = v.max.y = bounds.max.y + 1;	break;
	case 1:		v.min.x = v.max.x = bounds.max.x + 1;	break;
	case 2:		v.min.y = v.max.y = bounds.min.y - 1;	break;
	case 3:		v.min.x = v.max.x = bounds.min.x - 1;	break;
	default:	GLASSERT(0);	break;
	}

	return v;
}

