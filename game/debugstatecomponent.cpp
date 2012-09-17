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

#include "../xegame/spatialcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/xegamelimits.h"


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
}

void DebugStateComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	map->overlay.Add( &healthBar );

	healthBar.SetSize( SIZE_X, SIZE_Y );

	HealthComponent* pHealth = GET_COMPONENT( chit, HealthComponent );
	if ( pHealth ) {
		healthBar.SetRange( pHealth->GetHealthFraction() );
	}
	else {
		healthBar.SetRange( 1.0f );
	}
}


void DebugStateComponent::OnRemove()
{
	map->overlay.Remove( &healthBar );
	Component::OnRemove();
}


void DebugStateComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == SPATIAL_MSG_CHANGED ) {
		Vector2F pos = chit->GetSpatialComponent()->GetPosition2D() + OFFSET;
		healthBar.SetPos( pos.x, pos.y );
	}
	else if ( msg.ID() == HEALTH_MSG_CHANGED ) {
		HealthComponent* pHealth = GET_COMPONENT( chit, HealthComponent );
		healthBar.SetRange( pHealth->GetHealthFraction() );
	}
}

