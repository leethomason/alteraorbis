#include "inventorycomponent.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "chit.h"

using namespace grinliz;


void InventoryComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[Inventory] " );
}


void InventoryComponent::AddToInventory( const GameItem& item )
{
	if ( item.flags & GameItem::APPENDAGE ) {
		hardpoints.Push( item );
	}
	else if ( item.flags & GameItem::HELD ) {
		// FIXME: attach to the correct hardpoint, not just any.
		GLASSERT( attached.Size() < hardpoints.Size() );
		attached.Push( item );
		GLASSERT( !item.resource.empty() );
		GLASSERT( !item.hardpoint.empty() );
		
		RenderComponent* render = parentChit->GetRenderComponent();
		GLASSERT( render );
		if ( render ) {
			render->Attach( item.Hardpoint(), item.ResourceName() );
		}
	}
	else {
		pack.Push( item );
	}
}


GameItem* InventoryComponent::IsCarrying()
{
	// Do we have a held item on an "trigger" hardpoint?
	for( int i=0; i<attached.Size(); ++i ) {
		if ( StrEqual( attached[i].Hardpoint(), "trigger" )) {
			GLASSERT( i==0 );	// trigger must be the first hardpoint!
			return &attached[i];
		}
	}
	return 0;
}


void InventoryComponent::GetWeapons( grinliz::CArray< GameItem*, EL_MAX_METADATA >* weapons )
{
	weapons->Clear();
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( hardpoints.Size() > i ) {
			if ( hardpoints[i].ToWeapon() ) {
				weapons->Push( &hardpoints[i] );
			}
		}
		else if ( attached.Size() > i ) {
			if ( attached[i].ToWeapon() ) {
				weapons->Push( &attached[i] );
			}
		}
	}
}
