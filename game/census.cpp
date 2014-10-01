#include "census.h"

#include "../script/itemscript.h"


void Census::Add(const grinliz::IString& name)
{
	for (int i = 0; i < mobItems.Size(); ++i) {
		if (mobItems[i].name == name) {
			mobItems[i].count += 1;
			return;
		}
	}
	MOBItem item;
	item.name = name;
	item.count = 1;
	mobItems.Push(item);
}


void Census::Remove(const grinliz::IString& name)
{
	for (int i = 0; i < mobItems.Size(); ++i) {
		if (mobItems[i].name == name) {
			GLASSERT(mobItems[i].count > 0);
			mobItems[i].count -= 1;
			return;
		}
	}
	GLASSERT(false);
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

