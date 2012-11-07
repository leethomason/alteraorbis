#ifndef ITEM_BASE_COMPONENT_INCLUDED
#define ITEM_BASE_COMPONENT_INCLUDED

class GameItem;
class Engine;

#include "component.h"


class ItemBaseComponent : public Component
{
protected:
	void LowerEmitEffect( Engine* engine, const GameItem& item, U32 deltaTime );
};

#endif // ITEM_BASE_COMPONENT_INCLUDED