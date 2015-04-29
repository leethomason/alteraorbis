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
#include "../grinliz/glgeometry.h"

#include "../tinyxml2/tinyxml2.h"

class Component;
class SpatialComponent;
class RenderComponent;
class MoveComponent;
class ItemComponent;
class AIComponent;
class HealthComponent;
class ChitContext;

class ChitBag;
class Chit;
class ChitEvent;
class GameItem;
class ComponentFactory;
class XStream;
class LumosChitBag;
class DamageDesc;
class ChitMsg;
class IScript;
class Wallet;

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

	const DamageDesc&	dd;
	int					originID;		// chitID of who caused damage
	bool				awardXP;
	bool				isMelee;
	bool				isExplosion;
	grinliz::Vector3F	originOfImpact;
};

// Synchronous.
class ChitMsg
{
public:
	enum {
		//                          Broadcast?
		CHIT_DESTROYED,			//	Yes		sender: health. Chit was destroyed / derezzed. Not shutdown.
		CHIT_DAMAGE,			//	Yes		sender: chitBag, ptr=&ChitDamageInfo
		CHIT_HEAL,				//					dataF = hitpoints
		CHIT_SECTOR_HERD,		//			AI message: a lead unit is telling other units to herd to a different sector.
								//					ptr = &SectorPort
		CHIT_TRACKING_ARRIVED,	//			A tracking component arrived at its target.
								//					ptr = Chit* that arrived
		CHIT_POS_CHANGE,

		PATHMOVE_DESTINATION_REACHED,
		PATHMOVE_DESTINATION_BLOCKED,
		PATHMOVE_TO_GRIDMOVE,	//			ptr=&SectorPort

		RENDER_IMPACT,			// impact metadata event has occured, sender: render component
		WORKQUEUE_UPDATE,
		CORE_TAKEOVER,			// Yes		vector location, in grid coordinates.
	};

	ChitMsg( int _id, int _data=0, const void* _ptr=0 ) {
		vector.Zero();
	}

	int ID() const 			{ return id; }
	int Data() const 		{ return data; }
	const void* Ptr() const { return ptr; }

	// useful data members:
	grinliz::Vector3F	vector;
	float				dataF 		= 0;
	int					originID 	= 0;
	grinliz::IString	str;

private:
	int id 				= 0;
	int data 			= 0;
	const void* ptr 	= nullptr;
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

	void Serialize(XStream* xs);

	int ID() const { return id; }
	void Add( Component*, bool loading=false );
	Component* Remove( Component* );

	void SetTickNeeded()		{ timeToTick = 0; }
	void DoTick();

	void OnChitEvent( const ChitEvent& event );

	SpatialComponent*	GetSpatialComponent()			{ return spatialComponent; }
	const SpatialComponent*	GetSpatialComponent() const	{ return spatialComponent; }

	MoveComponent*		GetMoveComponent()		{ return moveComponent; }
	ItemComponent*		GetItemComponent()		{ return itemComponent; }
	AIComponent*		GetAIComponent()		{ return aiComponent; }
	HealthComponent*	GetHealthComponent()	{ return healthComponent; }
	RenderComponent*	GetRenderComponent()	{ return renderComponent; }

	Component* GetComponent( int id );
	Component* GetComponent( const char* name );

	bool StackedMoveComponent() const;

	const ChitContext* Context() const;

	// Returns the item if this has the ItemComponent.
	GameItem* GetItem();
	int GetItemID();
	const GameItem* GetItem() const	{ return const_cast<const GameItem*>(const_cast<Chit*>(this)->GetItem()); }
	Wallet* GetWallet();
	bool PlayerControlled() const;	// more correctly: IsAvatar()

	void QueueDelete();
	void DeRez();	// deletes, but goes through de-rez sequence, checks for indestructible, etc.

	// Send a message to the listeners, and every component
	// in the chit (which don't need to be listeners.)
	// Synchronous
	void SendMessage( const ChitMsg& message, Component* exclude=0 );			// useful to not call ourselves. 

	void DebugStr( grinliz::GLString* str );

	int Team() const;

	// "private"
	// used by the spatial hash:
	Chit* next;

	// for components, so there is one random # generator per chit (not per component)
	grinliz::Random random;
	int timeToTick;		// time until next tick needed: set by DoTick() call
	int timeSince;		// time since the last tick

	const grinliz::Vector3F& Position() const			{ return position; }

	void SetPosition(const grinliz::Vector3F& value);
	void SetPosition(float x, float y, float z) { grinliz::Vector3F v = { x, y, z }; SetPosition(v); }

	const grinliz::Quaternion& Rotation() const			{ return rotation;  }
	void SetRotation(const grinliz::Quaternion& value);
	grinliz::Vector2F Heading2D() const;
	grinliz::Vector3F Heading() const;

	// Slightly more efficient - sends one update.
	void SetPosRot(const grinliz::Vector3F& pos, const grinliz::Quaternion& q);

private:
	ChitBag* chitBag;
	int		 id;
	bool	 playerControlled;
	grinliz::Vector3F	position;
	grinliz::Quaternion	rotation;

	//grinliz::CDynArray< IChitListener* > listeners; doesn't work; can't be serialized

public:
	enum {
		SPATIAL_BIT		= (1<<0),
		MOVE_BIT		= (1<<1),
		ITEM_BIT		= (1<<2),
		AI_BIT			= (1<<4),
		HEALTH_BIT		= (1<<5),
		RENDER_BIT		= (1<<6)
	};

private:
	enum { MOVE_SLOT = 1,
		   GENERAL_SLOT = 5,
		   NUM_SLOTS = 10, 
		   NUM_GENERAL = 4 };
	union {
		// Update ordering is tricky. Defined by the order of the slots;
		struct {
			SpatialComponent*	spatialComponent;
			MoveComponent*		moveComponent;
			ItemComponent*		itemComponent;
			AIComponent*		aiComponent;
			HealthComponent*	healthComponent;
			Component*			general[NUM_GENERAL];	// tried to make this a linked list - hard to win, in terms of complexity and memory
			RenderComponent*	renderComponent;		// should be last
		};
		Component*			slot[NUM_SLOTS];
	};
};


#endif // XENOENGINE_CHIT_INCLUDED
