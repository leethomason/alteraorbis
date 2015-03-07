#ifndef HP_BAR_INCLUDED
#define HP_BAR_INCLUDED

#include "../gamui/gamui.h"

class GameItem;
class Shield;
class RangedWeapon;
class MeleeWeapon;
class ItemComponent;

class HPBar : public gamui::DigitalBar
{
public:
	HPBar() : userItemID(0)	{}
	~HPBar()	{}

	void Init(gamui::Gamui* gamui);
	void Set(const GameItem* item, const MeleeWeapon* melee, const RangedWeapon* ranged, const Shield* shield, const char* optionalName);
	void Set(ItemComponent* ic);
	virtual void SetVisible(bool v);

	virtual bool DoLayout();

	gamui::Image hitBounds;
	int userItemID;

private:
	gamui::Image meleeIcon;
	gamui::Image rangedIcon;
};

#endif // HP_BAR_INCLUDED
