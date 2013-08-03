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

#include "vertex.h"
#include "../grinliz/glstringutil.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

void BoneData::Bone::ToMatrix( grinliz::Matrix4* mat ) const
{
	mat->SetIdentity();
	rot.ToMatrix( mat );
	mat->SetTranslation( pos );
}


void BoneData::Serialize( XStream* xs )
{
	XarcOpen( xs, "BoneData" );
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		XarcOpen( xs, "bone" );
		XARC_SER_KEY( xs, "name", bone[i].name );
		XARC_SER_KEY( xs, "rot", bone[i].rot );
		XARC_SER_KEY( xs, "pos", bone[i].pos );
		XarcClose( xs );
	}
	XarcClose( xs );
}



