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

#include "../game/newsconsole.h"
#include "../game/workqueue.h"

#include "../widget/moneywidget.h"
#include "../widget/facewidget.h"
#include "../widget/startwidget.h"
#include "../widget/endwidget.h"
#include "../widget/barstack.h"
#include "../widget/hpbar.h"

#include "../script/buildscript.h"

#include "../ai/aineeds.h"

class LumosGame;
class Sim;
class NewsEvent;
class Chit;
class GameItem;
class Adviser;
class GameSceneMenu;
class TutorialWidget;

class GameScene : public Scene,
				  public IChitListener
{
	typedef Scene super;
public:
	GameScene( LumosGame* game );
	virtual ~GameScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );
	void MoveCamera(float dx, float dy);

	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
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
	virtual Engine* GetEngine();

private:
	void Tap3D(const grinliz::Vector2F& view, const grinliz::Ray& world);
	void Load();
	void DoDestTapped( const grinliz::Vector2F& grid );

	void TapModel( Chit* chit );
	void MoveModel( Chit* chit );
	void ClearTargetFlags();
	void SetBars(Chit* chit, bool isAvatar);

//	void SetPickupButtons();	// if the avatar can pick things up
	void ProcessNewsToConsole();
	void CheckGameStage(U32 delta);
	void ForceHerd(const grinliz::Vector2I& sector);
	bool AvatarSelected();
	bool CameraTrackingAvatar();
	bool DragBuildArea(gamui::RenderAtom* atom);
	bool StartDragPlanLocation(const grinliz::Vector2I& at, WorkItem* workItem);
	bool StartDragPlanRotation(const grinliz::Vector2I& at, WorkItem* workITem);
	bool StartDragCircuit(const grinliz::Vector2I& at);
	bool DragRotate(const grinliz::Vector2I& pos2i);
	void BuildAction(const grinliz::Vector2I& pos2i);
	void DragRotateBuilding(const grinliz::Vector2F& drag);	// rotate based on the mapDragStart and current location
	void DragRotatePlan(const grinliz::Vector2F& drag, WorkItem* workItem);
	void DrawBuildMarks(const WorkItem& workItem);
	void ControlTap(int slot, const grinliz::Vector2I& pos);
	void SetSquadDisplay(bool squadVisible);
	void OpenEndGame();
	void SetSelectionModel(const grinliz::Vector2F& view);

	void DoCameraHome();
	void DoAvatarButton();
	void DoCameraToggle();

	Chit* GetPlayerChit();	// wraps up the call to account for being attached to any domain.
	int GetPlayerChitID();
	CoreScript* GetHomeCore();
	grinliz::Vector2I GetHomeSector();

	enum {
		NUM_PICKUP_BUTTONS = 8,
		
		NUM_NEWS_BUTTONS = 12,
		NEWS_BUTTON_WIDTH  = 60,
		NEWS_BUTTON_HEIGHT = 25,
		NUM_BUILD_MARKS = 100
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

	enum class EDragMode
	{
		NONE,
		CIRCUIT,
		BUILDING_ROTATION,
		PLAN_AREA,
		PLAN_MOVE,
		PLAN_ROTATION
	};

	// returns the name from the build button
	grinliz::IString StructureInfo( int buildButtonIndex, int* size );

	LumosGame*			lumosGame;
	Sim*				sim;
	int					endTimer;
	bool				paused;
	grinliz::Vector2I	attached;
	int					targetChit;
	int					possibleChit;
	int					infoID;
	grinliz::Vector2I	voxelInfoID;
	int					chitTracking;
	int					coreWarningTimer, domainWarningTimer;
	grinliz::Vector2F	coreWarningPos, domainWarningPos;
	int					poolView;
	float				savedCameraHeight;
	EDragMode			dragMode;
	WorkItem			dragWorkItem;
	grinliz::Quaternion	savedCameraRotation;
	grinliz::Vector2F	mapDragStart;
	grinliz::Vector2F	tapView, tapDown;
	Adviser*			adviser;
	TutorialWidget*		tutorial;

	// Shows what is being built or removed.
	gamui::Image		selectionTile;
	GameSceneMenu*		menu;

	gamui::PushButton	okay;
	gamui::PushButton	saveButton;
	gamui::PushButton	allRockButton;
	gamui::PushButton	censusButton, viewButton, pauseButton;
	gamui::PushButton	newsButton[NUM_NEWS_BUTTONS];
	gamui::Image		minimap;
	gamui::Image		playerMark;
	gamui::Image		buildMark[NUM_BUILD_MARKS];	// used for click&drag building
	gamui::PushButton	coreWarningIcon;
	gamui::PushButton	domainWarningIcon;
	gamui::PushButton	atlasButton;
	gamui::Image		helpImage;
	gamui::TextLabel	helpText;
	gamui::PushButton	abandonButton, abandonConfirmButton;
	gamui::TextLabel	pausedLabel;

	BarStackWidget		summaryBars;

	FacePushWidget		faceWidget,
						targetFaceWidget;

	gamui::TextLabel	dateLabel;
	gamui::TextLabel	techLabel;
	MoneyWidget			moneyWidget;
	NewsConsole			newsConsole;
	StartGameWidget		startGameWidget;
	EndGameWidget		endGameWidget;

	//gamui::PushButton	pickupButton[NUM_PICKUP_BUTTONS];

	grinliz::CDynArray< Chit* >			chitQuery;
	grinliz::CDynArray< PickupData >	pickupData;
};


#endif // LUMOS_GAME_SCENE_INCLUDED
