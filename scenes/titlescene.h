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

#ifndef TITLESCENE_INCLUDED
#define TITLESCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/screenport.h"
#include "../xegame/cticker.h"

class LumosGame;
class TestMap;

class TitleScene : public Scene
{
	typedef Scene super;
public:
	TitleScene( LumosGame* game );
	virtual ~TitleScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D(U32 deltaTime);
	virtual void HandleHotKey(int value);
	virtual void DoTick(U32 delta);
	virtual grinliz::Color4F ClearColor();

private:
	void SetAudioButton();
	void DeleteEngine();

	LumosGame*		lumosGame;
	gamui::Image	background;

	enum {
		MANTIS,
		TROLL,
		HUMAN_FEMALE,
		HUMAN_MALE,
		RED_MANTIS,
		CYCLOPS,
		NUM_MODELS
	};

	// FIXME: custom engine+screenport is a PITA and weird. Hard to fix though.
	Screenport			screenport;
	Engine*				engine;
	TestMap*			testMap;
	Model* model[NUM_MODELS];
	//Model* temple;
	int					seed;
	CTicker				ticker;

	enum { 
		TEST_DIALOG,
		TEST_RENDER,
		TEST_COLOR,
		TEST_PARTICLE,
		TEST_NAV,
		TEST_NAV2,
		TEST_BATTLE,
		TEST_ANIMATION,
		TEST_ASSETPREVIEW,
		TEST_SOUND,
		TEST_FLUID,
		NUM_TESTS,

		TESTS_PER_ROW = 6,

		GENERATE_WORLD = 0,
		//DEFAULT_WORLD,
		CONTINUE,
		NUM_GAME
	};
	gamui::PushButton		testScene[NUM_TESTS];
	gamui::PushButton		gameScene[NUM_GAME];
	gamui::ToggleButton		audioButton;
	gamui::PushButton		creditsButton;
	gamui::TextLabel		note;
};

#endif // TITLESCENE_INCLUDED