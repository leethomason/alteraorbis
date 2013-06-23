#ifndef LUMOS_GAME_SCENE_INCLUDED
#define LUMOS_GAME_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../xegame/chitevent.h"

class LumosGame;
class Sim;
struct NewsEvent;
class Chit;

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

private:
	void Save();
	void Load();
	void SetFace();
	void SetBars();
	void DoDestTapped( const grinliz::Vector2F& grid );

	void TapModel( Chit* chit );
	void MoveModel( Chit* chit );
	void ClearTargetFlags();
	bool FreeCamera() const;

	enum {
		SAVE,
		LOAD,
		CYCLE,
		NUM_SERIAL_BUTTONS,
		
		NUM_NEWS_BUTTONS = 16,
		NEWS_BUTTON_WIDTH  = 60,
		NEWS_BUTTON_HEIGHT = 25
	};
	enum {
		NO_BUILD,
		CLEAR_ROCK,
		BUILD_ICE,
		BUILD_KIOSK,
		NUM_BUILD_BUTTONS
	};

	LumosGame*	lumosGame;
	Sim*		sim;
	bool		fastMode;
	int			simTimer;		// used to count sim ticks/second
	int			simCount;
	float		simPS;

	int			targetChit;
	int			possibleChit;
	int					infoID;
	grinliz::Vector2I	voxelInfoID;

	gamui::PushButton	okay;
	gamui::PushButton	serialButton[NUM_SERIAL_BUTTONS];
	gamui::ToggleButton freeCameraButton;
	gamui::ToggleButton	buildButton[NUM_BUILD_BUTTONS];
	gamui::PushButton	createWorkerButton;
	gamui::PushButton	allRockButton;
	gamui::PushButton	newsButton[NUM_NEWS_BUTTONS];
	gamui::PushButton	clearButton;
	gamui::Image		minimap;
	gamui::Image		playerMark;

	gamui::Image		faceImage;
	gamui::DigitalBar	healthBar, ammoBar, shieldBar;

	gamui::TextLabel	dateLabel;
	gamui::TextLabel	goldLabel;
	gamui::TextLabel	xpLabel;
};


#endif // LUMOS_GAME_SCENE_INCLUDED
