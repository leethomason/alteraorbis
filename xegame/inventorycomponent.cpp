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
#include "../script/procedural.h"

using namespace grinliz;
using namespace tinyxml2;

void InventoryComponent::Load( const tinyxml2::XMLElement* element )
{
	this->BeginLoad( element, "InventoryComponent" );
	element->QueryIntAttribute( "hardpoints", &hardpoints );

	const XMLElement* parent = element->FirstChildElement( "intrinsicAt" );
	int i=0;
	for( const XMLElement* it = parent->FirstChildElement();
		 it;
		 it = it->NextSiblingElement(), ++i )
	{
		if ( it->FirstAttribute() ) {
			intrinsicAt[i] = new GameItem();
			intrinsicAt[i]->Load( it );
		}
	}

	parent = element->FirstChildElement( "heldAt" );
	i=0;
	for( const XMLElement* it = parent->FirstChildElement();
		 it;
		 it = it->NextSiblingElement(), ++i )
	{
		if ( it->FirstAttribute() ) {
			heldAt[i] = new GameItem();
			heldAt[i]->Load( it );
		}
	}

	parent = element->FirstChildElement( "freeItems" );
	i=0;
	for( const XMLElement* it = parent->FirstChildElement();
		 it;
		 it = it->NextSiblingElement(), ++i )
	{
		if ( it->FirstAttribute() ) {
			GameItem* gi = new GameItem();
			gi->Load( it );
			freeItems.Push( gi );
		}
	}

	parent = element->FirstChildElement( "packItems" );
	i=0;
	for( const XMLElement* it = parent->FirstChildElement();
		 it;
		 it = it->NextSiblingElement(), ++i )
	{
		if ( it->FirstAttribute() ) {
			GameItem* gi = new GameItem();
			gi->Load( it );
			packItems.Push( gi );
		}
	}

	this->EndLoad( element );
}


void InventoryComponent::Save( tinyxml2::XMLPrinter* printer )
{
	this->BeginSave( printer, "InventoryComponent" );
	printer->PushAttribute( "hardpoints", hardpoints );

	printer->OpenElement( "intrinsicAt" );
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( intrinsicAt[i] ) {
			intrinsicAt[i]->Save( printer );
		}
	}
	printer->CloseElement();

	printer->OpenElement( "heldAt" );
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( heldAt[i] ) {
			heldAt[i]->Save( printer );
		}
	}
	printer->CloseElement();

	printer->OpenElement( "freeItems" );
	for( int i=0; i<freeItems.Size(); ++i ) {
		freeItems[i]->Save( printer );
	}
	printer->CloseElement();

	printer->OpenElement( "packItems" );
	for( int i=0; i<packItems.Size(); ++i ) {
		packItems[i]->Save( printer );
	}
	printer->CloseElement();

	this->EndSave( printer );
}


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

IString InventoryComponent::HardpointFlagToName( int f )
{
	GLASSERT( f >= 0 && f < NUM_HARDPOINTS );
	if ( f >= 0 && f < NUM_HARDPOINTS ) {
		return StringPool::Intern( gHardpointNames[f], true );
	}
	return IString();
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
					IString n = HardpointFlagToName( item->hardpoint );
					GLASSERT( !n.empty() );
					rc->Attach( n, item->ResourceName() );

					if ( item->procedural & PROCEDURAL_INIT_MASK ) {
						ProcRenderInfo info;
						int result = ItemGen::RenderItem( Game::GetMainPalette(), *heldAt[item->hardpoint], &info );

						if ( result == ItemGen::PROC4 ) {
							rc->SetProcedural( n, info );
						}
					}
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


void InventoryComponent::AbsorbDamage( DamageDesc dd, DamageDesc* remain, const char* logstr )
{
	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( heldAt[i] ) {
			heldAt[i]->AbsorbDamage( true, dd, &dd, logstr );
		}
		if ( intrinsicAt[i] ) {
			intrinsicAt[i]->AbsorbDamage( true, dd, &dd, logstr );
		}
	}
	if ( remain ) {
		*remain = dd;
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
				IString name = HardpointFlagToName(i);
				rc->GetMetaData( name.c_str(), &xform );
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
	CArray<GameItem*, NUM_HARDPOINTS*2> work;
	RenderComponent* rc = parentChit->GetRenderComponent();

	for( int i=0; i<NUM_HARDPOINTS; ++i ) {
		if ( intrinsicAt[i] )
			work.Push( intrinsicAt[i] );
		if ( heldAt[i] )
			work.Push( heldAt[i] );
	}

	for( int i=0; i<work.Size(); ++i ) {
		GameItem* gi = work[i];
		bool tick = gi->DoTick(delta);
		callback = callback || tick;
		
		if ( rc && (gi->hardpoint != NO_HARDPOINT) && (gi->procedural & PROCEDURAL_TICK_MASK) ) {
			ProcRenderInfo info;
			int result = ItemGen::RenderItem( Game::GetMainPalette(), *work[i], &info );
			IString name = HardpointFlagToName( gi->hardpoint );

			if ( result == ItemGen::COLOR_XFORM ) {
				rc->SetColor( name, info.color[0].ToVector() );
			}
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

	return callback;
}


