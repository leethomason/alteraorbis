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
#include "../game/lumoschitbag.h"
#include "spatialcomponent.h"
#include "../game/gridmovecomponent.h"
#include "../engine/camera.h"
#include "../engine/engine.h"

using namespace grinliz;


void CameraComponent::OnAdd( Chit* c, bool init )
{
	super::OnAdd(c, init);
}


void CameraComponent::OnRemove()
{
	super::OnRemove();
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
	str->AppendFormat( "[Camera] mode=" );
	switch( mode ) {
		case DONE:	str->Format( "done " ); break;
		case PAN:	str->Format( "pan " );	break;
		default:	GLASSERT( 0 ); break;
	}
}

	
void CameraComponent::SetPanTo( const grinliz::Vector2F& _dest, float _speed )
{
	GLOUTPUT(( "CC Pan set\n" ));
	mode = PAN;
	
	Vector3F at;
	Context()->engine->CameraLookingAt( &at );	
	dest = Context()->engine->camera.PosWC() + (ToWorld3F(_dest) - at);

	speed = _speed;
}


void CameraComponent::SetTrack( int targetID ) 
{
	if ( mode != TRACK || targetChitID != targetID ) {
		GLOUTPUT(( "CC Track set to %d\n", targetID ));
	}
	mode = TRACK;
	targetChitID = targetID;
}


int CameraComponent::DoTick(U32 delta)
{
	if (Context()->chitBag->GetCameraChitID() != parentChit->ID()) {
		parentChit->QueueDelete();
		return VERY_LONG_TICK;
	}

	float EASE = 0.2f;

	switch (mode) {

		case PAN:
		{
			/*
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
				*/

			Vector3F d = (dest - camera->PosWC());
			Vector3F c = camera->PosWC();

			if (d.Length() < 0.01f) {
				camera->SetPosWC(dest.x, dest.y, dest.z);
				mode = DONE;
			}
			else {
				camera->SetPosWC(c.x + EASE*d.x, c.y + EASE*d.y, c.z + EASE*d.z);
			}
		}
		break;

		case TRACK:
		{
			Chit* chit = Context()->chitBag->GetChit(targetChitID);
			if (chit && chit->GetSpatialComponent()) {

				Vector3F pos = chit->GetSpatialComponent()->GetPosition();
				pos.y = 0;

				// Scoot the camera to always focus on the target. Removes
				// errors that occur from rotation, drift, etc.
				const Vector3F* eye3 = camera->EyeDir3();
				Vector3F at;
				int result = IntersectRayPlane(camera->PosWC(), eye3[0], 1, 0.0f, &at);
				if (result == INTERSECT) {
					Vector3F t = (camera->PosWC() - at);

					//camera->SetPosWC( pos + t );
					Vector3F c = camera->PosWC();
					Vector3F d = (pos + t) - c;

					// If grid moving, the EASE contributes to jitter.
					if (GET_SUB_COMPONENT(chit, MoveComponent, GridMoveComponent)) {
						EASE = 1.0f;
					}
					camera->SetPosWC(c.x + EASE*d.x, pos.y + t.y, c.z + EASE*d.z);
				}
			}
		}
		break;

		case DONE:
		break;

		default:
		GLASSERT(0);
	}
	return 0;
}



