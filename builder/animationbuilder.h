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

void ProcessAnimation( const tinyxml2::XMLElement* element, gamedb::WItem* witem );

class SCMLParser
{
public:
	SCMLParser() {}
	~SCMLParser() {}

	void Parse( const tinyxml2::XMLDocument* doc, gamedb::WItem* witem );

private:
	void ReadPartNames( const tinyxml2::XMLDocument* doc );
	void ReadAnimations( const tinyxml2::XMLDocument* doc );
	void ReadAnimation( const tinyxml2::XMLDocument* doc, gamedb::WItem* witem, const Animation& a );

	grinliz::HashTable< int, grinliz::GLString > partIDNameMap;
	struct Animation {
		grinliz::GLString name;
		int id;
		U32 length;
		int nFrames;
	};
	grinliz::CDynArray< Animation > animationArr;
};

#endif // ANIMATION_BUILDER_INCLUDED