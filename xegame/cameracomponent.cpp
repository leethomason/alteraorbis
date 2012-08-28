#include "cameracomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "../engine/camera.h"

using namespace grinliz;

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


bool CameraComponent::DoTick( U32 delta ) 
{
	switch ( mode ) {
	case DONE:
		if ( autoDelete ) {
			parentChit->GetChitBag()->QueueDelete( parentChit );
		}
		break;
		
	case PAN:
		{
			float travel = Travel( speed, delta );
			Vector3F toDest = ( dest - camera->PosWC() );
			float lenToDest = toDest.Length();

			if ( lenToDest <= travel ) {
				camera->SetPosWC( dest );
				mode = DONE;
			}
			else {
				toDest.Normalize();
				Vector3F v = toDest * travel;
				camera->DeltaPosWC( v.x, v.y, v.z );
			}
		}
		break;

	default:
		GLASSERT( 0 );
	}
	return true;
}



