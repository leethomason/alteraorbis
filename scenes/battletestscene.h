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

#ifndef BATTLE_SCENE_INCLUDED
#define BATTLE_SCENE_INCLUDED

#include "../grinliz/glrandom.h"

#include "../xegame/scene.h"
#include "../xegame/chitbag.h"

#include "../script/itemscript.h"

class LumosGame;
class Engine;
class WorldMap;


class BattleTestScene : public Scene, public IChitListener
{
public:
	BattleTestScene( LumosGame* game );
	~BattleTestScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world ) { debugRay = world; }
	virtual void HandleHotKeyMask( int mask );

private:

	void LoadMap();
	void CreateChit( const grinliz::Vector2I& p, int type, int loadout, int team );
	void GoScene();
	int ButtonDownID( int group );

	gamui::PushButton okay, goButton;

	struct ButtonDef
	{
		int			id;
		const char* label;
		int			group;
	};

	enum {
		COUNT_1,
		COUNT_2,
		COUNT_4,
		COUNT_8,

		DUMMY,		
		HUMAN,		// fist, ring, ranged weapon
		MANTIS,		// pincer
		BALROG,		// fist, axe, ranged intrinsic
		
		NO_WEAPON,
		MELEE_WEAPON,
		PISTOL,
		
		NUM_OPTIONS,
		NUM_BUTTONS = NUM_OPTIONS*2,

		LEFT=0,
		MID,
		RIGHT,

		LEFT_COUNT=1,
		LEFT_MOB,
		LEFT_WEAPON,
		RIGHT_COUNT,
		RIGHT_MOB,
		RIGHT_WEAPON
	};
	gamui::ToggleButton optionButton[NUM_BUTTONS];
	static const ButtonDef buttonDef[NUM_BUTTONS];

	Engine* engine;
	WorldMap* map;
	bool battleStarted;
	U32 boltTimer;

	grinliz::Random random;
	grinliz::Random fuzz;
	grinliz::CDynArray<grinliz::Vector2I> waypoints[3];
	grinliz::Ray debugRay;
	grinliz::CDynArray<Chit*> chitArr;
	
	ChitBag chitBag;
	ItemStorage itemStorage;
};


#endif // BATTLE_SCENE_INCLUDED
