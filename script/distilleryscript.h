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

#ifndef DISTILLERY_SCRIPT_INCLUDED
#define DISTILLERY_SCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../xegame/cticker.h"

class DistilleryScript : public IScript
{
public:
	DistilleryScript();
	virtual ~DistilleryScript()	{}

	virtual void Init()					{}
	virtual void OnAdd()				{}
	virtual void OnRemove()				{}
	virtual void Serialize( XStream* xs );

	virtual int DoTick( U32 delta );
	virtual const char* ScriptName()	{ return "DistilleryScript"; }

	enum { 
		ELIXIR_TIME = 10*1000,
		ELIXIR_PER_FRUIT = 2
	};
	static int ElixirTime( int tech );

private:
	void SetInfoText();

	// serialized
	CTicker		progressTick;
	int			progress;
};

#endif // DISTILLERY_SCRIPT_INCLUDED
