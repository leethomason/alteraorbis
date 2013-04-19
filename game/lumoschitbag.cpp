#include "lumoschitbag.h"
#include "gameitem.h"
#include "gamelimits.h"
#include "census.h"
#include "pathmovecomponent.h"
#include "aicomponent.h"
#include "debugstatecomponent.h"
#include "healthcomponent.h"
#include "mapspatialcomponent.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"

#include "../script/battlemechanics.h"
#include "../script/itemscript.h"


//#define DEBUG_EXPLOSION

using namespace grinliz;

LumosChitBag::LumosChitBag() : engine( 0 )
{
}


LumosChitBag::~LumosChitBag()
{
	// Call the parent function so that chits aren't 
	// aren't using deleted memory.
	DeleteChits();
}



Chit* LumosChitBag::NewMonsterChit( const Vector3F& pos, const char* name, int team )
{
	Chit* chit = NewChit();

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( engine, name ));
	chit->Add( new PathMoveComponent( worldMap ));
	chit->Add( new AIComponent( engine, worldMap ));

	chit->GetSpatialComponent()->SetPosition( pos );

	AddItem( name, chit, engine, team, 0 );

	chit->Add( new HealthComponent( engine ));
	return chit;
}


void LumosChitBag::HandleBolt( const Bolt& bolt, Model* modelHit, const grinliz::Vector3F& at )
{
	GLASSERT( engine );
	Chit* chitShooter = GetChit( bolt.chitID );	// may be null
	int shooterTeam = -1;
	if ( chitShooter && chitShooter->GetItemComponent() ) {
		shooterTeam = chitShooter->GetItemComponent()->GetItem()->primaryTeam;
	}
	int explosive = bolt.effect & GameItem::EFFECT_EXPLOSIVE;
 
	if ( !explosive ) {
		if ( modelHit ) {
			Chit* chitHit = modelHit->userData;
			if ( chitHit ) {
				GLASSERT( GetChit( chitHit->ID() ) == chitHit );
				if ( chitHit->GetItemComponent() &&
					 chitHit->GetItemComponent()->GetItem()->primaryTeam == shooterTeam ) 
				{
					// do nothing. don't shoot own team.
				}
				else {
					DamageDesc dd( bolt.damage, bolt.effect );
		
					ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &dd );
					// 'vector' copied from BattleMechanics::GenerateExplosionMsgs
					// This code is strange: the vector doesn't contain the origin,
					// but the impulse (velocity). Weird.
					msg.vector = bolt.dir;
					msg.vector.Normalize();
					msg.vector.Multiply( 2.0f );
					msg.originID = bolt.chitID;
					chitHit->SendMessage( msg, 0 );
				}
			}
		}
	}
	else {
		// Here don't worry about the chit hit. Just ray cast to see
		// who is in range of the explosion and takes damage.
		
		// Back up the origin of the bolt just a bit, so it doesn't keep
		// intersecting the model it hit. Then do ray checks around to 
		// see what gets hit by the explosion.

		//GLLOG(( "Explosion: " ));

		float rewind = Min( 0.1f, 0.5f*bolt.len );
		GLASSERT( Equal( bolt.dir.Length(), 1.f, 0.001f ));
		Vector3F origin = at - bolt.dir * rewind;

		DamageDesc dd( bolt.damage, bolt.effect );
		BattleMechanics::GenerateExplosionMsgs( dd, origin, bolt.chitID, engine, &chitList );
	}
}


void LumosChitBag::AddItem( const char* name, Chit* chit, Engine* engine, int team, int level )
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get( name, &itemDefArr );
	GLASSERT( itemDefArr.Size() > 0 );

	GameItem item = *(itemDefArr[0]);
	item.primaryTeam = team;
	item.stats.SetExpFromLevel( level );
	item.InitState();

	if ( !chit->GetItemComponent() ) {
		ItemComponent* ic = new ItemComponent( engine, item );
		chit->Add( ic );
		for( int i=1; i<itemDefArr.Size(); ++i ) {
			ic->AddToInventory( new GameItem( *(itemDefArr[i]) ), true );
		}
	}
	else {
		GLASSERT( itemDefArr.Size() == 1 );
		chit->GetItemComponent()->AddToInventory( new GameItem( item ), true );
	}
}


bool LumosChitBag::HasMapSpatialInUse( Chit* chit ) {
	MapSpatialComponent* ms = GET_COMPONENT( chit, MapSpatialComponent );
	if ( ms ) {
		return true;
	}
	return false;
}


int LumosChitBag::MapGridUse( int x, int y )
{
	Rectangle2F r;
	r.min.x = (float)x;
	r.min.y = (float)y;
	r.max.x = r.min.x + 1.0f;
	r.max.y = r.min.y + 1.0f;

	QuerySpatialHash( &inUseArr, r, 0, HasMapSpatialInUse );
	int flags = 0;
	for( int i=0; i<inUseArr.Size(); ++i ) {
		MapSpatialComponent* ms = GET_COMPONENT( inUseArr[i], MapSpatialComponent );
		GLASSERT( ms );
		flags |= ms->Mode();
	}
	return flags;
}

