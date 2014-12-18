#ifndef HP_BAR_INCLUDED
#define HP_BAR_INCLUDED

#include "../gamui/gamui.h"

class GameItem;
class Shield;
class ItemComponent;

class HPBar : public gamui::DigitalBar
{
public:
	HPBar()	{}
	~HPBar()	{}

	void Init(gamui::Gamui* gamui);
	void Set(const GameItem* item, const Shield* shield, const char* optionalName, bool melee, bool ranged);
	void Set(ItemComponent* ic);
};

#endif // HP_BAR_INCLUDED
