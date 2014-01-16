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

#ifndef LUMOS_GAME_SCENE_INCLUDED
#define LUMOS_GAME_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../xegame/chitevent.h"

#include "../widget/moneywidget.h"
#include "../widget/facewidget.h"
#include "../widget/consolewidget.h"

#include "../script/buildscript.h"

#include "../ai/aineeds.h"

class LumosGame;
class Sim;
class NewsEvent;
class Chit;
class GameItem;


class GameScene : public Scene
{
	typedef Scene super;
public:
	GameScene( LumosGame* game );
	~GameScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void HandleHotKey( int mask );
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world );

	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();

	virtual void SceneResult( int sceneID, int result, const SceneData* data );

private:
	void Save();
	void Load();
	void SetBars( Chit* chit );
	void DoDestTapped( const grinliz::Vector2F& grid );

	void TapModel( Chit* chit );
	void MoveModel( Chit* chit );
	void ClearTargetFlags();
	void SetSelectionModel( const grinliz::Vector2F& view );

	void SetPickupButtons();
	bool FreeCameraMode();
	bool CoreMode();		// currently controlling the core (build or view)
	void ProcessNewsToConsole();

	enum {
		SAVE,
		LOAD,
		NUM_SERIAL_BUTTONS,
		NUM_PICKUP_BUTTONS = 8,
		
		NUM_NEWS_BUTTONS = 12,
		NEWS_BUTTON_WIDTH  = 60,
		NEWS_BUTTON_HEIGHT = 25
	};
	enum {
		BUILD_XFORM,
		BUILD_TECH0,
		BUILD_TECH1,
		BUILD_TECH2,
		BUILD_TECH3,
		NUM_BUILD_MODES
	};

	enum {
		UI_BUILD,
		UI_VIEW,
		UI_AVATAR,
		NUM_UI_MODES
	};

	struct PickupData {
		int		chitID;
		float	distance;	// pather distance

		bool operator<( const PickupData& rhs ) const { return distance < rhs.distance; }
	};

	// returns the name from the build button
	grinliz::IString StructureInfo( int buildButtonIndex, int* size );

	LumosGame*	lumosGame;
	Sim*		sim;
	bool		fastMode;
	int			simTimer;		// used to count sim ticks/second
	int			simCount;
	float		simPS;

	int					targetChit;
	int					possibleChit;
	int					infoID;
	grinliz::Vector2I	voxelInfoID;
	int					buildActive;	// which build button is active. 0 if none.
	int					chitFaceToTrack;
	int					currentNews;	// index of the last news item put in the console

	// Shows what is being built or removed.
	Model*				selectionModel;
	BuildScript			buildScript;

	gamui::PushButton	okay;
	gamui::PushButton	serialButton[NUM_SERIAL_BUTTONS];
	gamui::ToggleButton	buildButton[BuildScript::NUM_OPTIONS];
	gamui::ToggleButton modeButton[NUM_BUILD_MODES];
	gamui::PushButton	useBuildingButton;
	gamui::PushButton	cameraHomeButton;
	gamui::Image		tabBar;
	gamui::PushButton	createWorkerButton;
	gamui::ToggleButton	uiMode[NUM_UI_MODES];
	gamui::PushButton	allRockButton;
	gamui::PushButton	censusButton;
	gamui::PushButton	newsButton[NUM_NEWS_BUTTONS];
	gamui::PushButton	clearButton;
	gamui::Image		minimap;
	gamui::Image		playerMark;

	FacePushWidget		faceWidget;

	gamui::TextLabel	dateLabel;
	gamui::TextLabel	techLabel;
	MoneyWidget			moneyWidget;
	ConsoleWidget		consoleWidget;

	gamui::PushButton	pickupButton[NUM_PICKUP_BUTTONS];

	grinliz::CDynArray< Chit* >			chitQuery;
	grinliz::CDynArray< PickupData >	pickupData;
};


#endif // LUMOS_GAME_SCENE_INCLUDED
