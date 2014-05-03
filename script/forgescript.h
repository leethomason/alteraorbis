#ifndef FORGE_SCRIPT_INCLUDED
#define FORGE_SCRIPT_INCLUDED

#include "../grinliz/glrandom.h"

class ItemComponent;
struct Wallet;
class GameItem;

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

	void Build(	int itemType,		// GUN
				int subItemType,	// BLASTER
				int partsFlags,		
				int effectsFlags,	// GameItem effect flags, not enumeration above
				GameItem* target,
				Wallet* required,
				int* techRequired,
				bool randomTraits );			

private:
	int seed;
	int userLevel;
	int techLevel;
};


#endif // FORGE_AI_INCLUDED
