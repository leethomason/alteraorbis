#include "vertex.h"

void BoneData::Bone::ToMatrix( grinliz::Matrix4* mat ) const
{
	mat->SetIdentity();
	mat->SetXRotation( grinliz::ToDegree( angleRadians ));
	mat->SetTranslation( 0, dy, dz );
}

