#ifndef RENDER_COMPONENT_INCLUDED
#define RENDER_COMPONENT_INCLUDED

#include "component.h"
#include "../xegame/xegamelimits.h"

class Engine;
class ModelResource;
class Model;

class RenderComponent : public Component
{
public:
	RenderComponent( Engine* engine, const char* asset );	// spacetree probably  sufficient, but 'engine' easier to keep track of
	virtual ~RenderComponent();

	// ------ Component functionality: -------
	virtual const char* Name() const { return "RenderComponent"; };

	virtual RenderComponent*	ToRender()		{ return this; }

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void DoUpdate();

	// ------ Additional --------
	// Radius of the "base" the model stands on. Used to position
	// the model so it doesn't walk into walls or other models.
	float RadiusOfBase();
	//void SetModelFlag( int flag );

private:
	Engine* engine;
	const ModelResource* resource;
	Model* model;
};

#endif // RENDER_COMPONENT_INCLUDED