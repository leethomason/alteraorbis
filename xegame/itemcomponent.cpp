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

#include "itemcomponent.h"
#include "chit.h"
#include "../game/healthcomponent.h"

void ItemComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DAMAGE ) {
		const DamageDesc* dd = (const DamageDesc*) msg.Ptr();
		GLASSERT( dd );
		GLLOG(( "Chit %3d '%s' ", parentChit->ID(), item.Name() ));

		// FIXME: see if inventory can absorb first (shields)

		float hp = item.hp;
		item.AbsorbDamage( *dd, 0, "DAMAGE" );
		GLLOG(( "\n" ));

		if ( item.hp != hp ) {
			HealthComponent* hc = GET_COMPONENT( parentChit, HealthComponent );
			if ( hc ) {
				hc->DeltaHealth();
			}
		}
	}
}


bool ItemComponent::DoTick( U32 delta )
{
	return item.DoTick( delta );
}


void ItemComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	GLASSERT( !item.parentChit );
	item.parentChit = parentChit;
}


void ItemComponent::OnRemove() 
{
	GLASSERT( item.parentChit == parentChit );
	item.parentChit = 0;
	Component::OnRemove();
}

