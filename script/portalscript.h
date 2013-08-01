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

#ifndef PORTAL_SCRIPT_INCLUDED
#define PORTAL_SCRIPT_INCLUDED

#include "scriptcomponent.h"
#include "../xegame/cticker.h"

class WorldMap;
class WorkQueue;
class LumosChitBag;
#if 0
class PortalScript : public IScript
{
public:
	PortalScript( WorldMap* map, LumosChitBag* chitBag, Engine* engine );
	virtual ~PortalScript();

	virtual void Init( const ScriptContext& heap );
	virtual void Serialize( const ScriptContext& ctx, XStream* xs );
	virtual void OnAdd( const ScriptContext& ctx );
	virtual void OnRemove( const ScriptContext& ctx );

	virtual int DoTick( const ScriptContext& ctx, U32 delta, U32 since );
	virtual const char* ScriptName() { return "PortalScript"; }
	virtual PortalScript* ToPortalScript() { return this; }

private:
	WorldMap*	worldMap;
	WorkQueue*	workQueue;
};
#endif

#endif // PORTAL_SCRIPT_INCLUDED
