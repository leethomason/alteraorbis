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
#include "../xegame/rendercomponent.h"
#include "../xegame/game.h"
#include "../xegame/spatialcomponent.h"

#include "../grinliz/glutil.h"

#include "../engine/engine.h"
#include "../engine/particle.h"

#include "../audio/xenoaudio.h"

#include "../script/procedural.h"
#include "../script/countdownscript.h"
#include "../script/itemscript.h"

#include "../game/lumoschitbag.h"

using namespace grinliz;


void HealthComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	//XARC_SER_DEF( xs, destroyed, 0 );
	this->EndSerialize( xs );
}


float HealthComponent::DestroyedFraction() const 
{ 
	return (float)parentChit->Destroyed() / (float)COUNTDOWN; 
}

int HealthComponent::DoTick(U32 delta)
{
	DeltaHealth();
	if ( parentChit->Destroyed() ) {
		parentChit->SetDestroyed(this, parentChit->Destroyed() + delta);
		if ( parentChit->Destroyed() >= COUNTDOWN ) {
			parentChit->SendMessage( ChitMsg( ChitMsg::CHIT_DESTROYED_END ), this );
			Context()->chitBag->QueueDelete( parentChit );
		}
		else {
			ChitMsg msg( ChitMsg::CHIT_DESTROYED_TICK );
			msg.dataF = 1.0f - this->DestroyedFraction();
			parentChit->SendMessage( msg );
		}
	}
	return parentChit->Destroyed() ? 0 : VERY_LONG_TICK;
}


void HealthComponent::DeltaHealth()
{
	if ( parentChit->Destroyed() )
		return;
	const ChitContext* context = Context();

	GameItem* item = 0;
	if ( parentChit->GetItemComponent() ) {
		item = parentChit->GetItem();
	}
	if ( item ) {
		if ( item->hp == 0 && !parentChit->Destroyed() ) {
			parentChit->SendMessage( ChitMsg( ChitMsg::CHIT_DESTROYED_START), this );
//			GLLOG(( "Chit %3d destroyed.\n", parentChit->ID() ));
			parentChit->SetDestroyed(this, 1);

			const GameItem* item = parentChit->GetItem();
			if ( item && parentChit->GetSpatialComponent() ) {

				// Audio.
				if (XenoAudio::Instance()) {
					MOBIshFilter mobIsh;
					BuildingFilter bulding;
					if (mobIsh.Accept(parentChit) || bulding.Accept(parentChit)) {
						XenoAudio::Instance()->PlayVariation(ISC::derezWAV, parentChit->random.Rand(), &parentChit->GetSpatialComponent()->GetPosition());
					}
				}

				// Tombstone
				IString mob = item->keyValues.GetIString( "mob" );
				if ( !mob.empty() ) {
					const char* asset = 0;
					if ( mob == ISC::lesser ) asset = "tombstoneLesser";
					else if ( mob == ISC::greater ) asset = "tombstoneGreater";
					else if ( mob == ISC::denizen ) asset = "tombstoneDenizen";

					if ( asset ) {
						Chit* chit = Context()->chitBag->NewChit();

						Context()->chitBag->AddItem( "tombstone", chit, context->engine, 0, 0, asset );
						chit->Add( new SpatialComponent() );
						chit->Add( new RenderComponent( asset ));
						chit->Add( new CountDownScript( 30*1000 ));

						Vector3F pos = parentChit->GetSpatialComponent()->GetPosition();
						float r = parentChit->GetSpatialComponent()->GetYRotation();
						pos.y = 0;
						chit->GetSpatialComponent()->SetPosYRot( pos, r );

						GameItem* tombItem = chit->GetItem();
						GLASSERT( item->Team() != 0 );	// how would a neutral create a tombstone??
						tombItem->keyValues.Set( "tomb_team", item->Team() );
						tombItem->keyValues.Set( "tomb_mob", mob );
					}
				}
			}
		}
	}
}


void HealthComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	if ( chit == parentChit && msg.ID() == ChitMsg::RENDER_IMPACT ) {
		if ( !parentChit->Destroyed()) {
			RenderComponent* render = parentChit->GetRenderComponent();
			GLASSERT( render );	// it is a message from the render component, after all.
			ItemComponent* inventory = parentChit->GetItemComponent();
			GLASSERT( inventory );	// need to be  holding a melee weapon. possible the weapon
									// was lost before impact, in which case this assert should
									// be removed.

			MeleeWeapon* item=inventory->GetMeleeWeapon();
			if ( render && inventory && item  ) { /* okay */ }
			else return;

			Vector3F pos;
			render->CalcTrigger( &pos, 0 );

			const ChitContext* context = Context();
			BattleMechanics::MeleeAttack( context->engine, parentChit, item );
			context->engine->particleSystem->EmitPD( ISC::meleeImpact, pos, V3F_UP, 0 );
		}
	}
	else {
		super::OnChitMsg( chit, msg );
	}
}
