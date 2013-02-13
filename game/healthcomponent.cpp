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

#include "healthcomponent.h"
#include "gamelimits.h"

#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../xegame/itemcomponent.h"

#include "../grinliz/glutil.h"

using namespace grinliz;


void HealthComponent::Load( const tinyxml2::XMLElement* element )
{
	this->BeginLoad( element, "HealthComponent" );
	element->QueryUnsignedAttribute( "destroyed", &destroyed );
	this->EndLoad( element );
}


void HealthComponent::Save( tinyxml2::XMLPrinter* printer )
{
	this->BeginSave( printer, "HealthComponent" );
	printer->PushAttribute( "destroyed", destroyed );
	this->EndSave( printer );
}


void HealthComponent::Serialize( DBItem parent )
{
	DBItem item = BeginSerialize( parent, "HealthComponent" );
	DB_SERIAL( item, destroyed );
}


void HealthComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::GAMEITEM_TICK ) {
		DeltaHealth();
	}
	else {
		super::OnChitMsg( chit, msg );
	}
}


int HealthComponent::DoTick( U32 delta, U32 since )
{
	if ( destroyed ) {
		destroyed += delta;
		GLASSERT( parentChit );
		if ( destroyed >= COUNTDOWN ) {
			parentChit->SendMessage( ChitMsg( ChitMsg::CHIT_DESTROYED_END ), this );
			GetChitBag()->QueueDelete( parentChit );
		}
		else {
			ChitMsg msg( ChitMsg::CHIT_DESTROYED_TICK );
			msg.dataF = 1.0f - this->DestroyedFraction();
			parentChit->SendMessage( msg );
		}
	}
	return destroyed ? 0 : VERY_LONG_TICK;
}


void HealthComponent::DeltaHealth()
{
	if ( destroyed )
		return;

	GameItem* item = 0;
	if ( parentChit->GetItemComponent() ) {
		item = parentChit->GetItem();
	}
	if ( item ) {
		if ( item->hp == 0 && item->TotalHP() != 0 ) {
			parentChit->SendMessage( ChitMsg( ChitMsg::CHIT_DESTROYED_START), this );
			GLLOG(( "Chit %3d destroyed.\n", parentChit->ID() ));
			destroyed = 1;
		}
	}
}
