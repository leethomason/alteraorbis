#ifndef RENDER_COMPONENT_INCLUDED
#define RENDER_COMPONENT_INCLUDED

#include "component.h"
#include "../xegame/xegamelimits.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"

class Engine;
class ModelResource;
class Model;

class RenderComponent : public Component
{
public:
	// spacetree probably  sufficient, but 'engine' easier to keep track of
	// flags: MODEL_USER_AVOIDS
	RenderComponent( Engine* engine, const char* asset, int modelFlags );
	virtual ~RenderComponent();

	// ------ Component functionality: -------
	virtual Component*          ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "RenderComponent" ) ) return this;
		return Component::ToComponent( name );
	}
	virtual RenderComponent*	ToRender()		{ return this; }
	virtual void DebugStr( grinliz::GLString* str );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();

	virtual void DoUpdate();
	virtual void OnChitMsg( Chit* chit, int id, const ChitEvent* event );

	// ------ Additional --------
	// Radius of the "base" the model stands on. Used to position
	// the model so it doesn't walk into walls or other models.
	float RadiusOfBase();
	int   GetFlags() const { return flags; }
	bool  GetMetaData( const char* name, grinliz::Matrix4* xform );

private:
	Engine* engine;
	const ModelResource* resource;
	Model* model;
	int flags;
};

#endif // RENDER_COMPONENT_INCLUDED