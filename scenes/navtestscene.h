#ifndef NAVTESTSCENE_INCLUDED
#define NAVTESTSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../grinliz/glrandom.h"
#include "../xegame/chitbag.h"

class LumosGame;
class Engine;
class WorldMap;

class NavTestScene : public Scene, public IChitListener
{
public:
	NavTestScene( LumosGame* game );
	~NavTestScene();

	virtual int RenderPass( grinliz::Rectangle2I* clip3D, grinliz::Rectangle2I* clip2D ) { return RENDER_2D | RENDER_3D; }
	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );

	virtual void OnChitMsg( Chit* chit, int id );

	virtual void DrawDebugText();
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world );

private:
	gamui::PushButton okay,
					  block,
					  block20;
	gamui::TextLabel  textLabel;
	gamui::ToggleButton showAdjacent, 
		                showZonePath,
						showOverlay,
						toggleBlock;

	Engine* engine;
	WorldMap* map;
	grinliz::Random random;
	grinliz::Vector3F tapMark;
	grinliz::Ray debugRay;

	enum { NUM_CHITS = 5 };
	Chit* chit[NUM_CHITS];

	ChitBag chitBag;
};


#endif // NAVTESTSCENE_INCLUDED