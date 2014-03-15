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
#include "../script/evalbuildingscript.h"

using namespace grinliz;

MapSpatialComponent::MapSpatialComponent() : SpatialComponent()
{
	mode = GRID_IN_USE;
	building = false;
	hasPorch = 0;
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


void MapSpatialComponent::UpdateBlock( WorldMap* map )
{
	for( int y=bounds.min.y; y<=bounds.max.y; ++y ) {
		for( int x=bounds.min.x; x<=bounds.max.x; ++x ) {
			map->UpdateBlock( x, y );
		}
	}
}


void MapSpatialComponent::SetMode( int newMode ) 
{
	mode = newMode;	// UpdateBlock() makes callback occur - set mode first!
	// This code gets run on OnAdd() as well.
	if ( parentChit ) {
		if ( parentChit ) {
			UpdateBlock( GetChitContext()->worldMap );
		}
	}
}


void MapSpatialComponent::SetBuilding( bool b, bool p )
{
	GLASSERT( !parentChit );
	building = b;
	hasPorch = p ? 1 : 0;
}



void MapSpatialComponent::UpdatePorch( bool clearPorch )
{
	GLASSERT(this->parentChit);
	WorldMap* worldMap = this->parentChit ? this->GetChitContext()->worldMap : 0;
	LumosChitBag* bag  = this->parentChit ? this->GetLumosChitBag() : 0;

	if (clearPorch) {
		hasPorch = 0;
	}
	else if (building && hasPorch) {
		hasPorch = 1;	// standard porch.
		EvalBuildingScript* evalScript = static_cast<EvalBuildingScript*>(parentChit->GetScript("EvalBuildingScript"));
		if (evalScript) {
			GameItem* item = parentChit->GetItem();
			GLASSERT(item);
			if (item) {
				double consumes = item->GetBuildingIndustrial(false);
				if (consumes) {
					double scan = evalScript->EvalIndustrial(false);

					double dot = scan * consumes;
					int q = int((1.0 + dot) * 2.0 + 0.5);
					// q=0, no porch. q=1 default.
					hasPorch = q + 2;
					GLASSERT(hasPorch > 1 && hasPorch < WorldGrid::NUM_PORCH);
				}
			}
		}
	}
	Rectangle2I b = bounds;
	b.Outset( 1 );
	Rectangle2IEdgeIterator it( b );

	for( it.Begin(); !it.Done(); it.Next() ) {
		int type = 0;
		Chit* porch = bag->QueryPorch( it.Pos(), &type );
		worldMap->SetPorch( it.Pos().x, it.Pos().y, type );
	}
}



void MapSpatialComponent::SetPosRot( const grinliz::Vector3F& v, const grinliz::Quaternion& quat )
{
	super::SetPosRot( v, quat );
	if ( parentChit ) {
		UpdatePorch(false);
	}
}


void MapSpatialComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	if ( building ) {
		GetLumosChitBag()->AddToBuildingHash( this, bounds.min.x, bounds.min.y ); 
		UpdatePorch(false);
	}

	if ( mode == GRID_BLOCKED ) {
		UpdateBlock( GetChitContext()->worldMap );
	}
}


void MapSpatialComponent::OnRemove()
{
	const ChitContext* context = GetChitContext();
	LumosChitBag* chitBag = GetLumosChitBag();
	
	if ( building ) {
		Vector2I pos = GetPosition2DI();
		GetLumosChitBag()->RemoveFromBuildingHash( this, bounds.min.x, bounds.min.y ); 
	}
	UpdatePorch(true);

	// Remove so that the callback doesn't return blocking.
	super::OnRemove();

	if ( mode == GRID_BLOCKED ) {
		// This component is no longer in the block (the OnRemove() is above
		// this LOC), so this will set things to the correct value.
		UpdateBlock( context->worldMap );
	}
}


void MapSpatialComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, "MapSpatialComponent" );
	XARC_SER( xs, mode );
	XARC_SER( xs, building );
	XARC_SER( xs, hasPorch );
	XARC_SER( xs, bounds );
	super::Serialize( xs );
	this->EndSerialize( xs );
}


Rectangle2I MapSpatialComponent::PorchPos() const
{
	Rectangle2I v;
	v.Set( 0, 0, 0, 0 );
	if ( !hasPorch ) return v;

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

