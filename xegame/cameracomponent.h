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

#ifndef CAMERA_COMPONENT_INCLUDED
#define CAMERA_COMPONENT_INCLUDED

#include "component.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"

class Camera;

// Also needs other modes.
class CameraComponent : public Component
{
private:
	typedef Component super;
public:
	CameraComponent( Camera* _camera ) 
		:	camera( _camera ),
			autoDelete( false ),
			mode( DONE ),
			speed( 0 )
	{
		dest.Zero();
	}

	virtual Component* ToComponent( const char* name ) {
		if ( grinliz::StrEqual( name, "CameraComponent" ) ) return this;
		return super::ToComponent( name );
	}

	virtual void Load( const tinyxml2::XMLElement* element );
	virtual void Save( tinyxml2::XMLPrinter* printer );

	virtual void DebugStr( grinliz::GLString* str );
	virtual bool DoTick( U32 delta );

	void SetAutoDelete( bool ad )									{ autoDelete = ad; }
	void SetPanTo( grinliz::Vector3F& dest, float speed = 40.0f );
	void SetTrack( int targetChitID );

private:
	void Archive( tinyxml2::XMLPrinter* prn, const tinyxml2::XMLElement* ele );
	
	Camera* camera;
	bool autoDelete;

	enum {
		DONE,	// will delete
		PAN,
		TRACK
	};
	int					mode;
	grinliz::Vector3F	dest;
	int					targetChitID;
	float				speed;
};

#endif // CAMERA_COMPONENT_INCLUDED