#ifndef CENSUS_SCENE_INCLUDED
#define CENSUS_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;
class ItemComponent;
class ChitBag;


class CensusSceneData : public SceneData
{
public:
	CensusSceneData( ChitBag* _chitBag )				// apply markup, costs
		: SceneData(), chitBag(_chitBag) { GLASSERT(chitBag); }

	ChitBag* chitBag;
};


class CensusScene : public Scene
{
public:
	CensusScene( LumosGame* game, CensusSceneData* data );
	virtual ~CensusScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

private:
	void Scan();

	LumosGame*			lumosGame;
	ChitBag*			chitBag;
	gamui::PushButton	okay;
	gamui::TextLabel	text;
};

#endif // CENSUS_SCENE_INCLUDED

