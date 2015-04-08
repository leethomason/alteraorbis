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

#include "spatialcomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "xegamelimits.h"
#include "rendercomponent.h"
#include "istringconst.h"
#include "chitcontext.h"

#include "../engine/engine.h"
#include "../engine/particle.h"
#include "../grinliz/glmatrix.h"
#include "../game/lumoschitbag.h"
#include "../game/pathmovecomponent.h"	// see the Teleport() function.

using namespace grinliz;


void SpatialComponent::DebugStr( GLString* str )
{
	str->AppendFormat("[Spatial] ");
}

void SpatialComponent::Serialize( XStream* xs )
{
	BeginSerialize( xs, Name() );
	EndSerialize( xs );
}


void SpatialComponent::OnAdd( Chit* chit, bool init )
{
	Component::OnAdd( chit, init );
}


void SpatialComponent::OnRemove()
{
	Component::OnRemove();
}


void SpatialComponent::Teleport(Chit* chit, const grinliz::Vector3F& pos)
{
	GLASSERT(!GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent));
	if (ToWorld2I(pos) != ToWorld2I(chit->Position())) {

		chit->Context()->engine->particleSystem->EmitPD(ISC::teleport, chit->Position(), V3F_UP, 0);
		chit->SetPosition(pos);
		chit->Context()->engine->particleSystem->EmitPD(ISC::teleport, chit->Position(), V3F_UP, 0);

		// Sigh. Reaching into another component. Didn't think of this when I 
		// added the teleport method.
		while (chit->StackedMoveComponent()) {
			Component* c = chit->Remove(chit->GetMoveComponent());
			delete c;
		}
		GLASSERT(chit->GetMoveComponent());
		PathMoveComponent* pmc = GET_SUB_COMPONENT(chit, MoveComponent, PathMoveComponent);
		if (pmc) pmc->Stop();
	}
}


void SpatialComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}
