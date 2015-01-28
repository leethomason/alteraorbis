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
#include "visitorweb.h"

class WorldMap;
class Wallet;
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

class BuildingWithPorchFilter : public IChitAccept
{
public:
	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MAP; }
};

// This one is abstract - has no Type()
class RelationshipFilter : public IChitAccept
{
public:
	RelationshipFilter() : team(-1), relationship(0) {}

	virtual bool Accept( Chit* chit );

	void CheckRelationship(Chit* compareTo, int status);
	void CheckRelationship(int team, int status);

private:
	int team;
	int relationship;
};

// Literally has the MOB key: Denizen, Lesser, Greater
class MOBKeyFilter : public RelationshipFilter
{
public:
	virtual bool Accept( Chit* chit );
	virtual int  Type() { return MOB; }
		
	grinliz::IString value;	// if null (the default) will accept any value for the "mob" key
};


// Generally intended: Denizen, Lesser, Greater, Worker, Visitor, etc. etc.
class MOBIshFilter : public RelationshipFilter
{
public:
	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MOB; }

private:
};


// Anything that can be damaged by an attack.
class BattleFilter : public IChitAccept
{
public:
	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MOB | MAP; }
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


class FruitElixirFilter : public IChitAccept
{
public:
	virtual bool Accept(Chit* chit);
	virtual int  Type() { return MOB; }
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
	virtual void Serialize( XStream* xs );
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

	void BuildingCounts(const grinliz::Vector2I& sector, int* counts, int n);

	enum {
		DEITY_MOTHER_CODE,	// master control
		DEITY_Q_CORE,		// logistics, infrastructure
		DEITY_R1K_CORE,		// adventures
		DEITY_TRUULGA,		// troll deity

		// in progress:
		DEITY_BEAST_CORE,	// beast-man core. name TBD
		DEITY_SHOG_SCRIFT,	// the great evil
		NUM_DEITY
	};
	Chit* GetDeity(int id);

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team );
	Chit* NewGoldChit( const grinliz::Vector3F& pos, Wallet* src );		// consumes the gold!
	Chit* NewCrystalChit( const grinliz::Vector3F& pos, Wallet* src, bool fuzzPos );	// consumes the 1st crystal!
	Chit* NewWorkerChit( const grinliz::Vector3F& pos, int team );
	Chit* NewBuilding( const grinliz::Vector2I& pos, const char* name, int team );
	Chit* NewLawnOrnament(const grinliz::Vector2I& pos, const char* name, int team);
	Chit* NewVisitor( int visitorIndex, const Web& web );
	Chit* NewDenizen( const grinliz::Vector2I& pos, int team );
	Chit* NewWildFruit(const grinliz::Vector2I& pos);

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
	void NewWalletChits( const grinliz::Vector3F& pos, Wallet* srcWallet );
	GameItem* AddItem( const char* name, Chit* chit, Engine* engine, int team, int level, const char* altResource=0, const char* altName=0 );
	GameItem* AddItem( GameItem* item, Chit* chit, Engine* engine, int team, int level );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, const ModelVoxel& mv );

	virtual int MapGridUse( int x, int y );

	int GetHomeTeam() const	{ return homeTeam; }
	// Don't set to NEUTRAL: it's important to know that we're in the
	// state where the home team is gone and hasn't been re-created.
	void SetHomeTeam(int t)	{ GLASSERT(t);  homeTeam = t; }

	CoreScript* GetHomeCore() const;
	grinliz::Vector2I GetHomeSector() const;
	Chit* GetAvatar() const;

	Census census;

	// Query the porch at the location.
	Chit* QueryPorch( const grinliz::Vector2I& pos, int* type );

	// Query the first building in the bounds.
	// If !name.empty, filters for that type.
	// If arr != null, returns all the matches.
	Chit* QueryBuilding( const grinliz::IString& name, const grinliz::Vector2I& pos, CChitArray* arr ) {
		grinliz::Rectangle2I r;
		r.Set( pos.x, pos.y, pos.x, pos.y );
		return QueryBuilding( name, r, arr );
	}
	Chit* QueryBuilding( const grinliz::IString& name, const grinliz::Rectangle2I& bounds, CChitArray* arr );

	Chit* QueryRemovable( const grinliz::Vector2I& pos );

	//LumosGame* GetLumosGame() { return lumosGame; }
	// Why the duplicate? This is for components to request
	// a new scene because of a player action. Both queues,
	// and doesn't allow multiple scenes to queue.
	void PushScene( int id, SceneData* data );
	bool PopScene( int* id, SceneData** data );
	bool IsScenePushed() const { return sceneID >= 0; }

	grinliz::IString NameGen(const char* dataset, int seed, int min, int max);

	// The seed is just a lookup into the name, in this form.
	// Generally use the NameGen() method.
	static grinliz::IString StaticNameGen(const gamedb::Reader* database, const char* dataset, int seed, int min, int max);

private:

	static bool HasMapSpatialInUse( Chit* );

	int							sceneID;
	SceneData*					sceneData;
	grinliz::Random				random;	// use the chit version, if possible, generally want to avoid high level random
	int							homeTeam;	
	Sim*						sim;	// if part of a simulation. can be null.
	int							deityID[NUM_DEITY];

	struct NamePoolID {
		grinliz::IString dataset;
		int id;

		void Serialize(XStream* xs);
	};

	grinliz::CDynArray<NamePoolID> namePool;
	grinliz::CDynArray<Chit*>	inUseArr;
	grinliz::CDynArray<Chit*>	chitList;
	grinliz::CDynArray<Chit*>	findMatch;
	grinliz::CDynArray<float>	findWeight;
	grinliz::CDynArray<Chit*>	chitArr;				// local, temporary

	MapSpatialComponent*	mapSpatialHash[NUM_SECTORS*NUM_SECTORS];
};


#endif // LUMOS_CHIT_BAG_INCLUDED
