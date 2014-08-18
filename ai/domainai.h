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

#ifndef DOMAINAI_COMPONENT_INCLUDED
#define DOMAINAI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/chitbag.h"
#include "../xegame/cticker.h"

class DomainAI : public Component, public IChitListener
{
	typedef Component super;

public:
	DomainAI();
	~DomainAI();

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();
	virtual void Serialize(XStream* xs);

	virtual const char* Name() const { return "DomainAI"; }
	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);
	virtual int DoTick(U32 delta);

private:
	CTicker ticker;
};

#endif // REBUILDAI_COMPONENT_INCLUDED

