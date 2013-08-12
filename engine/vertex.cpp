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


/*
void BoneData::FlushTransform(	int frame0, int frame1, float fraction,	// the interpolation
								int nBones,
								grinliz::Matrix4* output ) const
{
	Matrix4 concat[EL_MAX_BONES];

	for( int i=0; i<nBones; ++i ) {
		output[i].SetIdentity();

		if ( bone[i].name.empty() )
			continue;

		// The matrix takes the transform back to the origin
		// then out transformed place.

		Matrix4 inv;
		inv.SetTranslation( -bone[i].refConcat );	// very easy inverse xform

		Vector3F	position;
		Quaternion	rotation;

		for( int k=0; k<3; ++k ) {
			position.X(k) = Lerp( bone[i].position[frame0].X(k), bone[i].position[frame1].X(k), fraction );
		}
		// FIXME: SLERP
		rotation = bone[i].rotation[frame0];

		Matrix4 t, r;
		t.SetTranslation( position );
		rotation.ToMatrix( &r );
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
*/
