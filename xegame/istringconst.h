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
	static grinliz::IString trigger;
	static grinliz::IString target;
	static grinliz::IString althand;
	static grinliz::IString head;
	static grinliz::IString shield;

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
	static grinliz::IString tombstone;
	
	static grinliz::IString pave;
	static grinliz::IString ice;
	static grinliz::IString core;
	static grinliz::IString vault;
	static grinliz::IString factory;
	static grinliz::IString forge;
	static grinliz::IString power;
	static grinliz::IString temple;
	static grinliz::IString bed;
	static grinliz::IString market;
	static grinliz::IString bar;
	static grinliz::IString guardpost;
	static grinliz::IString farm;
	static grinliz::IString distillery;

	static grinliz::IString zoneCreate;
	static grinliz::IString zoneConsume;
	static grinliz::IString industrial;
	static grinliz::IString natural;
	static grinliz::IString commercial;

	static grinliz::IString arachnoid;
	static grinliz::IString mantis;
	static grinliz::IString redMantis;
	static grinliz::IString troll;
	static grinliz::IString cyclops;
	static grinliz::IString fireCyclops;
	static grinliz::IString shockCyclops;
	static grinliz::IString dummyTarget;

	static grinliz::IString lesser;
	static grinliz::IString greater;
	static grinliz::IString denizen;

	// double underscore -> '.'
	static grinliz::IString kiosk__n;
	static grinliz::IString kiosk__m;
	static grinliz::IString kiosk__c;
	static grinliz::IString kiosk__s;
};

#endif // ISTRING_CONST_INCLUDED
