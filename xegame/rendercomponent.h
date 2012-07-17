#ifndef RENDER_COMPONENT_INCLUDED
#define RENDER_COMPONENT_INCLUDED

#include "component.h"
#include "../engine/enginelimits.h"
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

	virtual bool NeedsTick()	{ return true; }
	virtual void DoTick( U32 deltaTime );
	virtual void OnChitMsg( Chit* chit, int id, const ChitEvent* event );

	// ------ Additional --------
	// Radius of the "base" the model stands on. Used to position
	// the model so it doesn't walk into walls or other models.
	float RadiusOfBase();
	int   GetFlags() const { return flags; }
	bool  GetMetaData( const char* name, grinliz::Matrix4* xform );

	void Attach( const char* metaData, const char* asset );
	void Detach( const char* metaData );

private:
	const char* GetAnimationName() const;
	SpatialComponent* SyncToSpatial();	// this a scary function: location is stored in both the model and the spatialcomponent

	Engine* engine;
	enum { NUM_MODELS = EL_MAX_METADATA+1 };	// slot[0] is the main model; others are hardpoint attach
	const ModelResource* resource[ NUM_MODELS ];
	Model* model[ NUM_MODELS ];
	grinliz::CStr<EL_RES_NAME_LEN> metaDataName[EL_MAX_METADATA];
	int flags;
};

#endif // RENDER_COMPONENT_INCLUDED