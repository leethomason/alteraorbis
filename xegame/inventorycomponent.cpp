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

#include "inventorycomponent.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "../engine/engine.h"

using namespace grinliz;


void InventoryComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	hardpoints = -1;
	packItems.Clear();
	freeItems.Clear();
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		delete intrinsicAt[i];
		intrinsicAt[i] = 0;
		delete heldAt[i];
		heldAt[i] = 0;
	}
}


void InventoryComponent::OnRemove()
{
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		delete intrinsicAt[i];
		intrinsicAt[i] = 0;
		delete heldAt[i];
		heldAt[i] = 0;
	}
	super::OnRemove();
}


void InventoryComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[Inventory] " );
}


static const char* gHardpointNames[NUM_HARDPOINTS] = {
	"trigger",
	"althand",
	"head",
	"shield"
};

const char* InventoryComponent::HardpointFlagToName( int f )
{
	GLASSERT( f >= 0 && f < NUM_HARDPOINTS );
	if ( f >= 0 && f < NUM_HARDPOINTS ) {
		return gHardpointNames[f];
	}
	return 0;
}


int InventoryComponent::HardpointNameToFlag( const char* name )
{
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( StrEqual( name, gHardpointNames[i] ) )
			return i;
	}
	return NO_HARDPOINT;
}


bool InventoryComponent::AddToInventory( GameItem* item, bool equip )
{
	RenderComponent* rc = parentChit->GetRenderComponent();
	if ( hardpoints == -1 ) {
		hardpoints = 0;
		if ( rc ) {
			for( int i=0; i<EL_MAX_METADATA; ++i ) {
				const char* name = rc->GetMetaData(i);
				int h = HardpointNameToFlag( name );		// often 0; lots of metadata isn't a hardpoint
				if ( h != NO_HARDPOINT ) {
					hardpoints |= (1<<h);	
				}
			}
		}
	}
	int attachment = item->AttachmentFlags();
	bool equipped = false;
	
	if ( attachment == GameItem::INTRINSIC_AT_HARDPOINT ) {
		GLASSERT( (1<<item->hardpoint) & hardpoints );	// be sure the attachment point and hardpoint line up.
		GLASSERT( intrinsicAt[item->hardpoint] == 0 );
		intrinsicAt[item->hardpoint] = item;
		equipped = true;
	}
	else if (    attachment == GameItem::INTRINSIC_FREE
		      || attachment == GameItem::HELD_FREE) {
		freeItems.Push( item );
		equipped = true;
	} 
	else if ( attachment == GameItem::HELD_AT_HARDPOINT ) {
		if ( equip ) {

			GLASSERT( hardpoints & (1<<item->hardpoint) );
			// check that the needed hardpoint is free.
			if (    ( hardpoints & (1<<item->hardpoint))	// does the hardpoint exist?
			     && ( heldAt[item->hardpoint] == 0 ) )		// is the hardpoint available?
			{
				heldAt[item->hardpoint] = item;
				equipped = true;

				// Tell the render component to render.
				GLASSERT( rc );
				if ( rc ) {
					const char* n = HardpointFlagToName( item->hardpoint );
					GLASSERT( n );
					rc->Attach( n, item->ResourceName() );
				}
			}
		}
		if ( !equipped ) {
			packItems.Push( item );
		}
	}
	GLASSERT( item->parentChit == 0 );
	item->parentChit = parentChit;
	return equipped;
}


GameItem* InventoryComponent::IsCarrying()
{
	return heldAt[HARDPOINT_TRIGGER];
}


void InventoryComponent::AbsorbDamage( const DamageDesc& dd, DamageDesc* absorbed, const char* logstr )
{
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( heldAt[i] ) {
			heldAt[i]->AbsorbDamage( true, dd, absorbed, logstr );
		}
		if ( intrinsicAt[i] ) {
			intrinsicAt[i]->AbsorbDamage( true, dd, absorbed, logstr );
		}
	}
	if ( absorbed->damage > dd.damage ) {
		absorbed->damage = dd.damage;
	}
	parentChit->SetTickNeeded();
}


// FIXME: write queries to get all held and/or intrinsic and/or free and/or carried items

void InventoryComponent::GetRangedWeapons( grinliz::CArray< RangedInfo, NUM_HARDPOINTS >* weapons )
{
	weapons->Clear();
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		IRangedWeaponItem* ranged = 0;
		if ( heldAt[i] ) {
			ranged = heldAt[i]->ToRangedWeapon();
		}
		else if ( intrinsicAt[i] ) {
			ranged = intrinsicAt[i]->ToRangedWeapon();
		}
		if ( ranged ) {
			RenderComponent* rc = parentChit->GetRenderComponent();
			GLASSERT( rc );
			if ( rc ) {
				Matrix4 xform;
				rc->GetMetaData( HardpointFlagToName(i), &xform );
				Vector3F pos = xform * V3F_ZERO;
				RangedInfo info = { ranged, pos };
				weapons->Push( info );
			}
		}
	}
}


IMeleeWeaponItem* InventoryComponent::GetMeleeWeapon()
{
	if ( heldAt[HARDPOINT_TRIGGER] )
		return heldAt[HARDPOINT_TRIGGER]->ToMeleeWeapon();
	if ( intrinsicAt[HARDPOINT_TRIGGER] )
		return intrinsicAt[HARDPOINT_TRIGGER]->ToMeleeWeapon();
	return 0;
}


GameItem* InventoryComponent::GetShield() 
{
	return heldAt[HARDPOINT_SHIELD];
}


void InventoryComponent::GetChain( GameItem* item, grinliz::CArray< GameItem*, 4 >* chain )
{
	chain->Clear();
	if ( item->hardpoint >= 0 ) {
		if ( heldAt[item->hardpoint] ) {
			GLASSERT( heldAt[item->hardpoint] == item );
			chain->Push( heldAt[item->hardpoint] );
		}
		if ( intrinsicAt[item->hardpoint] ) {
			chain->Push( intrinsicAt[item->hardpoint] );
		}
	}
	if ( parentChit->GetItemComponent() ) {
		chain->Push( parentChit->GetItemComponent()->GetItem() );
	}
}


void InventoryComponent::EmitEffect( Engine* engine, U32 delta )
{
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( intrinsicAt[i] ) {
			LowerEmitEffect( engine, *intrinsicAt[i], delta );
		}
		if ( heldAt[i] ) {
			LowerEmitEffect( engine, *heldAt[i], delta );
		}
	}
}


bool InventoryComponent::DoTick( U32 delta )
{
	bool callback = false;
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( intrinsicAt[i] && intrinsicAt[i]->DoTick(delta) ) {
			callback = true;
		}
		if ( heldAt[i] && heldAt[i]->DoTick(delta) ) {
			callback = true;
		}
	}
	for( int i=0; i<freeItems.Size(); ++i ) {
		if ( freeItems[i] && freeItems[i]->DoTick(delta) )
			callback  = true;
	}
	for( int i=0; i<packItems.Size(); ++i ) {
		if ( packItems[i] && packItems[i]->DoTick(delta) )
			callback  = true;
	}

	GameItem* shield = GetShield();
	if ( shield && parentChit->GetRenderComponent() ) {
		Vector4F c = { 1, 1, 1, shield->RoundsFraction() };
		parentChit->GetRenderComponent()->ParamColor( "shield", c );
	}

	return callback;
}


