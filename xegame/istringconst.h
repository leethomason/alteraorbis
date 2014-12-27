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

#ifndef ISTRING_CONST_INCLUDED
#define ISTRING_CONST_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../game/gamelimits.h"

class IStringConst
{
public:
	static void Init();
	
	static grinliz::IString main;
	static grinliz::IString procedural;
	static grinliz::IString team;
	static grinliz::IString suit;
	static grinliz::IString gun;
	static grinliz::IString trigger;
	static grinliz::IString target;
	static grinliz::IString althand;
	static grinliz::IString head;
	static grinliz::IString shield;
	static grinliz::IString sound;
	static grinliz::IString speed;
	static grinliz::IString boltImpact;
	static grinliz::IString explosion;
	static grinliz::IString smoketrail;
	static grinliz::IString electron;
	static grinliz::IString fallingWater;
	static grinliz::IString fallingLava;
	static grinliz::IString mist;
	static grinliz::IString splash;
	static grinliz::IString smoke;
	static grinliz::IString fire;
	static grinliz::IString shock;
	static grinliz::IString derez;
	static grinliz::IString heal;
	static grinliz::IString constructiondone;
	static grinliz::IString meleeImpact;
	static grinliz::IString sparkExplosion;
	static grinliz::IString sparkPowerUp;
	static grinliz::IString teleport;
	static grinliz::IString construction;
	static grinliz::IString useBuilding;
	static grinliz::IString repair;

	static grinliz::IString human;
	static grinliz::IString humanFemale;
	static grinliz::IString humanMale;
	static grinliz::IString worker;
	static grinliz::IString mob;

	static grinliz::IString ring;
	static grinliz::IString pistol;
	static grinliz::IString blaster;
	static grinliz::IString pulse;
	static grinliz::IString beamgun;
	static grinliz::IString gold;
	static grinliz::IString crystal_green;
	static grinliz::IString crystal_red;
	static grinliz::IString crystal_blue;
	static grinliz::IString crystal_violet;
	static grinliz::IString fruit;
	static grinliz::IString elixir;
	static grinliz::IString tombstone;
	static grinliz::IString shieldBoost;

	static grinliz::IString rezWAV;
	static grinliz::IString derezWAV;
	static grinliz::IString explosionWAV;
	static grinliz::IString boltimpactWAV;
	static grinliz::IString buttonWAV;
	static grinliz::IString blasterWAV;

	static grinliz::IString pave;
	static grinliz::IString ice;
	static grinliz::IString core;
	static grinliz::IString vault;
	static grinliz::IString factory;
	static grinliz::IString forge;
	static grinliz::IString temple;
	static grinliz::IString bed;
	static grinliz::IString market;
	static grinliz::IString bar;
	static grinliz::IString guardpost;
	static grinliz::IString farm;
	static grinliz::IString distillery;
	static grinliz::IString exchange;
	static grinliz::IString turret;
	static grinliz::IString circuitFab;

	static grinliz::IString zone;
	static grinliz::IString industrial;
	static grinliz::IString natural;

	static grinliz::IString trilobyte;
	static grinliz::IString mantis;
	static grinliz::IString redMantis;
	static grinliz::IString troll;
	static grinliz::IString gobman;
	static grinliz::IString kamakiri;
	static grinliz::IString cyclops;
	static grinliz::IString fireCyclops;
	static grinliz::IString shockCyclops;
	static grinliz::IString dummyTarget;

	static grinliz::IString lesser;
	static grinliz::IString greater;
	static grinliz::IString denizen;

	static grinliz::IString destroyMsg;
	static grinliz::IString features;
	static grinliz::IString nStage;
	static grinliz::IString sun;
	static grinliz::IString rain;
	static grinliz::IString temp;

	static grinliz::IString Kills;
	static grinliz::IString Greater;
	static grinliz::IString Crafted;
	static grinliz::IString score;
	static grinliz::IString cost;
	static grinliz::IString size;
	static grinliz::IString accuracy;
	static grinliz::IString porch;
	static grinliz::IString circuit;
	static grinliz::IString nameGen;

	static grinliz::IString deity;
	static grinliz::IString Truulga;

	// double underscore -> '.'
	static grinliz::IString kiosk__n;
	static grinliz::IString kiosk__m;
	static grinliz::IString kiosk__c;
	static grinliz::IString kiosk__s;
	static grinliz::IString need__food;
	static grinliz::IString need__energy;
	static grinliz::IString need__fun;
};

typedef IStringConst ISC;

#endif // ISTRING_CONST_INCLUDED
