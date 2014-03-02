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


int BoneData::GetBoneIndex( const grinliz::IString& internedName ) const {
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		if ( bone[i].name == internedName ) {
			return i;
		}
	}
	GLASSERT( 0 );
	return 0;
}

const BoneData::Bone* BoneData::GetBone( const grinliz::IString& internedName ) const {
	return bone+GetBoneIndex( internedName );
}

void BoneData::Bone::Serialize( XStream* xs )
{
	XarcOpen( xs, "Bone" );
	XARC_SER( xs, name );
	XARC_SER( xs, parent );
	XARC_SER( xs, refPos );
	XARC_SER( xs, refConcat );

	XARC_SER_ARR( xs, &rotation[0].x, EL_MAX_ANIM_FRAMES*4 );
	XARC_SER_ARR( xs, &position[0].x, EL_MAX_ANIM_FRAMES*3 );
	XarcClose( xs );
}

void BoneData::Serialize( XStream* xs )
{
	XarcOpen( xs, "BoneData" );
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		bone[i].Serialize( xs );
	}
	XarcClose( xs );
}

