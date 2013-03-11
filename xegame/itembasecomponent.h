#ifndef ITEM_BASE_COMPONENT_INCLUDED
#define ITEM_BASE_COMPONENT_INCLUDED

class GameItem;
class Engine;

#include "component.h"

#if 0
class ItemBaseComponent : public Component
{
public:
	ItemBaseComponent( Engine* _engine ) : engine( _engine ) {}

protected:
	// Returns true if effect emitted.
	bool LowerEmitEffect( Engine* engine, const GameItem& item, U32 deltaTime );

	Engine* engine;
};
#endif


#endif // ITEM_BASE_COMPONENT_INCLUDED