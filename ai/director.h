#ifndef DIRECTOR_INCLUDED
#define DIRECTOR_INCLUDED

#include "../xegame/component.h"
#include "../xegame/chit.h"

class Director : public Component, public IChitListener
{
private:
	typedef Component super;

public:

	virtual const char* Name() const { return "Director"; }

	virtual void DebugStr(grinliz::GLString* str)	{ str->AppendFormat("[Director] "); }

	virtual void Serialize( XStream* );
	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );

	grinliz::Vector2I ShouldSendHerd(Chit* herd);

	//void NotifyArrive(from, to, chit);
	//void NotifyDelete(at, chit);

	//sendTo ShouldSendGreater(from, greater);

};

#endif // DIRECTOR_INCLUDED
