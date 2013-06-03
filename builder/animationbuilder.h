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

#ifndef ANIMATION_BUILDER_INCLUDED
#define ANIMATION_BUILDER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

#include "../tinyxml2/tinyxml2.h"

#include "../shared/gamedbwriter.h"

#include "../engine/enginelimits.h"

void ProcessAnimation( const tinyxml2::XMLElement* element, gamedb::WItem* witem );

class SCMLParser
{
public:
	SCMLParser() {}
	~SCMLParser() {}

	void Parse( const tinyxml2::XMLElement* element,	// element in the lumos XML file
				const tinyxml2::XMLDocument* doc,		// the parsed SCML file
				gamedb::WItem* witem,					// base to create database entries from
				float pur );							// pixel-unit ratio

private:

	struct PartXForm {
		// in Spriter coordinates:
		int		id;
		float	x;
		float	y;
		float	angle;
		int		spin;
	};

	struct Frame {
		int					time;
		PartXForm			xforms[EL_MAX_BONES];
	};
	
	struct Animation {
		grinliz::GLString	name;
		int					id;
		U32					length;
		int					nFrames;
		grinliz::GLString	metaName;
		int					metaTime;

		Frame frames[EL_MAX_ANIM_FRAMES];

	};
	void ReadPartNames( const tinyxml2::XMLDocument* doc );
	void ReadAnimations( const tinyxml2::XMLDocument* doc );
	void ReadAnimation( const tinyxml2::XMLDocument* doc, Animation* a );
	void WriteAnimation( gamedb::WItem* witem, const Animation& a, const Animation* reference, float pur );

	const tinyxml2::XMLElement* GetAnimationEle( const tinyxml2::XMLDocument* doc, const grinliz::GLString& name );
	const tinyxml2::XMLElement* GetObjectEle(	const tinyxml2::XMLElement* animationEle,
												int timeline,
												int partID );

	grinliz::HashTable< int, grinliz::GLString > partIDNameMap;

	grinliz::CDynArray< Animation > animationArr;
};

#endif // ANIMATION_BUILDER_INCLUDED