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

#include "debugstatecomponent.h"
#include "worldmap.h"
#include "lumosgame.h"
#include "healthcomponent.h"
#include "gamelimits.h"
#include "gameitem.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/xegamelimits.h"
#include "../xegame/itemcomponent.h"


using namespace gamui;
using namespace grinliz;

static const float SIZE_X = 0.8f;
static const float SIZE_Y = 0.2f;
static const Vector2F OFFSET = { -0.5f, -0.5f };

DebugStateComponent::DebugStateComponent( WorldMap* _map ) : map( _map )
{
	RenderAtom a1 = LumosGame::CalcPaletteAtom( 1, 3 );	
	RenderAtom a2 = LumosGame::CalcPaletteAtom( 1, 1 );
	healthBar.Init( &map->overlay, 10, a1, a2 ); 

	RenderAtom blue   = LumosGame::CalcPaletteAtom( 8, 0 );	
	ammoBar.Init( &map->overlay, 10, blue, blue );

	RenderAtom purple = LumosGame::CalcPaletteAtom( 10, 0 );
	RenderAtom grey   = LumosGame::CalcPaletteAtom( 0, 6 );
	shieldBar.Init( &map->overlay, 10, purple, grey );
}

void DebugStateComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	map->overlay.Add( &healthBar );

	healthBar.SetSize( SIZE_X, SIZE_Y );
	healthBar.SetRange( 1.0f );

	HealthComponent* pHealth = GET_COMPONENT( chit, HealthComponent );
	if ( pHealth ) {
		healthBar.SetRange( pHealth->GetHealthFraction() );
	}

	map->overlay.Add( &ammoBar );
	ammoBar.SetSize( SIZE_X, SIZE_Y );
	ammoBar.SetRange( 1.0f );
	ItemComponent* pItem = chit->GetItemComponent();
	if ( pItem ) {
		float r = (float)pItem->GetItem()->rounds / (float)pItem->GetItem()->clipCap;
		ammoBar.SetRange( r ); 
	}

	map->overlay.Add( &shieldBar );
	shieldBar.SetSize( SIZE_X, SIZE_Y );
	shieldBar.SetRange( 1.0f );

}


void DebugStateComponent::OnRemove()
{
	map->overlay.Remove( &healthBar );
	map->overlay.Remove( &ammoBar );
	map->overlay.Remove( &shieldBar );
	Component::OnRemove();
}


void DebugStateComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::SPATIAL_CHANGED ) {
		Vector2F pos = chit->GetSpatialComponent()->GetPosition2D() + OFFSET;
		healthBar.SetPos( pos.x, pos.y );
		ammoBar.SetPos( pos.x, pos.y + SIZE_Y*1.5f );
		shieldBar.SetPos( pos.x, pos.y + SIZE_Y*3.0f );
	}
	else if ( msg.ID() == ChitMsg::HEALTH_CHANGED ) {
		HealthComponent* pHealth = GET_COMPONENT( chit, HealthComponent );
		healthBar.SetRange( pHealth->GetHealthFraction() );
	}
	else if ( msg.ID() == ChitMsg::GAMEITEM_TICK ) {
		GameItem* pItem = (GameItem*)msg.Ptr();
		RenderAtom grey   = LumosGame::CalcPaletteAtom( 0, 6 );
		RenderAtom blue   = LumosGame::CalcPaletteAtom( 8, 0 );	
		RenderAtom orange = LumosGame::CalcPaletteAtom( 4, 0 );

		if ( pItem->ToRangedWeapon() ) {			
			float r = 1;
			if ( pItem->Reloading() ) {
				if ( pItem->reload ) {
					r = (float)pItem->reloadTime / (float)pItem->reload;
				}
				ammoBar.SetLowerAtom( orange );
				ammoBar.SetHigherAtom( grey );
			}
			else {
				if ( pItem->clipCap ) {
					r = (float)pItem->rounds / (float)pItem->clipCap;
				}
				ammoBar.SetLowerAtom( blue );
				ammoBar.SetHigherAtom( grey );
			}
			ammoBar.SetRange( Clamp( r, 0.f, 1.f ) );
		}
		if ( pItem->absorbsDamage != 0 ) {
			// will tweak out if there are multiple absorbers.
			float r = 1;
			if ( pItem->TotalHP() ) {
				r = pItem->hp / pItem->TotalHP();
			}
			shieldBar.SetRange( Clamp( r, 0.f, 1.0f ));
		}
	}
}

