#include "inventorycomponent.h"
#include "spatialcomponent.h"
#include "rendercomponent.h"
#include "chit.h"

using namespace grinliz;


void InventoryComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[Inventory] " );
}


void InventoryComponent::AddToInventory( Chit* itemChit )
{
	inventory.Add( itemChit );
	itemChit->AddListener( this );

	// Need a relative spatial component. If there is a render
	// component, that will get picked up. If no render, no 
	// worries, it just won't render.
	if ( itemChit->GetSpatialComponent() ) {
		itemChit->Remove( itemChit->GetSpatialComponent() );
	}
	GLASSERT( !itemChit->GetSpatialComponent() );

	Vector3F trigger = { 0, 0, 0 };
	if ( parentChit->GetRenderComponent() ) {
		parentChit->GetRenderComponent()->GetMetaData( "trigger", &trigger );
	}

	RelativeSpatialComponent* rsc = new RelativeSpatialComponent( false );
	itemChit->Add( rsc );
	parentChit->AddListener( rsc );
	rsc->SetRelativePosYRot( trigger, 0 );

	// Sleazy trick: the relative spatial updates as a consequence of the
	// spatial. But that's hard to remember, to reset the spatial.
	if ( parentChit->GetSpatialComponent() ) {
		Vector3F pos = parentChit->GetSpatialComponent()->GetPosition();
		Vector3F posPrime = pos;
		posPrime.y += 0.01f;
		parentChit->GetSpatialComponent()->SetPosition( posPrime );
		parentChit->GetSpatialComponent()->SetPosition( pos );
	}
}


void InventoryComponent::RemoveFromInventory( Chit* itemChit )
{
	itemChit = inventory.Remove( itemChit );
	if ( itemChit >= 0 ) {
		itemChit->RemoveListener( this );
		itemChit->Remove( itemChit->GetSpatialComponent() );
	}
}


/*
frought with deletion order peril
void InventoryComponent::OnChitMsg( Chit* chit, int id )
{
	if ( id == CHIT_MSG_DELETING ) {

		int index = inventory.Find( chit );
		if ( index >= 0 ) {
			inventory.SwapRemove( index );
		}
	}
}
*/
