#ifndef VISITOR_STATE_COMPONENT_INCLUDED
#define VISITOR_STATE_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../gamui/gamui.h"
#include "visitor.h"

class WorldMap;

class VisitorStateComponent : public Component
{
private:
	typedef Component super;

public:
	VisitorStateComponent( WorldMap* map );
	~VisitorStateComponent();

	virtual const char* Name() const { return "VisitorStateComponent"; }

	virtual void Serialize( XStream* xs );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual int DoTick( U32 delta, U32 since );
	void OnChitMsg( Chit* chit, const ChitMsg& msg );

private:
	WorldMap* worldMap;
	gamui::Image		wants[VisitorData::NUM_VISITS];
	gamui::DigitalBar	bar;	// FIXME: this is a pretty general need to have here. Probably should be head decos.
};


#endif // VISITOR_STATE_COMPONENT_INCLUDED

