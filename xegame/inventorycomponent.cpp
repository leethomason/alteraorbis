#include "inventorycomponent.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "chit.h"

using namespace grinliz;


void InventoryComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	hardpoints = -1;
	hardpointsInUse = 0;
	equippedItems.Clear();
	packItems.Clear();
}


void InventoryComponent::OnRemove()
{
	Component::OnRemove();
}


void InventoryComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[Inventory] " );
}


const char* InventoryComponent::HardpointFlagToName( int f )
{
	if ( f & GameItem::HARDPOINT_TRIGGER )
		return "trigger";
	else if ( f & GameItem::HARDPOINT_SHIELD )
		return "shield";
	return 0;
}


int InventoryComponent::HardpointNameToFlag( const char* name )
{
	if ( StrEqual( name, "trigger" ) ) 
		return GameItem::HARDPOINT_TRIGGER;
	else if ( StrEqual( name, "althand" ))
		return GameItem::HARDPOINT_ALTHAND;
	else if ( StrEqual( name, "head" ))
		return GameItem::HARDPOINT_HEAD;
	else if ( StrEqual( name, "shield" ))
		return GameItem::HARDPOINT_SHIELD;
	return 0;
}


void InventoryComponent::AddToInventory( const GameItem& item, bool equip )
{
	RenderComponent* rc = parentChit->GetRenderComponent();
	if ( hardpoints == -1 ) {
		hardpoints = 0;
		hardpointsInUse = 0;
		if ( rc ) {
			for( int i=0; i<EL_MAX_METADATA; ++i ) {
				const char* name = rc->GetMetaData(i);
				int h = HardpointNameToFlag( name );		// often 0; lots of metadata isn't a hardpoint
				hardpoints |= h;	
			}
		}
	}
	GLASSERT( hardpoints != -1 );
	if ( hardpoints == -1 ) return;

	int attachment = item.AttachmentFlags();
	bool equipped = false;
	
	if ( attachment == GameItem::INTRINSIC_AT_HARDPOINT ) {
		GLASSERT( item.HardpointFlags() & hardpoints );	// be sure the attachment point and hardpoint line up.
		equippedItems.Push( item );
		equipped = true;
	}
	else if ( attachment == GameItem::INTRINSIC_FREE ) {
		equippedItems.Push( item );
		equipped = true;
	}
	else if ( attachment == GameItem::HELD_AT_HARDPOINT ) {
		if ( equip ) {
			// check that the needed hardpoint is free.
			if (    (( hardpoints & item.HardpointFlags() ) !=0 )			// does the hardpoint exist?
				 && (( hardpointsInUse & item.HardpointFlags() ) == 0 ) )	// is the hardpoint available?
			{
				hardpointsInUse |= item.HardpointFlags();
				equippedItems.Push( item );
				equipped = true;

				GLASSERT( rc );
				if ( rc ) {
					const char* hardpoint = HardpointFlagToName( item.HardpointFlags() );
					GLASSERT( hardpoint );
					rc->Attach( hardpoint, item.ResourceName() );
				}
			}
		}
		if ( !equipped ) {
			packItems.Push( item );
		}
	}
	else if ( attachment == GameItem::HELD_FREE ) {
		if ( equip ) {
			equippedItems.Push( item );
			equipped = true;
		}
		else {
			packItems.Push( item );
		}
	}
}


GameItem* InventoryComponent::IsCarrying()
{
	// Do we have a held item on an "trigger" hardpoint?
	if ( hardpointsInUse & GameItem::HARDPOINT_TRIGGER ) {
		for( int i=0; i<equippedItems.Size(); ++i ) {
			if (    equippedItems[i].AttachmentFlags() == GameItem::HELD_AT_HARDPOINT
				 && equippedItems[i].HardpointFlags() == GameItem::HARDPOINT_TRIGGER ) 
			{
				return &equippedItems[i];
			}
		}
		GLASSERT( 0 );	// the hardpoint is in use; it should have been found.
	}
	return 0;
}


void InventoryComponent::GetWeapons( grinliz::CArray< GameItem*, EL_MAX_METADATA >* weapons )
{
	weapons->Clear();
	for( int i=0; i<equippedItems.Size(); ++i ) {
		GameItem* item = &equippedItems[i];
		if ( item->ToWeapon() ) {
			if ( item->AttachmentFlags() == GameItem::INTRINSIC_AT_HARDPOINT ) {
				// may  be overridden by a held item.
				if ( hardpointsInUse & item->HardpointFlags() ) {
					// yep; holding something else at this slot.
				}
				else {
					weapons->Push( item );
				}
			}
			else {
				weapons->Push( item );
			}
		}
	}
}
