#ifndef FORGE_SCRIPT_INCLUDED
#define FORGE_SCRIPT_INCLUDED

#include "../grinliz/glrandom.h"

class ItemComponent;
class Wallet;
class GameItem;
class TransactAmt;

class ForgeScript
{
public:
	// seed should be stable: GetItem()->ID() ^ forgeUser->GetItem()->Traits().Experience()
	ForgeScript( int seed, int userLevel, int techLevel );

	enum {
		RING,
		GUN,
		SHIELD,
		NUM_ITEM_TYPES
	};

	enum {
		PISTOL,
		BLASTER,
		PULSE,
		BEAMGUN,
		NUM_GUN_TYPES,
		NUM_TECH0_GUNS = 2
	};

	enum {
		NUM_RING_PARTS = 4,
		NUM_GUN_PARTS = 4
	};

	// gun parts and ring parts are in WeaponGen
	enum {
		EFFECT_FIRE,
		EFFECT_SHOCK,
//		EFFECT_EXPLOSIVE,
		NUM_EFFECTS
	};

	static const char* ItemName( int item );
	static const char* GunType( int type );
	static const char* ItemPart( int item, int part );	// part from 0-3, matches WeaponGen order, but index not flag
	static const char* Effect( int effect );

	static GameItem* DoForge(	int itemType,		// GUN, etc.
								const Wallet& avail,
								TransactAmt* cost,
								int partsMask,
								int effectsMask,
								int tech,
								int level,
								int seed);


	GameItem* Build(	int itemType,		// GUN
				int subItemType,	// BLASTER
				int partsFlags,		// WeaponGen::GUN_CELL, etc.	
				int effectsFlags,	// GameItem effect flags, not enumeration above. GameItem::EFFECT_FIRE, etc.
				TransactAmt* required,
				int* techRequired,
				bool randomTraits );			

private:
	int seed;
	int userLevel;
	int techLevel;
};


#endif // FORGE_AI_INCLUDED
