#include "aicomponent.h"

AIComponent::AIComponent( int _team )
{
	team = _team;
}


AIComponent::~AIComponent()
{
}


void AIComponent::DoTick( U32 delta )
{

}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[AI] " );
}

