#include "census.h"

#include "../script/itemscript.h"


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

void Census::NumByType(int *lesser, int *greater, int *denizen)
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

