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
	healthBar.Init( &map->overlay0, 10, a1, a2 ); 
	healthBar.SetVisible( false );

	RenderAtom blue   = LumosGame::CalcPaletteAtom( 8, 0 );	
	ammoBar.Init( &map->overlay0, 10, blue, blue );
	ammoBar.SetVisible( false );

	RenderAtom purple = LumosGame::CalcPaletteAtom( 10, 0 );
	RenderAtom grey   = LumosGame::CalcPaletteAtom( 0, 6 );
	shieldBar.Init( &map->overlay0, 10, purple, grey );
	shieldBar.SetVisible( false );
}


void DebugStateComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	this->EndSerialize( xs );
}


void DebugStateComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );
	map->overlay0.Add( &healthBar );

	healthBar.SetSize( SIZE_X, SIZE_Y );
	healthBar.SetRange( 1.0f );

	map->overlay0.Add( &ammoBar );
	ammoBar.SetSize( SIZE_X, SIZE_Y );
	ammoBar.SetRange( 1.0f );

	map->overlay0.Add( &shieldBar );
	shieldBar.SetSize( SIZE_X, SIZE_Y );
	shieldBar.SetRange( 1.0f );
}


void DebugStateComponent::OnRemove()
{
	map->overlay0.Remove( &healthBar );
	map->overlay0.Remove( &ammoBar );
	map->overlay0.Remove( &shieldBar );
	super::OnRemove();
}


int DebugStateComponent::DoTick( U32 delta )
{
	GameItem* pItem = parentChit->GetItem();
	if ( !pItem )
		return VERY_LONG_TICK;

	GameItem* weapon = 0;
	GameItem* shield = 0;

	if ( parentChit->GetItemComponent() ) {
		IShield* ishield = parentChit->GetItemComponent()->GetShield();
		IRangedWeaponItem* iweapon = parentChit->GetItemComponent()->GetRangedWeapon(0);

		if ( ishield )
			shield = ishield->GetItem();
		if ( iweapon )
			weapon = iweapon->GetItem();
	}

	healthBar.SetRange( pItem->HPFraction() );

	RenderAtom grey   = LumosGame::CalcPaletteAtom( 0, 6 );
	RenderAtom blue   = LumosGame::CalcPaletteAtom( 8, 0 );	
	RenderAtom orange = LumosGame::CalcPaletteAtom( 4, 0 );

	if ( weapon ) {			
		float r = 1;
		if ( weapon->Reloading() ) {
			r = weapon->ReloadFraction();
			ammoBar.SetLowerAtom( orange );
			ammoBar.SetHigherAtom( grey );
		}
		else {
			r = weapon->RoundsFraction();
			ammoBar.SetLowerAtom( blue );
			ammoBar.SetHigherAtom( grey );
		}
		ammoBar.SetRange( Clamp( r, 0.f, 1.f ) );
	}
	if ( shield ) {
		// will tweak out if there are multiple absorbers.
		float r = shield->RoundsFraction();
		shieldBar.SetRange( Clamp( r, 0.f, 1.0f ));
	}

	ammoBar.SetVisible( true );
	shieldBar.SetVisible( true );
	healthBar.SetVisible( true );

	return VERY_LONG_TICK;
}


void DebugStateComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}

