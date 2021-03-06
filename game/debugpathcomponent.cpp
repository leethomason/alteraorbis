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

#include "debugpathcomponent.h"
#include "lumosgame.h"
#include "worldmap.h"
#include "gamelimits.h"
#include "pathmovecomponent.h"

#include "../engine/engine.h"
#include "../engine/texture.h"

#include "../xegame/chit.h"
#include "../xegame/chitbag.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chitcontext.h"

using namespace grinliz;

DebugPathComponent::DebugPathComponent()
{
	resource = ModelResourceManager::Instance()->GetModelResource( "unitPlateCentered" );
}


DebugPathComponent::~DebugPathComponent()
{
}


void DebugPathComponent::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	//const ChitContext* context = Context();
	model = new Model(resource, Context()->engine->GetSpaceTree());
	model->SetFlag(Model::MODEL_NO_SHADOW);
}


void DebugPathComponent::OnRemove()
{
	// Get the context before remove - still valid but parent chit goes away.
	//const ChitContext* context = Context();

	super::OnRemove();
	delete model;
	model = 0;
}


int DebugPathComponent::DoTick(U32 delta)
{
	Vector3F pos = parentChit->Position();
	pos.y = 0.01f;

	RenderComponent* render = parentChit->GetRenderComponent();
	float radius = MAX_BASE_RADIUS;

	if (render) {
		radius = render->RadiusOfBase();
		model->SetScale(radius*2.f);
	}
	model->SetPos(pos);
	Vector4F color = { 0, 1, 0, 1 };

	PathMoveComponent* pmc = static_cast<PathMoveComponent*>(parentChit->GetComponent("PathMoveComponent"));

	if (pmc) {
		if (!pmc->IsMoving())
			color.Set(0, 0, 0, 1);
		if (pmc->BlockForceApplied())
			color.Set(1, 1, 0, 1);
		if (pmc->IsAvoiding())
			color.z = 1;
	}

	model->SetColor(color);
	return VERY_LONG_TICK;
}


void DebugPathComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	this->EndSerialize( xs );
}

