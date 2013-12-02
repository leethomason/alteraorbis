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

class BuildingFilter : public IChitAccept
{
public:
	BuildingFilter( bool needSupplyOnly=false );

	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP; }

private:
	bool needSupplyOnly;
};


/* Hard to define...Not Buildings,
   but something that can be a friend or enemy.
   MoB is used in the genereric sense, doesn't
   actually need to be mobile.
   There are probably some non-exclusive cases,
   so there is no reason to assume a Building
   is not a MoB and vice versio.
*/
class MoBFilter : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MOB; }
};


class PlantFilter : public IChitAccept
{
public:
	PlantFilter( int type=-1, int maxStage=3 );
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP; }

private:
	int typeFilter;
	int maxStage;
};


class RemovableFilter : public IChitAccept
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return buildingFilter.Type() | plantFilter.Type(); }	

private:
	BuildingFilter buildingFilter;
	PlantFilter    plantFilter;
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
	ItemNameFilter( const char* names[], int n );
	ItemNameFilter( const grinliz::IString& name );
	ItemNameFilter( const grinliz::IString* names, int n );

	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MAP | MOB; }

protected:
	ItemNameFilter();
	const char**			cNames;
	const grinliz::IString* iNames;
	int						count;
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
	LumosChitBag();
	virtual ~LumosChitBag();

	virtual LumosChitBag* ToLumos() { return this; }
	// initialization
	void SetContext( Engine* e, WorldMap* wm, LumosGame* lg ) { engine = e; worldMap = wm; lumosGame = lg; }

	// Buildings can't move - no update.
	void AddToBuildingHash( MapSpatialComponent* chit, int x, int y );
	void RemoveFromBuildingHash( MapSpatialComponent* chit, int x, int y );

	enum {
		NEAREST,
		RANDOM_NEAR
	};
	Chit* FindBuilding( const grinliz::IString& name,		// particular building, or emtpy to match all
						const grinliz::Vector2I& sector,	// sector to query
						const grinliz::Vector2F* pos,		// used for evaluating NEAREST, etc.
						int flags,
						CChitArray* arr,					// optional; the first N hits
						IChitAccept* filter );				// optional; run this filter first

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team );
	Chit* NewGoldChit( const grinliz::Vector3F& pos, int amount );
	Chit* NewCrystalChit( const grinliz::Vector3F& pos, int crystal, bool fuzzPos );
	Chit* NewWorkerChit( const grinliz::Vector3F& pos, int team );
	Chit* NewBuilding( const grinliz::Vector2I& pos, const char* name, int team );
	Chit* NewVisitor( int visitorIndex );
	Chit* NewDenizen( const grinliz::Vector2I& pos, int team );

	// Creates "stuff in the world". The GameItem is passed by ownership.
	Chit* NewItemChit( const grinliz::Vector3F& pos, GameItem* orphanItem, bool fuzzPos, bool placeOnGround );


	Bolt* NewBolt(	const grinliz::Vector3F& pos,
					grinliz::Vector3F dir,
					int effectFlags,
					int chitID,
					float damage,
					float speed,
					bool trail );
	// Creates enough chits to empty the wallet.
	void NewWalletChits( const grinliz::Vector3F& pos, const Wallet& wallet );
	void AddItem( const char* name, Chit* chit, Engine* engine, int team, int level );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, const ModelVoxel& mv );

	virtual int MapGridUse( int x, int y );

	// Get the core for this sector.
	CoreScript* GetCore( const grinliz::Vector2I& sector );
	CoreScript* GetHomeCore()	{ return GetCore( PlayerHomeSector() ); }
	
	// FIXME placeholder code
	grinliz::Vector2I PlayerHomeSector() const {
		grinliz::Vector2I v = { 5, 5 };
		return v;
	}

	Census census;

	// Query the porch at the location.
	Chit* QueryPorch( const grinliz::Vector2I& pos );
	// Query the first building in the bounds.
	Chit* QueryBuilding( const grinliz::Vector2I& pos ) {
		grinliz::Rectangle2I r;
		r.Set( pos.x, pos.y, pos.x, pos.y );
		return QueryBuilding( r );
	}
	Chit* QueryBuilding( const grinliz::Rectangle2I& bounds );
	Chit* QueryRemovable( const grinliz::Vector2I& pos );

	LumosGame* GetLumosGame() { return lumosGame; }
	// Why the duplicate? This is for components to request
	// a new scene because of a player action. Both queues,
	// and doesn't allow multiple scenes to queue.
	void PushScene( int id, SceneData* data );
	bool PopScene( int* id, SceneData** data );
	bool IsScenePushed() const { return sceneID >= 0; }

	// Used by the CoreScript.
	// A table that maps MOB chit ids -> core chit IDs
	// This is generated by the CoreScripts, and doesn't need
	// to be serialized.
	grinliz::HashTable<int, int>	chitToCoreTable;

private:

	static bool HasMapSpatialInUse( Chit* );

	Engine* engine;
	WorldMap* worldMap;
	LumosGame* lumosGame;
	int sceneID;
	SceneData* sceneData;
	grinliz::Random random;	// use the chit version, if possible, generally want to avoid high level random

	grinliz::CDynArray<Chit*>	inUseArr;
	grinliz::CDynArray<Chit*>	chitList;

	MapSpatialComponent*	mapSpatialHash[NUM_SECTORS*NUM_SECTORS];
	// Cores can't be destroyed, so we can cache them.
	Chit*					coreCache[NUM_SECTORS*NUM_SECTORS];
};


#endif // LUMOS_CHIT_BAG_INCLUDED
