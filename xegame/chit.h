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

#ifndef XENOENGINE_CHIT_INCLUDED
#define XENOENGINE_CHIT_INCLUDED


#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"

#include "../tinyxml2/tinyxml2.h"

class Component;
class SpatialComponent;
class RenderComponent;
class MoveComponent;
class ItemComponent;
class ScriptComponent;
class AIComponent;
class HealthComponent;

class ChitBag;
class Chit;
class ChitEvent;
class GameItem;
class ComponentFactory;
class XStream;
class LumosChitBag;
class DamageDesc;
class ChitMsg;

// Allows registering to listen to chit messages.
// This is not serialized. Calls are synchronous. (Be careful.)
class IChitListener
{
public:
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg ) = 0;
};

struct ChitDamageInfo
{
	ChitDamageInfo( const DamageDesc& _dd ) : dd(_dd), originID(0), awardXP(false), isMelee(true), isExplosion(false) {
		originOfImpact.Zero();
	}

	int					originID;		// chitID of who caused damage
	bool				awardXP;
	bool				isMelee;
	bool				isExplosion;
	const DamageDesc&	dd;
	grinliz::Vector3F	originOfImpact;
};

// Synchronous.
class ChitMsg
{
public:
	enum {
		// ---- Chit --- //
		CHIT_DESTROYED_START,	// sender: health. *starts* the countdown sequence.
		CHIT_DESTROYED_TICK,	// dataF = fraction
		CHIT_DESTROYED_END,		// sender: health. ends the sequence, next step is the chit delete
		CHIT_DAMAGE,			// sender: chitBag, ptr=&ChitDamageInfo
		CHIT_HEAL,				//					dataF = hitpoints
		CHIT_SECTOR_HERD,		// AI message: a lead unit is telling other units to herd to a different sector.
								//					ptr = &SectorPort
		CHIT_TRACKING_ARRIVED,	// A tracking component arrived at its target.
								//					ptr = Chit* that arrived
		// ---- Component ---- //
		PATHMOVE_DESTINATION_REACHED,
		PATHMOVE_DESTINATION_BLOCKED,

		HEALTH_CHANGED,
		
		// XE
		SPATIAL_CHANGED,		// the position or rotation has changed, sender: spatial
		RENDER_IMPACT,			// impact metadata event has occured, sender: render component
		WORKQUEUE_UPDATE,
	};

	ChitMsg( int _id, int _data=0, const void* _ptr=0 ) : id(_id), data(_data), ptr(_ptr), dataF(0), originID(0) {
		vector.Zero();
	}

	int ID() const { return id; }
	int Data() const { return data; }
	const void* Ptr() const { return ptr; }

	// useful data members:
	grinliz::Vector3F	vector;
	float				dataF;
	int					originID;
	grinliz::IString	str;

private:
	int id;
	int data;
	const void* ptr;
};


// Gets a component that is a direct child of the general component.
#define GET_GENERAL_COMPONENT( chit, name ) static_cast<name*>( chit->GetComponent( #name ) )
#define GET_SUB_COMPONENT( chit, top, sub ) ( chit->Get##top() ? chit->Get##top()->To##sub() : 0 )

/*	General purpose GameObject.
	A class to hold Components.
	Actually a POD. Do not subclass.
*/
class Chit
{
	friend class ChitBag;	// used for the OUTER_TICK optimization
public:
	// Should always use Create from ChitBag
	Chit( int id=0, ChitBag* chitBag=0 );
	// Should always use Delete from ChitBag
	~Chit();

	void Init( int id, ChitBag* chitBag );
	void Free();

	void Serialize( const ComponentFactory* factory, XStream* xs );

	int ID() const { return id; }
	void Add( Component* );
	void Remove( Component* );

	void SetTickNeeded()		{ timeToTick = 0; }
	void DoTick( U32 delta );

