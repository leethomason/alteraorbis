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

#ifndef LUMOS_CHIT_BAG_INCLUDED
#define LUMOS_CHIT_BAG_INCLUDED

#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"
#include "../engine/map.h"
#include "census.h"
#include "visitor.h"

class WorldMap;
struct Wallet;
class CoreScript;
class LumosGame;
class MapSpatialComponent;
class SceneData;
class Sim;

class BuildingFilter : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP; }
};


class BuildingRepairFilter : public IChitAccept
{
public:
	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MAP; }
};


// Literally has the MOB key: Denizen, Lesser, Greater
class MOBKeyFilter : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MOB; }
};


// Generally intended: Denizen, Lesser, Greater, Worker, Visitor, etc. etc.
class MOBIshFilter : public IChitAccept
{
public:
	MOBIshFilter() : relateTo(0), relationship(0) {}
	// If set, will check for a relationship.

	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MOB; }

	void CheckRelationship(Chit* compareTo, int status) {
		relateTo = compareTo;
		relationship = status;
	}

private:
	bool RelateAccept( Chit* chit) const;

	Chit* relateTo;
	int relationship;
};


// Anything that can be damaged by an attack.
class BattleFilter : public IChitAccept
{
public:
	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MOB | MAP; }
};


class PlantFilter : public IChitAccept
{
public:
	PlantFilter( int type=-1, int minStage=0, int maxStage=3 );
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP; }

private:
	int typeFilter;
	int minStage;
	int maxStage;
};


class RemovableFilter : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP; }
};


class ChitHasMapSpatial : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP; }
};

class ItemNameFilter : public IChitAccept
{
public:
	ItemNameFilter( const grinliz::IString& name, int type );
	ItemNameFilter( const grinliz::IString* names, int n, int type );

	virtual bool Accept( Chit* chit );
	virtual int  Type()		{ return type; } 

protected:
	ItemNameFilter();
	const grinliz::IString* names;
	int						count;
	int						type;
};

class ItemFlagFilter : public IChitAccept
{
public:
	ItemFlagFilter( int _required, int _excluded ) : required(_required), excluded(_excluded) {}

	virtual bool Accept( Chit* chit );
	virtual int Type() { return MAP | MOB; }
	
private:
	int excluded, required;
};


class GoldCrystalFilter : public ItemNameFilter
{
public:
	GoldCrystalFilter();
private:
	grinliz::IString arr[5];
};


// "loot" being items that can be dropped on the ground and picked up.
class LootFilter : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MOB; }
};


class GoldLootFilter : public IChitAccept
{
private:
	GoldCrystalFilter	gold;
	LootFilter			loot;
public:
	virtual bool Accept( Chit* chit ) { return gold.Accept( chit ) || loot.Accept( chit ); }
	virtual int  Type() { return gold.Type() | loot.Type(); }
};


