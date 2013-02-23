/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cameracomponent.h"
#include "chit.h"
#include "chitbag.h"
#include "spatialcomponent.h"
#include "../engine/camera.h"
#include "../engine/engine.h"

using namespace grinliz;


void CameraComponent::OnAdd( Chit* c )
{
	super::OnAdd( c );
}


void CameraComponent::OnRemove()
{
}

void CameraComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	XARC_SER( xs, mode );
	XARC_SER( xs, targetChitID );
	XARC_SER( xs, speed );
	XARC_SER( xs, dest );
	this->EndSerialize( xs );
}


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


void CameraComponent::SetTrack( int targetID ) 
{
	mode = TRACK;
	targetChitID = targetID;
}


int CameraComponent::DoTick( U32 delta, U32 since ) 
{
	if ( GetChitBag()->GetCameraChitID() != parentChit->ID() ) {
		parentChit->QueueDelete();
		return VERY_LONG_TICK;
	}

	switch ( mode ) {

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

	case TRACK:
		{
			Chit* chit = this->GetChitBag()->GetChit( targetChitID );
			if ( chit && chit->GetSpatialComponent() ) {
				
				Vector3F pos = chit->GetSpatialComponent()->GetPosition();
				pos.y = 0;

				// Scoot the camera to always focus on the target. Removes
				// errors that occur from rotation, drift, etc.
				const Vector3F* eye3 = camera->EyeDir3();
				Vector3F at;
				int result = IntersectRayPlane( camera->PosWC(), eye3[0], 1, 0.0f, &at );
				if ( result == INTERSECT ) {
					Vector3F t = (camera->PosWC() - at);
					
					//camera->SetPosWC( pos + t );
					Vector3F c = camera->PosWC();
					Vector3F d = (pos+t) - c;
					static const float EASE = 0.5f;
					camera->SetPosWC( c.x+EASE*d.x, pos.y+t.y, c.z+EASE*d.z );
				}
			}
		}
		break;

	default:
		GLASSERT( 0 );
	}
	return 0;
}



