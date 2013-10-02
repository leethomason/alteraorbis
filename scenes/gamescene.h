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

class LumosGame;
class Sim;
struct NewsEvent;
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

	virtual void SceneResult( int sceneID, int result );

private:
	void Save();
	void Load();
	void SetFace();
	void SetBars();
	void DoDestTapped( const grinliz::Vector2F& grid );

	void TapModel( Chit* chit );
	void MoveModel( Chit* chit );
	void ClearTargetFlags();
	void SetSelectionModel( const grinliz::Vector2F& view );

	void SetPickupButtons();

	enum {
		SAVE,
		LOAD,
		CYCLE,
		NUM_SERIAL_BUTTONS,
		NUM_PICKUP_BUTTONS = 8,
		
		NUM_NEWS_BUTTONS = 16,
		NEWS_BUTTON_WIDTH  = 60,
		NEWS_BUTTON_HEIGHT = 25
	};
	enum {
		BUILD_XFORM,
		BUILD_BASIC,
		BULID_BASIC_2,
		NUM_BUILD_MODES
	};
	enum {
		NO_BUILD,
		CLEAR_ROCK,
		PAVE,
		ROTATE,
		BUILD_ICE,
		BUILD_KIOSK_N,
		BUILD_KIOSK_M,
		BUILD_KIOSK_C,
		BUILD_KIOSK_S,
		BUILD_VAULT,
		NUM_BUILD_BUTTONS
	};

	struct PickupData {
		int		chitID;
		float	distance;	// pather distance

		bool operator<( const PickupData& rhs ) const { return distance < rhs.distance; }
	};

	static const int BUILD_MODE_START[NUM_BUILD_MODES];

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
	grinliz::IString	BuildActiveInfo( int* size );

	// Shows what is being built or removed.
	Model*				selectionModel;

	gamui::PushButton	okay;
	gamui::PushButton	serialButton[NUM_SERIAL_BUTTONS];
	gamui::ToggleButton freeCameraButton;
	gamui::ToggleButton	buildButton[NUM_BUILD_BUTTONS];
	gamui::ToggleButton modeButton[NUM_BUILD_MODES];
	gamui::Image		tabBar;
	gamui::PushButton	createWorkerButton;
	gamui::PushButton	ejectButton;
	gamui::PushButton	allRockButton;
	gamui::PushButton	newsButton[NUM_NEWS_BUTTONS];
	gamui::PushButton	clearButton;
	gamui::Image		minimap;
	gamui::Image		playerMark;

	gamui::PushButton	faceButton;
	gamui::DigitalBar	healthBar, ammoBar, shieldBar;

	gamui::TextLabel	dateLabel;
	gamui::TextLabel	goldLabel;
	gamui::TextLabel	xpLabel;

	gamui::PushButton	pickupButton[NUM_PICKUP_BUTTONS];

	grinliz::CDynArray<const GameItem*> dropList;	// stuff dropped in the character scene
	grinliz::CDynArray< Chit* >			chitQuery;
	grinliz::CDynArray< PickupData >	pickupData;
};


#endif // LUMOS_GAME_SCENE_INCLUDED
