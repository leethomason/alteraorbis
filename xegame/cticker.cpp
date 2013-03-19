#include "cticker.h"
#include "../xarchive/glstreamer.h"

void CTicker::Serialize( XStream* xs, const char* name )
{
	XarcOpen( xs, name );
	XARC_SER( xs, period );
	XARC_SER( xs, time );
	XarcClose( xs );
}