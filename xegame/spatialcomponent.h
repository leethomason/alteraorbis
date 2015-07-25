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

class SpatialComponent : public Component
{
private:
	typedef Component super;
public:

	virtual const char* Name() const { return "SpatialComponent"; }
	virtual SpatialComponent*		ToSpatialComponent()			{ return this; }
	virtual MapSpatialComponent*	ToMapSpatialComponent()			{ return 0; }

	virtual void DebugStr( grinliz::GLString* str );

	virtual void Serialize( XStream* );
	virtual void OnAdd( Chit* chit, bool init );
	virtual void OnRemove();
	virtual void OnChitMsg( Chit* chit, const ChitMsg& msg );

	static void Teleport(Chit* chit, const grinliz::Vector3F& pos);

protected:
	SpatialComponent() {}
};


#endif // SPACIAL_COMPONENT_INCLUDED
