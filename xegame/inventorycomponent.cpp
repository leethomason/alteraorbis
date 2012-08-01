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
			return &attached[i];
		}
	}
	return 0;
}
