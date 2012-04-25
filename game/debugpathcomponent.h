#ifndef DEBUG_PATH_COMPONENT
#define DEBUG_PATH_COMPONENT

#include "../xegame/component.h"

class Engine;
class Model;
class ModelResource;

class DebugPathComponent : public Component
{
public:
	DebugPathComponent( Engine* );
	~DebugPathComponent();

	virtual const char* Name() const			{ return "DebugPathComponent"; }
	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual bool NeedsTick()					{ return true; }
	virtual void DoTick( U32 delta );

private:
	Engine* engine;
	const ModelResource* resource;
	Model* model;
};

#endif // DEBUG_PATH_COMPONENT
