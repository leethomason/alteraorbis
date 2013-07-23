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

#ifndef SPACIAL_COMPONENT_INCLUDED
#define SPACIAL_COMPONENT_INCLUDED

#include "component.h"
#include "chit.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glgeometry.h"

#include "../engine/enginelimits.h"
#include "../game/gamelimits.h"

class MapSpatialComponent;

/* Chits with a SpatialComponent are in the map world as there own entity. (Hardpoint
   items do not have SpatialComponents.)
*/
class SpatialComponent : public Component
{
private:
	typedef Component super;
public:
	SpatialComponent() {
		position.Zero();

		static const grinliz::Vector3F UP = { 0, 1, 0 };
		rotation.FromAxisAngle( UP, 0 );
	}

	virtual const char* Name() const { return "SpatialComponent"; }
	virtual SpatialComponent*		ToSpatialComponent()			{ return this; }
	virtual MapSpatialComponent*	ToMapSpatialComponent()			{ return 0; }

	virtual void DebugStr( grinliz::GLString* str );

	virtual void Serialize( XStream* );

	virtual void OnAdd( Chit* chit );
	virtual void OnRemove();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );

	// Position and rotation are absolute (post-transform)
	void SetPosition( float x, float y, float z )	{ grinliz::Vector3F v = { x, y, z }; SetPosition( v ); }
	void SetPosition( const grinliz::Vector3F& v )	{ SetPosRot( v, rotation ); }
	const grinliz::Vector3F& GetPosition() const	{ return position; }

	// yRot=0 is the +z axis
	void SetYRotation( float yDegrees )				{ SetPosYRot( position, yDegrees ); }
	float GetYRotation() const;
	const grinliz::Quaternion& GetRotation() const	{ return rotation; }

	void SetPosYRot( float x, float y, float z, float yRot );
	void SetPosYRot( const grinliz::Vector3F& v, float yRot ) { SetPosYRot( v.x, v.y, v.z, yRot ); }
	void SetPosRot( const grinliz::Vector3F& v, const grinliz::Quaternion& quat );

	grinliz::Vector3F GetHeading() const;

	grinliz::Vector2F GetPosition2D() const			{ grinliz::Vector2F v = { position.x, position.z }; return v; }
	grinliz::Vector2I GetPosition2DI() const		{ grinliz::Vector2I v = { (int)position.x, (int)position.z }; return v; }
	grinliz::Vector2F GetHeading2D() const;
	grinliz::Vector2I GetSector() const				{ grinliz::Vector2I s = { (int)position.x/SECTOR_SIZE, (int)position.z/SECTOR_SIZE }; return s; }

protected:
	grinliz::Vector3F	position;
	grinliz::Quaternion	rotation;
};


#endif // SPACIAL_COMPONENT_INCLUDED
