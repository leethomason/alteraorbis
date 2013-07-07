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

class LumosChitBag : public ChitBag,
					 public IMapGridUse
{
public:
	LumosChitBag();
	virtual ~LumosChitBag();

	virtual LumosChitBag* ToLumos() { return this; }

	void SetContext( Engine* e, WorldMap* wm ) { engine = e; worldMap = wm; }

	Chit* NewMonsterChit( const grinliz::Vector3F& pos, const char* name, int team );
	Chit* NewGoldChit( const grinliz::Vector3F& pos, int amount );
	Chit* NewCrystalChit( const grinliz::Vector3F& pos, int crystal, bool fuzzPos );
	Chit* NewWorkerChit( const grinliz::Vector3F& pos, int team );
	Chit* NewBuilding( const grinliz::Vector2I& pos, const char* name, int team );
	Chit* NewVisitor( int visitorIndex );

	// Creates enough chits to empty the wallet.
	void NewWalletChits( const grinliz::Vector3F& pos, const Wallet& wallet );
	void AddItem( const char* name, Chit* chit, Engine* engine, int team, int level );

	// IBoltImpactHandler
	virtual void HandleBolt( const Bolt& bolt, const ModelVoxel& mv );

	virtual int MapGridUse( int x, int y );
	CoreScript* IsBoundToCore( Chit* );
	// Get the core for this sector.
	CoreScript* GetCore( const grinliz::Vector2I& sector );

	Census census;

	static bool GoldFilter( Chit* chit );			// Is this gold?
	static bool GoldCrystalFilter( Chit* chit );	// Is this gold or crystal? (wallet items)
	static bool CoreFilter( Chit* chit );
	static bool WorkerFilter( Chit* chit );
	static bool BuildingFilter( Chit* chit );
	static bool BuildingWithPorchFilter( Chit* chit );
	static bool RemovableFilter( Chit* chit );			// something that can be removed - building, plant
	static bool KioskFilter( Chit* chit );

	Chit* QueryBuilding( const grinliz::Vector2I& pos, bool orPorch );

private:

	static bool HasMapSpatialInUse( Chit* );

	grinliz::CDynArray<Chit*> inUseArr;
	grinliz::CDynArray<Chit*> chitList;
	Engine* engine;
	WorldMap* worldMap;
	grinliz::Random random;	// use the chit version, if possible, generally want to avoid high level random
};


#endif // LUMOS_CHIT_BAG_INCLUDED
