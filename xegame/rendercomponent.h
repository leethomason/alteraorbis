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
	// spacetree probably  sufficient, but 'engine' easier to keep track of
	RenderComponent( Engine* engine, const char* asset, int flags );
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
	int GetFlags() const { return flags; }

private:
	Engine* engine;
	const ModelResource* resource;
	Model* model;
	int flags;
};

#endif // RENDER_COMPONENT_INCLUDED