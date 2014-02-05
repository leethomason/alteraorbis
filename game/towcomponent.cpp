#include "towcomponent.h"

void TowComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
}


void TowComponent::OnRemove()
{
	super::OnRemove();
}


void TowComponent::Serialize( XStream* xs )
{
	BeginSerialize( xs, Name() );
	XARC_SER( xs, towingID );
	EndSerialize( xs );
}


void TowComponent::DebugStr( grinliz::GLString* str )
{
	str->AppendFormat( "[Tow] id=%d ", towingID );
}


int TowComponent::DoTick( U32 delta )
{
	return VERY_LONG_TICK;
}

