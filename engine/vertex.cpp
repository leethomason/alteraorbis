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

void BoneData::Serialize( XStream* xs )
{
	XarcOpen( xs, "BoneData" );
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		XarcOpen( xs, "bone" );
		XARC_SER_KEY( xs, "name", bone[i].name );
		XARC_SER_KEY( xs, "parent", bone[i].parent );
		XARC_SER_KEY( xs, "refPos", bone[i].refPos );
		XARC_SER_KEY( xs, "refConcat", bone[i].refConcat );
		XARC_SER_KEY( xs, "rotation", bone[i].rotation );
		XARC_SER_KEY( xs, "position", bone[i].position );
		XarcClose( xs );
	}
	XarcClose( xs );
}


void BoneData::FlushTransform( grinliz::Matrix4* output, int num )
{
	Matrix4 concat[EL_MAX_BONES];

	for( int i=0; i<num; ++i ) {
		output[i].SetIdentity();

		if ( bone[i].name.empty() )
			continue;

		// The matrix takes the transform back to the origin
		// then out transformed place.

		Matrix4 inv;
		inv.SetTranslation( -bone[i].refConcat );	// very easy inverse xform

		Matrix4 t, r;
		t.SetTranslation( bone[i].position );
		bone[i].rotation.ToMatrix( &r );
		Matrix4 m = r * t;

		if ( bone[i].parent ) {
			concat[i] = concat[bone[i].parent] * m;
		}
		else {
			concat[i] = m;
		}

		output[i] = concat[i] * inv;
	}
}

