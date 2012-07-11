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
	void LoadModel( const char* name );
	void SetModelVis( bool onlyShowOne );
	void UpdateBoneInfo();
	void UpdateAnimationInfo();

	void InitXML( const grinliz::Rectangle2I& bounds );
	void FinishXML();

	void InitFrame();
	void PushSprite( const char* name, const grinliz::Rectangle2I& bounds );
	void FinishFrame();

	Engine* engine;
	enum { NUM_MODELS = 3 };
	Model* model[ NUM_MODELS ];

	int  currentBone;
	int  currentAnim;
	bool doExport;
	int  exportCount;

	gamui::PushButton okay;
	gamui::PushButton boneLeft, boneRight;
	gamui::PushButton animLeft, animRight;
	gamui::PushButton exportSCML;

	gamui::ToggleButton ortho;
	gamui::ToggleButton instance;
	gamui::ToggleButton particle;

	gamui::TextLabel boneName, animName;
	gamui::TextLabel pixelUnitRatio;

	tinyxml2::XMLDocument* xmlDocument;
	tinyxml2::XMLPrinter*  xmlPrinter;
	FILE* scmlFP;
	grinliz::Vector2I origin;
};



#endif // ANIMATION_SCENE_INCLUDED