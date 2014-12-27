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

#ifndef CENSUS_LUMOS_INCLUDED
#define CENSUS_LUMOS_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

class Census
{
public:
	Census() {
	}

	void AddMOB(const grinliz::IString& name);
	void RemoveMOB(const grinliz::IString& name);
	void AddCore(const grinliz::IString& team);
	void RemoveCore(const grinliz::IString& team);

	void NumByType(int *lesser, int *greater, int *denizen);

	struct MOBItem {
		MOBItem() : count(0) {}
		MOBItem(const grinliz::IString& n) : name(n), count(1) {}
		bool operator<(const MOBItem& rhs) const { return name < rhs.name; }
		bool operator==(const MOBItem& rhs) const { return name == rhs.name; }

		grinliz::IString name;
		int count;
	};

	const grinliz::CDynArray<MOBItem>& MOBItems() const { return mobItems; }
	const grinliz::CDynArray<MOBItem>& CoreItems() const { return coreItems; }

private:
	grinliz::CDynArray<MOBItem> mobItems;
	grinliz::CDynArray<MOBItem> coreItems;
};

#endif 
