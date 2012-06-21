#ifndef ANIMATION_SCENE_INCLUDED
#define ANIMATION_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../tinyxml2/tinyxml2.h"

class LumosGame;
class Engine;
class Model;

class AnimationScene : public Scene
{
public:
	AnimationScene( LumosGame* game );
	virtual ~AnimationScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );

private:
	void UpdateBoneInfo();
	void InitXML( const grinliz::Rectangle2I& bounds );
	void FinishXML();

	void InitFrame();
	void PushSprite( const char* name, const grinliz::Rectangle2I& bounds );
	void FinishFrame();

	Engine* engine;
	Model* model;

	int  currentBone;
	bool doExport;
	int  exportCount;

	gamui::PushButton okay;
	gamui::PushButton boneLeft, boneRight, exportSCML;
	gamui::ToggleButton ortho;
	gamui::TextLabel boneName;
	gamui::TextLabel pixelUnitRatio;

	tinyxml2::XMLDocument* xmlDocument;
	tinyxml2::XMLPrinter*  xmlPrinter;
	FILE* scmlFP;
	grinliz::Vector2I origin;
};



#endif // ANIMATION_SCENE_INCLUDED