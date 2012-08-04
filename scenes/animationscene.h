#ifndef ANIMATION_SCENE_INCLUDED
#define ANIMATION_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glcontainer.h"

class LumosGame;
class Engine;
class Model;
class ModelResource;

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
	virtual void DoTick( U32 deltaTime );
	virtual void Draw3D( U32 deltaTime );

private:
	void LoadModel();
	void SetModelVis( bool onlyShowOne );
	void UpdateBoneInfo();
	void UpdateAnimationInfo();
	void UpdateModelInfo();

	void InitXML( const grinliz::Rectangle2I& bounds );
	void FinishXML();

	void InitFrame();
	void PushSprite( const char* name, const grinliz::Rectangle2I& bounds );
	void FinishFrame();

	Engine* engine;
	enum { NUM_MODELS = 3 };

	grinliz::CDynArray< const ModelResource* > resourceArr;
	Model* model[ NUM_MODELS ];
	Model* gunModel;

	int currentModel;
	int  currentBone;
	int  currentAnim;
	bool doExport;
	int  exportCount;

	gamui::PushButton okay;
	gamui::PushButton boneLeft, boneRight;
	gamui::PushButton animLeft, animRight;
	gamui::PushButton modelLeft, modelRight;
	gamui::PushButton exportSCML;

	gamui::ToggleButton ortho;
	gamui::ToggleButton instance;
	gamui::ToggleButton particle, gun;

	gamui::TextLabel boneName, animName, modelName;
	gamui::TextLabel pixelUnitRatio;

	tinyxml2::XMLDocument* xmlDocument;
	tinyxml2::XMLPrinter*  xmlPrinter;
	FILE* scmlFP;
	grinliz::Vector2I origin;
};



#endif // ANIMATION_SCENE_INCLUDED