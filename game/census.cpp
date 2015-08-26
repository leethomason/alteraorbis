#include "census.h"

#include "../grinliz/glstringutil.h"
#include "../script/itemscript.h"
#include "../game/team.h"


void Census::AddMOB(const grinliz::IString& name)
{
	MOBItem item(name);

	int i = mobItems.Find(item);
	if (i >= 0) {
		mobItems[i].count += 1;
		return;
	}
	mobItems.Push(item);
	mobItems.Sort();
}


void Census::RemoveMOB(const grinliz::IString& name)
{
	MOBItem item(name);

	int i = mobItems.Find(name);
	GLASSERT(i >= 0);
	if (i >= 0) {
		mobItems[i].count -= 1;
	}
}


void Census::AddCore(const grinliz::IString& name)
{
	MOBItem item(name);

	int i = coreItems.Find(name);
	if (i >= 0) {
		coreItems[i].count += 1;
		return;
	}
	coreItems.Push(item);
	coreItems.Sort();
}


void Census::RemoveCore(const grinliz::IString& name)
{
	MOBItem item(name);
	int i = coreItems.Find(item);
	GLASSERT(i >= 0);
	if (i >= 0) {
		coreItems[i].count -= 1;
	}
}

void Census::NumByType(int *lesser, int *greater, int *denizen) const
{
	*lesser = 0;
	*greater = 0;
	*denizen = 0;

	ItemDefDB* itemDefDB = ItemDefDB::Instance();

	for (int i = 0; i < mobItems.Size(); ++i) {
		const MOBItem& item = mobItems[i];
		if (itemDefDB->GreaterMOBs().Find(item.name) >= 0) {
			*greater += item.count;
		}
		else if (itemDefDB->LesserMOBs().Find(item.name) >= 0) {
			*lesser += item.count;
		}
		else {
			*denizen += item.count;
		}
	}
}


int Census::NumCoresOfTeam(int group) const
{
	grinliz::IString name = Team::Instance()->TeamName(Team::Group(group));
	int count = 0;
	for (int i = 0; i < coreItems.Size(); ++i) {
		if (coreItems[i].name == name) {
			count += coreItems[i].count;
		}
	}
	return count;
}


int Census::NumOf(const grinliz::IString& name, int* typical) const
{
	int result = 0;
	int idx = mobItems.Find(name);
	if (idx >= 0) {
		result = mobItems[idx].count;
	}
	if (typical) {
		*typical = 0;
		idx = typicalMobs.Find(name);
		if (idx >= 0) {
			*typical = typicalMobs[idx].count;
		}
	}
	return result;
}


void Census::SetTypical(const grinliz::IString& name, int typicalNum)
{
	int idx = typicalMobs.Find(name);
	if (idx >= 0) {
		typicalMobs[idx].count = typicalNum;
	}
	else {
		MOBItem item(name);
		item.count = typicalNum;
		typicalMobs.Push(item);
	}
}