	void OnChitEvent( const ChitEvent& event );
	void Swap( Component* removeAndDelete, Component* addThis )	{ GLASSERT( swapOut == 0 ); GLASSERT( swapIn == 0 ); swapOut = removeAndDelete; swapIn = addThis; }

	SpatialComponent*	GetSpatialComponent()	{ return spatialComponent; }
	const SpatialComponent*	GetSpatialComponent() const	{ return spatialComponent; }

	MoveComponent*		GetMoveComponent()		{ return moveComponent; }
	ItemComponent*		GetItemComponent()		{ return itemComponent; }
	ScriptComponent*	GetScriptComponent()	{ return scriptComponent; }
	AIComponent*		GetAIComponent()		{ return aiComponent; }
	HealthComponent*	GetHealthComponent()	{ return healthComponent; }
	RenderComponent*	GetRenderComponent()	{ return renderComponent; }

	Component* GetComponent( int id );
	Component* GetComponent( const char* name );

	ChitBag* GetChitBag()						{ return chitBag; }
	virtual LumosChitBag* GetLumosChitBag();

	// Returns the item if this has the ItemComponent.
	GameItem* GetItem();
	const GameItem* GetItem() const	{ return const_cast<const GameItem*>(const_cast<Chit*>(this)->GetItem()); }
	void QueueDelete();

	// Send a message to the listeners, and every component
	// in the chit (which don't need to be listeners.)
	// Synchronous
	void SendMessage(	const ChitMsg& message, Component* exclude=0 );			// useful to not call ourselves. 

	void AddListener( IChitListener* handler ) {
		GLASSERT( listeners.Find( handler ) < 0 );
		listeners.Push( handler );
	}
	void RemoveListener( IChitListener* handler ) {
		int i = listeners.Find( handler );
		GLASSERT( i >= 0 );
		if ( i >= 0 ) listeners.SwapRemove( i );
	}
	
	void DebugStr( grinliz::GLString* str );

	void SetPlayerControlled( bool player ) { playerControlled = player; }
	bool PlayerControlled() const			{ return playerControlled; }
	int PrimaryTeam() const;

	// "private"
	// used by the spatial hash:
	Chit* next;

	// for components, so there is one random # generator per chit (not per component)
	grinliz::Random random;
	int timeToTick;		// time until next tick needed: set by DoTick() call
	int timeSince;		// time since the last tick

private:
	ChitBag* chitBag;
	int		 id;
	bool	 playerControlled;
	grinliz::CDynArray< IChitListener* > listeners;

public:
	enum {
		SPATIAL_BIT		= (1<<0),
		MOVE_BIT		= (1<<1),
		ITEM_BIT		= (1<<2),
		SCRIPT_BIT		= (1<<3),
		AI_BIT			= (1<<4),
		HEALTH_BIT		= (1<<5),
		RENDER_BIT		= (1<<6)
	};

private:
	enum { NUM_SLOTS = 9 };
	union {
		// Update ordering is tricky. Defined by the order of the slots;
		struct {
			SpatialComponent*	spatialComponent;
			MoveComponent*		moveComponent;
			ItemComponent*		itemComponent;
			ScriptComponent*	scriptComponent;
			AIComponent*		aiComponent;
			HealthComponent*	healthComponent;
			Component*			general0;
			Component*			general1;
			RenderComponent*	renderComponent;		// should be last
		};
		Component*			slot[NUM_SLOTS];
	};

	static Component* swapOut;
	static Component* swapIn;
};


struct ComponentSet
{
	enum {
		IS_ALIVE		= (1<<30),
		NOT_IN_IMPACT	= (1<<29),
	};

	ComponentSet( Chit* chit, int bits );
	void Zero();

	bool	okay;
	Chit*	chit;
	bool	alive;

	SpatialComponent*	spatial;
	MoveComponent*		move;
	ItemComponent*		itemComponent;
	GameItem*			item;
	RenderComponent*	render;
	AIComponent*		ai;
};


#endif // XENOENGINE_CHIT_INCLUDED
