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
#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

using namespace grinliz;

DebugPathComponent::DebugPathComponent( Engine* _engine, WorldMap* _map, LumosGame* _game )
{
	engine = _engine;
	map = _map;
	game = _game;
	resource = ModelResourceManager::Instance()->GetModelResource( "unitPlateCentered" );
}


DebugPathComponent::~DebugPathComponent()
{
}


void DebugPathComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	model = engine->AllocModel( resource );
	model->SetFlag( Model::MODEL_NO_SHADOW );
}


void DebugPathComponent::OnRemove()
{
	Component::OnRemove();
	if ( model ) {
		engine->FreeModel( model );
		model = 0;
	}
}


bool DebugPathComponent::DoTick( U32 delta )
{
	SpatialComponent* spatial = parentChit->GetSpatialComponent();
	if ( spatial ) {
		Vector3F pos = spatial->GetPosition();
		pos.y = 0.01f;

		RenderComponent* render = parentChit->GetRenderComponent();
		float radius = MAX_BASE_RADIUS;

		if ( render ) {
			radius = render->RadiusOfBase();
			model->SetScale( radius*2.f );
		}
		model->SetPos( pos );
		Vector4F color = { 0, 1, 0, 1 };
		
		PathMoveComponent* pmc = static_cast<PathMoveComponent*>( parentChit->GetComponent( "PathMoveComponent" ) );

		if ( pmc ) {
			if ( !pmc->IsMoving() )
				color.Set( 0, 0, 0, 1 );
			if ( pmc->BlockForceApplied() )
				color.Set( 1, 1, 0, 1 );
			if ( pmc->IsAvoiding() ) 
				color.z = 1;
			if ( pmc->IsStuck() )
				color.Set( 1, 0, 0, 1 );
		}

		model->SetColor( color );
	}
	return false;
}

