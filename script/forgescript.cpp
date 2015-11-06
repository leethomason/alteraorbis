#include "forgescript.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "procedural.h"
#include "../game/wallet.h"
#include "../game/gameitem.h"
#include "../game/team.h"
#include "../xegame/itemcomponent.h"
#include "itemscript.h"
#include "../grinliz/glutil.h"

using namespace grinliz;


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
		GLASSERT( part >= 0 && part < WeaponGen::NUM_PARTS );
		static const char* name[WeaponGen::NUM_PARTS] = { "Main", "Guard", "Triad", "Blade" };
		return name[part];
	}
	else if ( item == GUN ) {
		GLASSERT( part >= 0 && part < WeaponGen::NUM_PARTS );
		static const char* name[WeaponGen::NUM_PARTS] = { "Body", "Cell", "Driver", "Scope" };
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


GameItem* ForgeScript::ForgeRandomItem(const ForgeData& forgeData, const Wallet& avail, TransactAmt* cost, int seed, Wallet* payer)
{
	// Guarentee we can build something:
	if (avail.Crystal(0) == 0) {
		return 0;
	}

	Random random(seed);
	CArray<int, 8> partsArr;
	CArray<int, 4> effectsArr;

	// 0
	partsArr.Push(WeaponGen::GUN_CELL | WeaponGen::GUN_DRIVER | WeaponGen::GUN_SCOPE);
	// 1-3
	partsArr.Push(WeaponGen::GUN_DRIVER | WeaponGen::GUN_SCOPE);
	partsArr.Push(WeaponGen::GUN_CELL | WeaponGen::GUN_SCOPE);
	partsArr.Push(WeaponGen::GUN_CELL | WeaponGen::GUN_DRIVER);
	// 4-6
	partsArr.Push(WeaponGen::GUN_CELL );
	partsArr.Push(WeaponGen::GUN_DRIVER);
	partsArr.Push(WeaponGen::GUN_SCOPE);
	// 7
	partsArr.Push(0);

	// Mix up the equivalent values.
//	random.ShuffleArray(&partsArr[1], 3);
//	random.ShuffleArray(&partsArr[4], 3);
	random.SmallShuffle(partsArr.Mem(), partsArr.Size());

	effectsArr.Push(GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK);
	effectsArr.Push(GameItem::EFFECT_SHOCK);
	effectsArr.Push(GameItem::EFFECT_FIRE);
	effectsArr.Push(0);

	GameItem* item = 0;

	for (int j = 0; j < partsArr.Size(); ++j) {
		for (int i = 0; i < effectsArr.Size(); ++i) {
			int parts = partsArr[j];
			int effects = effectsArr[i];
			if ((~forgeData.partsMask) & parts) continue;
			if ((~forgeData.effectsMask) & effects) continue;

			ForgeData fd = forgeData;
			fd.partsMask = parts;
			fd.effectsMask = effects;

			int tech = 0;
			item = ForgeScript::Build(fd, cost, &tech, seed);
			if (item && avail.CanWithdraw(*cost) && tech <= forgeData.tech) {
				if (payer) {
					if (payer->CanWithdraw(*cost) || payer->CanBeUnderwater()) {
						item->wallet.Deposit(payer, *cost);
					}
					else {
						delete item; 
						item = 0;
					}
				}
				return item; // success!
			}
			delete item;
			item = 0;
		}
	}

	GLOUTPUT(("Warning. Item not created in forgescript.\n"));
	return 0;
}


GameItem* ForgeScript::Build(const ForgeData& forgeData,
							 TransactAmt* required,
							 int* techRequired,
							 int seed)
{
	*techRequired = 0;
	required->Clear();
	required->AddCrystal(CRYSTAL_GREEN, 1);

	GLASSERT(forgeData.type >= 0);
	GLASSERT(forgeData.subType >= 0);

	// Random, but can't leave forge and come back
	// to get different numbers.
	Random random;
	random.SetSeed(seed
				   ^ (forgeData.type));	// do NOT include parts - should not change on parts changing
	// not changing on sub-type either

	const char* typeName = "pistol";

	int roll[GameTrait::NUM_TRAITS] = { 10, 10, 10, 10, 10 };
	if (seed) {
		for (int i = 0; i < GameTrait::NUM_TRAITS; ++i) {
			roll[i] = random.WeightedDice(3, 6, 1, 1);
		}
	}
	static const int COMPETENCE = 4;
	for (int i = 0; i < GameTrait::NUM_TRAITS; ++i) {
		roll[i] = Clamp(roll[i] - COMPETENCE + forgeData.level, 3, 18);
	}

	static const int BONUS = 2;

	int features = 0;
	if (forgeData.type == ForgeScript::GUN) {
		if (forgeData.subType == BLASTER)		{ typeName = "blaster"; }
		else if (forgeData.subType == PULSE)	{ typeName = "pulse";   *techRequired += 1; }
		else if (forgeData.subType == BEAMGUN)	{ typeName = "beamgun"; *techRequired += 1; }

		int nParts = 0;
		if (forgeData.partsMask & WeaponGen::GUN_CELL)		{
			features |= WeaponGen::GUN_CELL;
			roll[GameTrait::ALT_CAPACITY] += BONUS * 2;
			*techRequired += 1;
			++nParts;
		}
		if (forgeData.partsMask & WeaponGen::GUN_DRIVER)	{
			features |= WeaponGen::GUN_DRIVER;
			roll[GameTrait::ALT_DAMAGE] += BONUS;
			*techRequired += 1;
			++nParts;
		}
		if (forgeData.partsMask & WeaponGen::GUN_SCOPE)		{
			features |= WeaponGen::GUN_SCOPE;
			roll[GameTrait::ALT_ACCURACY] += BONUS;
			*techRequired += 1;
			++nParts;
		}
		if (nParts > 1) {
			required->AddCrystal(0, (nParts - 1)*(nParts - 1));
		}
	}
	else if (forgeData.type == ForgeScript::RING) {
		typeName = "ring";

		if (forgeData.partsMask & WeaponGen::RING_GUARD)		{
			features |= WeaponGen::RING_GUARD;
			roll[GameTrait::CHR] += BONUS;
			*techRequired += 1;
		}
		if (forgeData.partsMask & WeaponGen::RING_TRIAD)		{
			features |= WeaponGen::RING_TRIAD;
			roll[GameTrait::ALT_DAMAGE] += BONUS / 2;
			*techRequired += 1;
		}
		if (forgeData.partsMask & WeaponGen::RING_BLADE)		{
			features |= WeaponGen::RING_BLADE;
			roll[GameTrait::CHR] -= BONUS;
			roll[GameTrait::ALT_DAMAGE] += BONUS;
		}
	}
	else if (forgeData.type == ForgeScript::SHIELD) {
		typeName = "shield";
	}

	for (int i = 0; i < GameTrait::NUM_TRAITS; ++i) {
		roll[i] = Clamp(roll[i], 3, 18);
	}

	const GameItem& itemDef = ItemDefDB::Instance()->Get(typeName);
	GameItem* item = itemDef.Clone();
	item->Roll(forgeData.team, roll);

	if (forgeData.effectsMask & GameItem::EFFECT_FIRE) {
		required->AddCrystal(CRYSTAL_RED, 1);
	}
	if (forgeData.effectsMask & GameItem::EFFECT_SHOCK) {
		required->AddCrystal(CRYSTAL_BLUE, 1);
	}
	if (forgeData.effectsMask & GameItem::EFFECT_EXPLOSIVE) {
		required->AddCrystal(CRYSTAL_VIOLET, 1);
	}
	// 2 effects adds a 2nd violet crystal
	if (int(FloorPowerOf2(forgeData.effectsMask)) != forgeData.effectsMask) {
		required->AddCrystal(CRYSTAL_VIOLET, 1);
	}

	if (item) {
		item->keyValues.Set("features", features);
		item->flags &= ~GameItem::EFFECT_MASK;
		item->flags |= forgeData.effectsMask;
	}
	return item;
}


void ForgeScript::TeamLimitForgeData(ForgeScript::ForgeData* data)
{
	int group = 0, id = 0;
	Team::SplitID(data->team, &group, &id);

	// Only trolls use blades.
	if (data->type == RING) {
		if (group == TEAM_TROLL || group == DEITY_TRUULGA) {
			// don't limit.
		}
		else {
			data->partsMask &= (~WeaponGen::RING_BLADE);
		}
	}
	// Q core very cheap weapons
	if (group == DEITY_Q) {
		data->effectsMask = 0;
		data->partsMask   = 0;
	}
}


void ForgeScript::BestSubItem(ForgeScript::ForgeData* data, int seed)
{
	if (data->type != GUN) {
		data->subType = 0;
		return;
	}
	Random random(seed);
	int group = 0, id = 0;
	Team::SplitID(data->team, &group, &id);

	if (data->tech) {
		data->subType = random.Bit() ? BEAMGUN : PULSE;
	}
	else {
		data->subType = random.Bit() ? PISTOL : BLASTER;
	}

	if (data->tech == 0 && group == TEAM_GOB)
		data->subType = PISTOL;
	else if (data->tech > 0 && group == TEAM_KAMAKIRI)
		data->subType = BEAMGUN;
}
