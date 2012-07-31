#include "gameitem.h"
#include "../xegame/chit.h"
#include "../xegame/inventorycomponent.h"
#include "../xegame/itemcomponent.h"

#if 0
void GameItem::GetActiveItems( Chit* chit, grinliz::CArray<XEItem*, MAX_ACTIVE_ITEMS>* array )
{
	// Chits can contain Item components and Inventory components. 
	InventoryComponent *ic = GET_COMPONENT( chit, InventoryComponent );
	GLASSERT( ic );
	if ( ic ) {
		const SafeChitList& list = ic->GetInventory();
		for( Chit* chitItem = list.First(); chitItem; chitItem = list.Next() ) {
			ItemComponent* itemComp = GET_COMPONENT( chitItem, ItemComponent );
			// FIXME: should itemComponent and Item be different??
			// Sure doesn't seem like it.
			if ( itemComp && itemComp->GetItem() ) {
				array->Push( itemComp->GetItem() );
			}
		}
	}
}
#endif