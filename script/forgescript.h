#ifndef FORGE_SCRIPT_INCLUDED
#define FORGE_SCRIPT_INCLUDED

#include "../grinliz/glrandom.h"
#include "../game/gameitem.h"

class ItemComponent;
class Wallet;
class TransactAmt;

class ForgeScript
{
public:
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
	};

	// gun parts and ring parts are in WeaponGen
	enum {
		EFFECT_FIRE,
		EFFECT_SHOCK,
		NUM_EFFECTS
	};

	static const char* ItemName(int item);
	static const char* GunType(int type);
	static const char* ItemPart(int item, int part);	// part from 0-3, matches WeaponGen order, but index not flag
	static const char* Effect(int effect);

	struct ForgeData {
		int type = GUN;				// GUN, etc.
		int subType = PISTOL; 		// if GUN, then: PISTOL, BLASTER, etc. (else ignored)
		int partsMask = 0xff;		// allowed parts (0xff for all)
		int effectsMask = GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK;		// allowed effects
		int tech = 0;
		int level = 0;
		int team = 0;
	};

	// Some teams have special restrictions on effects & parts.
	static void TeamLimitForgeData(ForgeData* data);
	// Some teams prefer some guns to another.
	static void BestSubItem(ForgeData* data, int seed);

	// seed=0 is special! creates the "average" weapon
	static GameItem* ForgeRandomItem(const ForgeData& forgeData, const Wallet& availabe, TransactAmt* cost, int seed);

	static GameItem* Build(const ForgeData& forgeData,
						   TransactAmt* required,
						   int* techRequired,
						   int seed);
};

#endif // FORGE_AI_INCLUDED
