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

#ifndef GAME_MOVE_COMPONENT_INCLUDED
#define GAME_MOVE_COMPONENT_INCLUDED

#include "../xegame/component.h"

class WorldMap;

/*
	A GameMoveComponent, without a subclass, impart basic Fluid Physics
	(and hopefully in the future general physics.)
*/
class GameMoveComponent : public MoveComponent
{
private:
	typedef MoveComponent super;

public:
	GameMoveComponent()				{ velocity.Zero(); }
	virtual ~GameMoveComponent()	{}

	virtual const char* Name() const { return "GameMoveComponent"; }
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	float Speed() const;

	virtual bool ShouldAvoid() const			{ return false; }	// FIXME: base on mass, or item properties??
	virtual void Serialize( XStream* xs );
	virtual int DoTick( U32 delta );
	virtual grinliz::Vector3F Velocity()		{ return velocity; }

protected:
	// Apply fluid effects
	// return: true if in fluid
	// in/out: pos adjusted by fluid motion
	// out: floating, true if floating and should NOT use path or block motion
	bool ApplyFluid(U32 delta, grinliz::Vector3F* pos, bool* floating);
	// Keep from hitting world objects.
	void ApplyBlocks( grinliz::Vector2F* pos, bool* forceApplied );

private:
	grinliz::Vector3F velocity;
};


#endif // GAME_MOVE_COMPONENT_INCLUDED
