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
#include "../xegame/chit.h"

#include "../widget/moneywidget.h"
#include "../widget/facewidget.h"
#include "../widget/consolewidget.h"
#include "../widget/startwidget.h"
#include "../widget/endwidget.h"
#include "../widget/barstack.h"

#include "../script/buildscript.h"

#include "../ai/aineeds.h"

class LumosGame;
class Sim;
class NewsEvent;
class Chit;
class GameItem;
class Adviser;

class GameScene : public Scene,
				  public IChitListener
{
	typedef Scene super;
public:
	GameScene( LumosGame* game );
	~GameScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );
	void MoveCamera(float dx, float dy);

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	void Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world);
	virtual void ItemTapped(const gamui::UIItem* item);
	virtual void HandleHotKey( int mask );
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world );

	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();

	virtual void SceneResult( int sceneID, int result, const SceneData* data );
	virtual void DialogResult(const char* dialogName, void* data);

	void Save();
	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);

private:
	void Tap3D(const grinliz::Vector2F& view, const grinliz::Ray& world);
	void Load();
	void SetBars( Chit* chit );
	void DoDestTapped( const grinliz::Vector2F& grid );

	void TapModel( Chit* chit );
	void MoveModel( Chit* chit );
	void ClearTargetFlags();
	void SetSelectionModel( const grinliz::Vector2F& view );

	void SetPickupButtons();	// if the avatar can pick things up
	void SetBuildButtons(const int* buildingCount);		// enable / disable menu items
	//void SetHelpText(const int* buildingCount, int nWorkers);
	void ProcessNewsToConsole();
	void CheckGameStage(U32 delta);
	void ForceHerd(const grinliz::Vector2I& sector);
	bool AvatarSelected();
	bool CameraTrackingAvatar();
	bool DragAtom(gamui::RenderAtom* atom);
	void BuildAction(const grinliz::Vector2I& pos2i);

	void DoCameraHome();
	void DoAvatarButton();

	enum {
		NUM_PICKUP_BUTTONS = 8,
		
		NUM_NEWS_BUTTONS = 12,
		NEWS_BUTTON_WIDTH  = 60,
		NEWS_BUTTON_HEIGHT = 25,
		NUM_BUILD_MARKS = 100
	};
	enum {
		NUM_BUILD_MODES = 6
	};

	enum {
		UI_VIEW,
		UI_BUILD,
		UI_CONTROL,
		NUM_UI_MODES
	};

	struct SectorInfo
	{
		const SectorData* sectorData;
		int bioFlora;

		// sort descending
		bool operator<(const SectorInfo& rhs) const { return this->bioFlora > rhs.bioFlora; }
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
	int			endTimer;

	int					targetChit;
	int					possibleChit;
	int					infoID;
	grinliz::Vector2I	voxelInfoID;
	int					buildActive;	// which build button is active. 0 if none.
	int					chitTracking;
	int					currentNews;	// index of the last news item put in the console
	int					coreWarningTimer, domainWarningTimer;
	grinliz::Vector2F	coreWarningPos, domainWarningPos;
	int					poolView;
	float				savedCameraHeight;
	grinliz::Quaternion	savedCameraRotation;
	grinliz::Vector2F	mapDragStart;
	grinliz::Vector2F	tapView;
	Adviser*			adviser;

	// Shows what is being built or removed.
	Model*				selectionModel;
	BuildScript			buildScript;

	gamui::PushButton	okay;
	gamui::PushButton	saveButton, loadButton;
	gamui::ToggleButton	buildButton[BuildScript::NUM_OPTIONS];
	gamui::ToggleButton modeButton[NUM_BUILD_MODES];
	gamui::PushButton	useBuildingButton;
	gamui::PushButton	cameraHomeButton;
	gamui::PushButton	nextUnit, avatarUnit, prevUnit;
	gamui::Image		tabBar0, tabBar1;
	gamui::PushButton	createWorkerButton;
	gamui::ToggleButton	uiMode[NUM_UI_MODES];
	gamui::PushButton	allRockButton;
	gamui::PushButton	censusButton;
	gamui::PushButton	newsButton[NUM_NEWS_BUTTONS];
	gamui::Image		minimap;
	gamui::Image		playerMark;
	gamui::Image		buildMark[NUM_BUILD_MARKS];	// used for click&drag building
	gamui::PushButton	coreWarningIcon;
	gamui::PushButton	domainWarningIcon;
	gamui::PushButton	atlasButton;
	gamui::ToggleButton	autoRebuild;
	gamui::PushButton	abandonButton;
	gamui::TextLabel	buildDescription;
	gamui::PushButton	swapWeapons;
	gamui::Image		helpImage;
	gamui::TextLabel	helpText;
	BarStackWidget		summaryBars;

	FacePushWidget		faceWidget,
						targetFaceWidget;

	gamui::TextLabel	dateLabel;
	gamui::TextLabel	techLabel;
	MoneyWidget			moneyWidget;
	ConsoleWidget		consoleWidget;
	StartGameWidget		startGameWidget;
	EndGameWidget		endGameWidget;

	gamui::PushButton	pickupButton[NUM_PICKUP_BUTTONS];

	grinliz::CDynArray< Chit* >			chitQuery;
	grinliz::CDynArray< PickupData >	pickupData;
};


#endif // LUMOS_GAME_SCENE_INCLUDED
