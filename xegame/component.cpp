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


LumosChitBag* Component::GetLumosChitBag()
{
	ChitBag* cb = GetChitBag();
	if ( cb ) return cb->ToLumos();
	return 0;
}

void Component::BeginSerialize( XStream* xs, const char* name )
{
	XarcOpen( xs, name );
}


void Component::EndSerialize( XStream* xs )
{
	XarcClose( xs );
}


void Component::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( msg.ID() == ChitMsg::CHIT_DESTROYED_START ) {
		parentChit->GetChitBag()->QueueRemoveAndDeleteComponent( this );
	}
}


void MoveComponent::Serialize( XStream* xs )
{
	XarcOpen( xs, "MoveComponent" );
	XarcClose( xs );
}


