#include "cameracomponent.h"


void CameraComponent::DebugStr( grinliz::GLString* str )
{
	str->Format( "[Camera] mode=" );
	switch( mode ) {
		case DONE:	str->Format( "done " ); break;
		case PAN:	str->Format( "pan " );	break;
		default:	GLASSERT( 0 ); break;
	}
}

	
void CameraComponent::SetPanTo( grinliz::Vector3F& _dest, float _speed )
{
	mode = PAN;
	dest = _dest;
	speed = _speed;
}