class LumosChitBag : public ChitBag,
					 public IMapGridUse
{
private:
	typedef ChitBag super;

public:
	LumosChitBag( const ChitContext&, Sim* );
	virtual ~LumosChitBag();

	virtual LumosChitBag* ToLumos() { return this; }
	virtual void Serialize( const ComponentFactory* factory, XStream* xs );
	// Can be null - not required
	Sim* GetSim() const { return sim; }

	// Buildings can't move - no update.
	void AddToBuildingHash( MapSpatialComponent* chit, int x, int y );
	void RemoveFromBuildingHash( MapSpatialComponent* chit, int x, int y );

	enum {
		NEAREST,
		RANDOM_NEAR
	};

	Chit* FindBuilding( const grinliz::IString& name,		// particular building, or emtpy to match all
						const grinliz::Vector2I& sector,	// sector to query
						const grinliz::Vector2F* pos,		// optional IN: used for evaluating NEAREST, etc.
						int flags,
						grinliz::CDynArray<Chit*>* arr,		// optional; all the matches
						IChitAccept* filter );				// optional; run this filter first

	// Same as above; takes a CChitArray
	Chit* FindBuildingCC(const grinliz::IString& name,		// particular building, or emtpy to match all
						const grinliz::Vector2I& sector,	// sector to query
						const grinliz::Vector2F* pos,		// optional IN: used for evaluating NEAREST, etc.
						int flags,
						CChitArray* arr,					// optional; the matches that fit
						IChitAccept* filter);				// optional; run this filter first

	enum {
		DEITY_MOTHER_CODE,	// master control
		DEITY_Q_CORE,		// logistics, infrastructure
		DEITY_R1K_CORE,		// adventures

		// in progress:
		DEITY_BEAST_CORE,	// beast-man core. name TBD
		DEITY_SHOG_SCRIFT,	// the great evil
		NUM_DEITY
	};
	Chit* GetDeity(int id);

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team );
	Chit* NewGoldChit( const grinliz::Vector3F& pos, int amount );
	Chit* NewCrystalChit( const grinliz::Vector3F& pos, int crystal, bool fuzzPos );
	Chit* NewWorkerChit( const grinliz::Vector3F& pos, int team );
	Chit* NewBuilding( const grinliz::Vector2I& pos, const char* name, int team );
	Chit* NewLawnOrnament(const grinliz::Vector2I& pos, const char* name, int team);
	Chit* NewVisitor( int visitorIndex );
	Chit* NewDenizen( const grinliz::Vector2I& pos, int team );

	// Creates "stuff in the world". The GameItem is passed by ownership.
	Chit* NewItemChit(  const grinliz::Vector3F& pos, 
						GameItem* orphanItem, 
						bool fuzzPos, 
						bool placeOnGround, 
						int selfDestructTimer );

	Bolt* NewBolt(	const grinliz::Vector3F& pos,
					grinliz::Vector3F dir,
					int effectFlags,
					int chitID,
					float damage,
					float speed,
					bool trail );
	// Creates enough chits to empty the wallet.
	void NewWalletChits( const grinliz::Vector3F& pos, const Wallet& wallet );
	GameItem* AddItem( const char* name, Chit* chit, Engine* engine, int team, int level, const char* altResource=0 );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, const ModelVoxel& mv );

	virtual int MapGridUse( int x, int y );

	// Get the core for this sector.
	CoreScript* GetHomeCore();
	
	grinliz::Vector2I GetHomeSector() const {
		return homeSector;
	}

	void SetHomeSector( const grinliz::Vector2I& home ) {
		homeSector = home;
	}

	Census census;

	// Query the porch at the location.
	Chit* QueryPorch( const grinliz::Vector2I& pos, int* type );
	// Query the first building in the bounds.
	Chit* QueryBuilding( const grinliz::Vector2I& pos ) {
		grinliz::Rectangle2I r;
		r.Set( pos.x, pos.y, pos.x, pos.y );
		return QueryBuilding( r );
	}
	Chit* QueryBuilding( const grinliz::Rectangle2I& bounds );
	Chit* QueryRemovable( const grinliz::Vector2I& pos, bool ignorePlants );
	Chit* QueryPlant( const grinliz::Vector2I& pos, int* type, int* stage );

	//LumosGame* GetLumosGame() { return lumosGame; }
	// Why the duplicate? This is for components to request
	// a new scene because of a player action. Both queues,
	// and doesn't allow multiple scenes to queue.
	void PushScene( int id, SceneData* data );
	bool PopScene( int* id, SceneData** data );
	bool IsScenePushed() const { return sceneID >= 0; }

	enum {
		SUMMON_TECH
	};
	void AddSummoning(const grinliz::Vector2I& sector, int reason);
	grinliz::Vector2I PopSummoning(int reason);

private:

	static bool HasMapSpatialInUse( Chit* );

	int							sceneID;
	SceneData*					sceneData;
	grinliz::Random				random;	// use the chit version, if possible, generally want to avoid high level random
	grinliz::Vector2I			homeSector;
	Sim*						sim;	// if part of a simulation. can be null.
	int							deityID[NUM_DEITY];

	grinliz::CDynArray<Chit*>	inUseArr;
	grinliz::CDynArray<Chit*>	chitList;
	grinliz::CDynArray<Chit*>	findMatch;
	grinliz::CDynArray<float>	findWeight;
	grinliz::CDynArray<Chit*>	chitArr;				// local, temporary
	grinliz::CDynArray<grinliz::Vector2I> summoningArr;	// not serialized (maybe should be?)

	MapSpatialComponent*	mapSpatialHash[NUM_SECTORS*NUM_SECTORS];
};


#endif // LUMOS_CHIT_BAG_INCLUDED
