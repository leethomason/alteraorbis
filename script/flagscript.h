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

#ifndef Flag_SCRIPT_INCLUDED
#define Flag_SCRIPT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/cticker.h"


class FlagScript : public Component
{
	typedef Component super;
public:
	FlagScript();
	virtual ~FlagScript()		{}

	void Attach(const grinliz::Vector2I& sector, int squad);

	virtual void OnAdd(Chit* chit, bool init);
	virtual void Serialize(XStream* xs);
	virtual int DoTick(U32 delta);
	virtual const char* Name() const { return "FlagScript"; }

private:
	CTicker timer;
	bool colorsSet;
	grinliz::Vector2I sector;
	int squadID;
};

#endif // Flag_SCRIPT_INCLUDED
