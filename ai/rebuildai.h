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

#ifndef REBUILDAI_COMPONENT_INCLUDED
#define REBUILDAI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/chitbag.h"
#include "../xegame/cticker.h"

class RebuildAIComponent : public Component, public IChitListener
{
	typedef Component super;

public:
	RebuildAIComponent();
	~RebuildAIComponent();

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();
	virtual void Serialize(XStream* xs);

	virtual const char* Name() const { return "RebuildAIComponent"; }
	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);
	virtual int DoTick(U32 delta);

private:
	struct WorkItem {
		grinliz::IString structure;
		grinliz::Vector2I pos;
		int rot;
	};
	grinliz::CDynArray<WorkItem> workItems;
	CTicker ticker;
};

#endif // REBUILDAI_COMPONENT_INCLUDED

