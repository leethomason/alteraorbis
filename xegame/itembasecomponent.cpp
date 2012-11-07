#include "itembasecomponent.h"
#include "../game/gameitem.h"
#include "../engine/engine.h"
#include "../engine/particle.h"
#include "chit.h"
#include "spatialcomponent.h"

void ItemBaseComponent::LowerEmitEffect( Engine* engine, const GameItem& item, U32 delta )
{
	ParticleSystem* ps = engine->particleSystem;

	ComponentSet compSet( parentChit, Chit::ITEM_BIT | Chit::SPATIAL_BIT | ComponentSet::IS_ALIVE );

	if ( compSet.okay ) {
		if ( item.accruedFire > 0 ) {
			ps->EmitPD( "fire", compSet.spatial->GetPosition(), V3F_UP, engine->camera.EyeDir3(), delta );
			ps->EmitPD( "smoke", compSet.spatial->GetPosition(), V3F_UP, engine->camera.EyeDir3(), delta );
		}
		if ( item.accruedShock > 0 ) {
			ps->EmitPD( "shock", compSet.spatial->GetPosition(), V3F_UP, engine->camera.EyeDir3(), delta );
		}
	}
}