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
	if ( GetChitBag()->ActiveCamera() == 0 ) {
		GetChitBag()->SetActiveCamera( this->ID() );
	}
}


void CameraComponent::OnRemove()
{
	if ( GetChitBag()->ActiveCamera() == this->ID() ) {
		GetChitBag()->SetActiveCamera( 0 );
	}
}


void CameraComponent::Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele )
{
	XE_ARCHIVE( autoDelete );
	XE_ARCHIVE( mode );
	XE_ARCHIVE( targetChitID );
	XE_ARCHIVE( speed );
	XEArchive( prn, ele, "dest", dest );
}


void CameraComponent::Load( const tinyxml2::XMLElement* element )
{
	this->BeginLoad( element, "CameraComponent" );
	Archive( 0, element );
	this->EndLoad( element );
}

void CameraComponent::Save( tinyxml2::XMLPrinter* printer )
{
	this->BeginSave( printer, "CameraComponent" );
	Archive( printer, 0 );
	this->EndSave( printer );
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


bool CameraComponent::DoTick( U32 delta ) 
{
	if ( GetChitBag()->ActiveCamera() != this->ID() ) {
		return true;
	}

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
					Vector3F delta = camera->PosWC() - at;
					camera->SetPosWC( pos + delta );
				}
			}
		}
		break;

	default:
		GLASSERT( 0 );
	}
	return true;
}



