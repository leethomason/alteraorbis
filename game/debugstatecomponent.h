#ifndef DEBUG_STATE_COMPONENT
#define DEBUG_STATE_COMPONENT

#include "../xegame/component.h"
#include "../gamui/gamui.h"

class WorldMap;
class LumosGame;

class DebugStateComponent : public Component
{
public:
	DebugStateComponent( WorldMap* _map );
	~DebugStateComponent()	{}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "DebugStateComponent" ) ) return this;
		return Component::ToComponent( name );
	}

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual void OnChitMsg( Chit* chit, int id );

private:
	WorldMap* map;
	gamui::DigitalBar healthBar;
};

#endif // DEBUG_STATE_COMPONENT
