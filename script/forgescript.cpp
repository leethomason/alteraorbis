#include "forgescript.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "procedural.h"
#include "../game/wallet.h"
#include "../game/gameitem.h"
#include "../xegame/itemcomponent.h"
#include "itemscript.h"
#include "../grinliz/glutil.h"

using namespace grinliz;


ForgeScript::ForgeScript( int seed, int userLevel, int techLevel )
{
	this->seed = seed;
	this->userLevel = userLevel;
	this->techLevel = techLevel;
}


const char* ForgeScript::ItemName( int item )
{
	GLASSERT( item >= 0 && item < NUM_ITEM_TYPES );
	static const char* name[NUM_ITEM_TYPES] = { "Ring", "Gun", "Shield" };
	return name[item];
}


const char* ForgeScript::GunType( int type )
{
	static const char* name[NUM_GUN_TYPES] = { "Pistol", "Blaster", "Pulse", "Beamgun" };
	GLASSERT( type >= 0 && type < NUM_GUN_TYPES );
	return name[type];
}


const char* ForgeScript::ItemPart( int item, int part )
{
	if ( item == RING ) {
		GLASSERT( part >= 0 && part < NUM_RING_PARTS );
		static const char* name[NUM_RING_PARTS] = { "Main", "Guard", "Triad", "Blade" };
		return name[part];
	}
	else if ( item == GUN ) {
		GLASSERT( part >= 0 && part < NUM_GUN_PARTS );
		static const char* name[NUM_GUN_PARTS] = { "Body", "Cell", "Driver", "Scope" };
		return name[part];				
	}
	else {
		GLASSERT( 0 );
	}
	return 0;
}



const char* ForgeScript::Effect( int effect )
{
	GLASSERT( effect >= 0 && effect <NUM_EFFECTS );
	static const char* name[NUM_EFFECTS] = { "Fire", "Shock", /*"Explosive"*/ };
	return name[effect];
}



GameItem* ForgeScript::DoForge(int itemType,		// GUN, etc.
							   int subItemType,
							   const Wallet& avail,
							   TransactAmt* cost,	// in/out
							   int partsMask,
							   int effectsMask,
							   int tech,
							   int level,
							   int seed,
							   int team)
{
	// Guarentee we can build something:
	if (avail.Crystal(0) == 0) {
		return 0;
	}

	int subType = 0;
	int parts = 1;		// always have body.
	Random random(seed);

	int partsArr[] = { WeaponGen::GUN_CELL, WeaponGen::GUN_DRIVER, WeaponGen::GUN_SCOPE };
	int effectsArr[] = { GameItem::EFFECT_FIRE, GameItem::EFFECT_SHOCK };	// don't allow explosive to be manufactured, yet: , GameItem::EFFECT_EXPLOSIVE

	// Get the inital to "everything"
	int effects = 0;
	for (int i = 0; i < GL_C_ARRAY_SIZE(effectsArr); ++i) {
		effects |= effectsArr[i];
	}
	effects = effects & effectsMask;

	if (itemType == ForgeScript::GUN) {
		if (subItemType >= 0) {
			subType = subItemType;
		}
		else {
			if (tech == 0)
				subType = random.Rand(ForgeScript::NUM_TECH0_GUNS);
			else
				subType = random.Rand(ForgeScript::NUM_GUN_TYPES);
			parts = WeaponGen::PART_MASK & partsMask;
		}
	}
	else if (itemType == ForgeScript::RING) {
		parts = WeaponGen::RING_GUARD | WeaponGen::RING_TRIAD | WeaponGen::RING_BLADE;
		parts = parts & partsMask;
	}

	random.ShuffleArray(partsArr, GL_C_ARRAY_SIZE(partsArr));
	random.ShuffleArray(effectsArr, GL_C_ARRAY_SIZE(effectsArr));

	ForgeScript forge(random.Rand(), level, tech);

	int cp = 0;
	int ce = 0;

	// Given we have a crystal, we should always be able to build something.
	// Remove sub-parts and effects until we succeed. As the forge 
	// changes, this is no longer a hard rule.
	int maxIt = 10;
	GameItem* item = 0;
	while (maxIt) {
		delete item; item = 0;

		cost->Clear();
		int techRequired = 0;
		item = forge.Build(itemType, subType, parts, effects, cost, &techRequired, true, team);

		if (avail.CanWithdraw(*cost) && techRequired <= tech) {
			break;
		}

		bool didSomething = false;
		if (avail.CanWithdraw(*cost) && ce < GL_C_ARRAY_SIZE(effectsArr)) {
			effects &= ~(effectsArr[ce++]);
			didSomething = true;
		}
		if (techRequired > tech && cp < GL_C_ARRAY_SIZE(partsArr)) {
			parts &= ~(partsArr[cp++]);
			didSomething = true;
		}

		// The equations between cost are more complex; the
		// model above doesn't capture all the combos any more.
		// Make sure something changes every loop.
		if (!didSomething) {
			if (ce < GL_C_ARRAY_SIZE(effectsArr)) {
				effects &= ~(effectsArr[ce++]);
			}
			if (cp < GL_C_ARRAY_SIZE(partsArr)) {
				parts &= ~(partsArr[cp++]);
			}
		}
		--maxIt;
	}
	if (!item) {
		GLOUTPUT(("Warning. Item not created in forgescript.\n"));
	}
	return item;
}


