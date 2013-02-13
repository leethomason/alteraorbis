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

#include "component.h"
#include "chit.h"
#include "chitbag.h"
#include "../grinliz/glstringutil.h"

using namespace grinliz;

int Component::idPool = 1;

Chit* Component::GetChit( int id )
{
	if ( parentChit ) {
		return parentChit->GetChitBag()->GetChit( id );
	}
	return 0;
}


ChitBag* Component::GetChitBag()
{
	if ( parentChit ) {
		return parentChit->GetChitBag();
	}
	return 0;
}


DBItem Component::BeginSerialize( DBItem parent, const char* name )
{
	DBItem i = DBChild( parent, name );
	DB_SERIAL( i, id );
	return i;
}


void Component::BeginSave( tinyxml2::XMLPrinter* printer, const char* name )
{
	printer->OpenElement( name );
	printer->PushAttribute( "id", id );
}


void Component::EndSave( tinyxml2::XMLPrinter* printer )
{
	printer->CloseElement();
}


void Component::BeginLoad( const tinyxml2::XMLElement* element, const char* name )
{
	GLASSERT( StrEqual( element->Value(), name ));
	id = 0;
	element->QueryIntAttribute( "id", &id );
}


void Component::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DESTROYED_START ) {
		parentChit->GetChitBag()->QueueDeleteComponent( this );
	}
}


void MoveComponent::Load( const tinyxml2::XMLElement* element )
{
	GLASSERT( 0 );
}


void MoveComponent::Save( tinyxml2::XMLPrinter* printer )
{
	this->BeginSave( printer, "MoveComponent" );
	this->EndSave( printer );
}


void MoveComponent::Serialize( DBItem parent )
{
	DBItem item = this->BeginSerialize( parent, "MoveComponent" );
}


