#include "inventorycomponent.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "itemcomponent.h"
#include "chit.h"

using namespace grinliz;


void InventoryComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
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
	Component::OnRemove();
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
			if (    ( hardpoints & (1<<item->hardpoint))		// does the hardpoint exist?
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
	return equipped;
}


GameItem* InventoryComponent::IsCarrying()
{
	return heldAt[HARDPOINT_TRIGGER];
}


void InventoryComponent::GetRangedWeapons( grinliz::CArray< IRangedWeaponItem*, NUM_HARDPOINTS >* weapons )
{
	weapons->Clear();
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( heldAt[i] ) {
			IRangedWeaponItem* ranged = heldAt[i]->ToRangedWeapon();
			if ( ranged ) {
				weapons->Push( ranged );
			}
		}
		else if ( intrinsicAt[i] ) {
			IRangedWeaponItem* ranged = heldAt[i]->ToRangedWeapon();
			if ( ranged ) {
				weapons->Push( ranged );
			}
			weapons->Push( ranged );
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