GameItem* ForgeScript::Build(	int type,			// GUN
								int subType,		// BLASTER
								int partsFlags,		
								int effectFlags,
								TransactAmt* required,
								int *techRequired,
								bool doRandom,
								int team)
{
	*techRequired = 0;
	required->Clear();
	required->AddCrystal(CRYSTAL_GREEN, 1);

	// Random, but can't leave forge and come back
	// to get different numbers.
	Random random;
	random.SetSeed(   seed
					^ (type) );	// do NOT include parts - should not change on parts changing
								// not changing on sub-type either

	const char* typeName = "pistol";

	int roll[GameTrait::NUM_TRAITS] = { 10, 10, 10, 10, 10 };
	if ( doRandom ) {
		for( int i=0; i<GameTrait::NUM_TRAITS; ++i ) {
			roll[i] = random.Dice(3, 6);
		}
	}
	static const int COMPETENCE = 4;
	for (int i = 0; i < GameTrait::NUM_TRAITS; ++i) {
		roll[i] = Clamp(roll[i] - COMPETENCE + userLevel, 3, 18);
	}

	static const int BONUS = 2;

	int features = 0;
	if ( type == ForgeScript::GUN ) {
		if      ( subType == BLASTER )	{ typeName = "blaster"; }
		else if ( subType == PULSE )	{ typeName = "pulse";   *techRequired += 1; }
		else if ( subType == BEAMGUN )	{ typeName = "beamgun"; *techRequired += 1; }

		int nParts = 0;
		if ( partsFlags & WeaponGen::GUN_CELL )		{ 
			features |= WeaponGen::GUN_CELL;		
			roll[GameTrait::ALT_CAPACITY] += BONUS*2; 
			*techRequired += 1;
			++nParts;
		}
		if ( partsFlags & WeaponGen::GUN_DRIVER )		{ 
			features |= WeaponGen::GUN_DRIVER;	
			roll[GameTrait::ALT_DAMAGE]   += BONUS;	
			*techRequired += 1;
			++nParts;
		}
		if ( partsFlags & WeaponGen::GUN_SCOPE )		{ 
			features |= WeaponGen::GUN_SCOPE;		
			roll[GameTrait::ALT_ACCURACY] += BONUS; 
			*techRequired += 1;
			++nParts;
		}
		if (nParts > 1) {
			required->AddCrystal(0, (nParts - 1)*(nParts - 1));
		}
	}
	else if ( type == ForgeScript::RING ) {
		typeName = "ring";

		if ( partsFlags & WeaponGen::RING_GUARD )		{ 
			features |= WeaponGen::RING_GUARD;	
			roll[GameTrait::CHR] += BONUS; 
			*techRequired += 1;
		}	
		if ( partsFlags & WeaponGen::RING_TRIAD )		{ 
			features |= WeaponGen::RING_TRIAD;	
			roll[GameTrait::ALT_DAMAGE] += BONUS/2; 
			*techRequired += 1;
		}
		if ( partsFlags & WeaponGen::RING_BLADE )		{ 
			features |= WeaponGen::RING_BLADE;	
			roll[GameTrait::CHR] -= BONUS; 
			roll[GameTrait::ALT_DAMAGE] += BONUS; 
		}	
	}
	else if ( type == ForgeScript::SHIELD ) {
		typeName = "shield";
	}

	for (int i = 0; i < GameTrait::NUM_TRAITS; ++i) {
		roll[i] = Clamp(roll[i], 1, 20);
	}

	
	const GameItem& itemDef = ItemDefDB::Instance()->Get( typeName );
	GameItem* item = itemDef.Clone();
	item->Roll(team, roll);

	if ( effectFlags & GameItem::EFFECT_FIRE ) {
		required->AddCrystal(CRYSTAL_RED, 1);
	}
	if ( effectFlags & GameItem::EFFECT_SHOCK ) {
		required->AddCrystal(CRYSTAL_BLUE, 1);
	}
	if ( effectFlags & GameItem::EFFECT_EXPLOSIVE ) {
		required->AddCrystal(CRYSTAL_VIOLET, 1);
	}
	// 2 effects adds a 2nd violet crystal
	if ( FloorPowerOf2( effectFlags ) != effectFlags ) {
		required->AddCrystal(CRYSTAL_VIOLET, 1);
	}

	if ( item ) {
		item->keyValues.Set( "features", features );
		item->flags &= ~GameItem::EFFECT_MASK;
		item->flags |= effectFlags;
	}
	return item;
}

