#ifndef ISTRING_CONST_INCLUDED
#define ISTRING_CONST_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../game/gamelimits.h"

class IStringConst
{
public:
	static void Init();

	static grinliz::IString ktrigger;
	static grinliz::IString ktarget;
	static grinliz::IString kalthand;
	static grinliz::IString khead;
	static grinliz::IString kshield;

	static int Hardpoint( grinliz::IString str ) {
		if		( str == ktrigger ) return HARDPOINT_TRIGGER;
		else if ( str == kalthand ) return HARDPOINT_ALTHAND;
		else if ( str == khead )	return HARDPOINT_HEAD;
		else if ( str == kshield )	return HARDPOINT_SHIELD;
		GLASSERT( 0 );
		return 0;
	}
	static grinliz::IString Hardpoint( int i ) {
		switch ( i ) {
		case HARDPOINT_TRIGGER:	return ktrigger;
		case HARDPOINT_ALTHAND: return kalthand;
		case HARDPOINT_HEAD:	return khead;
		case HARDPOINT_SHIELD:	return kshield;
		default:
			GLASSERT( 0 );
			return grinliz::IString();
		}
	}
};

#endif // ISTRING_CONST_INCLUDED