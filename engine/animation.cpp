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

#include "animation.h"
#include "../grinliz/glstringutil.h"
#include "../shared/gamedbreader.h"
#include "model.h"

using namespace grinliz;

AnimationResourceManager* AnimationResourceManager::instance = 0;

static const char* gAnimationName[ ANIM_COUNT ] = {
	"stand",
	"walk",
	"gunstand",
	"gunwalk",
	"melee",
	"impact"
};

static const bool gAnimationLooping[ ANIM_COUNT ] = {
	false,
	true,
	false,
	true,
	false,
	false
};

static const bool gAnimationSync[ ANIM_COUNT ] = {
	false,
	true,
	false,
	true,
	false,
	false
};

static const bool gAnimationMotion[ ANIM_COUNT ] = {
	false,
	true,
	false,
	true,
	false,
	true
};

bool AnimationResource::Looping( int type ) { 
	return gAnimationLooping[type]; 
}

bool AnimationResource::Synchronized( int type ) { 
	return gAnimationSync[type]; 
}

bool AnimationResource::Motion( int type ) {
	return gAnimationMotion[type];
}


/* static */ const char* AnimationResource::TypeToName( int type )
{
	GLASSERT( type >= 0 && type < ANIM_COUNT );
	return gAnimationName[ type ];
}

/* static */ int AnimationResource::NameToType( const char* name )
{
	for( int i=0; i<ANIM_COUNT; ++i ) {
		if ( StrEqual( gAnimationName[i], name )) {
			return i;
		}
	}
	GLASSERT( 0 );
	return ANIM_OFF;
}


void AnimationResourceManager::Create()
{
	GLASSERT( !instance );
	instance = new AnimationResourceManager();

	Quaternion::Test();
}


void AnimationResourceManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


AnimationResourceManager::AnimationResourceManager()
{
	nullAnimation = 0;
}


AnimationResourceManager::~AnimationResourceManager()
{
	delete nullAnimation;
}


void AnimationResourceManager::Load( const gamedb::Reader* reader )
{
	const gamedb::Item* parent = reader->Root()->Child( "animations" );
	GLASSERT( parent );

	for( int i=0; i<parent->NumChildren(); ++i )
	{
		const gamedb::Item* node = parent->ChildAt( i );
		
		AnimationResource* ar = new AnimationResource( node );
		GLASSERT( !resArr.Query( ar->ResourceName(), 0 ));
		resArr.Add( ar->ResourceName(), ar );
	}
}


const AnimationResource* AnimationResourceManager::GetResource( const char* name )
{
	if ( !name || !(*name) ) {
		return 0;
	}
	if ( StrEqual( name, "nullAnimation" )) {
		if ( !nullAnimation ) {
			nullAnimation = new AnimationResource(0);
		}
		return nullAnimation; 
	}
	return resArr.Get( name );
}


bool AnimationResourceManager::HasResource( const char* name )
{
	if ( !name || !(*name) ) {
		return false;
	}
	if ( StrEqual( name, "nullAnimation" )) {
		return true; 
	}
	return resArr.Query( name, 0 );
}


/*
  V1
    mantisAnimation []
      melee [ metaData='impact' metaDataTime=300 totalDuration=598]
        0000 [ time=0]
          arm.lower.left [ anglePrime=0.000000 dy=0.000000 dz=0.000000]
          arm.lower.right [ anglePrime=0.000000 dy=0.000000 dz=0.000000]
          arm.upper.left [ anglePrime=0.000000 dy=0.000000 dz=0.000000]
		  ...
        0001 [ time=72]
          arm.lower.left [ anglePrime=0.000000 dy=0.020558 dz=0.079295]
          arm.lower.right [ anglePrime=0.000000 dy=0.000000 dz=0.000000]
          arm.upper.left [ anglePrime=354.709930 dy=-0.020194 dz=0.052289]
		  ...
*/


/*
  V2
    humanMale2Animation []
      walk [ totalDuration=833 metaData='impact' metaDataTime=300 ]
        0000 [ time=0]
          base [ position=(0.000000 -0.000000 -0.000000 ) rotation=(0.000000 0.000000 0.000000 1.000000 )]
            leg.upper.left [ parent='base' position=(0.108094 0.160939 38.067875 ) rotation=(-0.001098 0.004170 -0.254688 0.967014 )]
              leg.lower.left [ parent='leg.upper.left' position=(-0.273515 -0.014067 -0.077826 ) rotation=(0.000000 0.000000 0.000000 1.000000 )]
            leg.upper.right [ parent='base' position=(0.694870 0.011449 0.346949 ) rotation=(0.000642 0.004264 0.148850 0.988850 )]
              leg.lower.right [ parent='leg.upper.right' position=(-1.273861 -0.075174 0.000000 ) rotation=(0.000000 0.000000 0.000000 1.000000 )]
            torso [ parent='base' position=(0.000000 -0.000000 -0.000000 ) rotation=(0.000000 0.000000 0.000000 1.000000 )]
			...
*/

AnimationResource::AnimationResource( const gamedb::Item* _item )
{
	// Set up a null animation.
	item = 0;
	resName = "nullAnimation";
	nAnimations = ANIM_COUNT;
	memset( sequence, 0, sizeof(Sequence)*ANIM_COUNT );
		
	static const int DURATION[ANIM_COUNT] = { 
		1000, // stand
		1000, // walk
		1000, // gun stand
		1000, // gun walk
		1000, // melee,
		200,  // impact
	};

	for( int i=0; i<ANIM_COUNT; ++i ) {
		sequence[i].totalDuration = DURATION[i];
		sequence[i].item = 0;
		sequence[i].nFrames = 1;
		sequence[i].nBones = 0;

		for( int j=0; j<EL_MAX_BONES; ++j ) {
			for( int k=0; k<EL_MAX_ANIM_FRAMES; ++k ) {
				sequence[i].boneData.bone[j].rotation[k] = Quaternion();
			}
		}
	}

	sequence[ANIM_MELEE].metaDataID = ANIM_META_IMPACT;
	sequence[ANIM_MELEE].metaDataTime = DURATION[ANIM_MELEE]/2;

	if ( _item ) {
		item = _item;
		resName = item->Name();
		nAnimations = item->NumChildren();
		GLASSERT( nAnimations <= ANIM_COUNT );
		memset( sequence, 0, sizeof(Sequence)*ANIM_COUNT );

		for( int i=0; i<nAnimations; ++i ) {
			const gamedb::Item* animItem = item->ChildAt(i);		// "gunrun" in the example
			int type = NameToType( animItem->Name() );
		
			sequence[type].item = animItem;
			sequence[type].totalDuration = animItem->GetInt( "totalDuration" );

			if ( animItem->HasAttribute( "metaData" )) {
				static const char* metaNames[ANIM_META_COUNT] = { "none", "impact" };
				const char* mdName = animItem->GetString( "metaData" );

				for( int k=1; k<ANIM_META_COUNT; ++k ) {
					if ( StrEqual( metaNames[k], mdName )) {
						int time = animItem->GetInt( "metaDataTime" );

						sequence[type].metaDataID = k;
						sequence[type].metaDataTime = time;
						break;
					}
				}
			}

			int nFrames = animItem->NumChildren();
			GLASSERT( nFrames >= 1 );
			sequence[type].nFrames = nFrames;

			for( int frame=0; frame<nFrames; ++frame ) {
				const gamedb::Item* frameItem = animItem->Child( frame );	// frame[0]
			
				// Set up and perform a recursive walk to generate a list
				// of bones from the tree. Dependent bones are further
				// down the list than their parents.
				int boneIndex = 0;
				const gamedb::Item* boneItem = frameItem->ChildAt( 0 );
				RecBoneWalk( boneItem, frame, &boneIndex, &sequence[type].boneData );
				sequence[type].nBones = boneIndex;

				// Walk and compute the reference matrices.
				for( int i=0; i<sequence[type].nBones; ++i ) {
					BoneData::Bone* bone = &sequence[type].boneData.bone[i];
					BoneData::Bone* parentBone = 0;
					if ( bone->parent >= 0 ) {
						GLASSERT( bone->parent < i );
						parentBone = &sequence[type].boneData.bone[bone->parent];
					}
					if ( parentBone ) {
						bone->refConcat = parentBone->refConcat + bone->refPos;
						//bone->refConcat = parentBone->refConcat + parentBone->refPos;
					}
					else {
						bone->refConcat = bone->refPos;
						//bone->refConcat.Set( 0, 0, 0 );
					}
				}
				for( int i=0; i<sequence[type].nBones; ++i ) {
					BoneData::Bone* bone = &sequence[type].boneData.bone[i];
//					GLLOG(( "An %s Sq %d Fr %2d Bn %16s Ref %5.2f,%5.2f,%5.2f\n",
//						resName, type, frame, bone->name.c_str(), 
//						bone->refConcat.x, bone->refConcat.y, bone->refConcat.z ));
				}
			}
		}
	}
}


void AnimationResource::RecBoneWalk( const gamedb::Item* boneItem, int frame, int *boneIndex, BoneData* boneData )
{
	int index = *boneIndex;
	*boneIndex += 1;
	BoneData::Bone* bone = &boneData->bone[index];
	
	if ( frame == 0 ) {
		bone->name = StringPool::Intern( boneItem->Name() );

		bone->parent = -1;
		if ( boneItem->HasAttribute( "parent" )) {
			const char* parent = boneItem->GetString( "parent" );
			for( int i=0; i<index; ++i ) {
				if ( boneData->bone[i].name == parent ) {
					bone->parent = i;
					break;
				}
			}
			GLASSERT( bone->parent >= 0 );	// should always be higher in the list.
		}

		boneItem->GetFloatArray( "refPosition", 3, &bone->refPos.x );
		bone->refConcat.Zero();
	}
	boneItem->GetFloatArray( "rotation", 4, &bone->rotation[frame].x );
	bone->rotation[frame].Normalize();
	boneItem->GetFloatArray( "position", 3, &bone->position[frame].x );

	for( int i=0; i<boneItem->NumChildren(); ++i ) {
		const gamedb::Item* child = boneItem->ChildAt( i );
		RecBoneWalk( child, frame, boneIndex, boneData );
	}
}


const char* AnimationResource::AnimationName( int index ) const
{
	GLASSERT( index >= 0 && index < nAnimations );
	if ( item ) 
		return item->ChildAt( index )->Name();
	return gAnimationName[index];
}


bool AnimationResource::HasAnimation( int type ) const
{
	GLASSERT( type >= 0 && type < ANIM_COUNT );
	return sequence[type].nFrames > 0;
}


U32 AnimationResource::Duration( int type ) const
{
	GLASSERT( type >= 0 && type < ANIM_COUNT );
	return sequence[type].totalDuration;
}


void AnimationResource::ComputeFrame( int type,
									  U32 timeClock,
									  int *frame0, int* frame1, float* fraction ) const
{
	U32 timeInSequence = timeClock % sequence[type].totalDuration;
	*frame0 = timeInSequence * sequence[type].nFrames / sequence[type].totalDuration;
	*frame1 = (*frame0 + 1);
	if ( *frame1 == sequence[type].nFrames ) {
		*frame1 = 0;
	}
	int delta = timeInSequence - (*frame0) * sequence[type].totalDuration / sequence[type].nFrames;
	*fraction = (float)delta / (float)(sequence[type].totalDuration / sequence[type].nFrames);
}


void AnimationResource::GetTransform(	int typeA,					// which animation to play: "reference", "gunrun", etc.
										U32 timeA,					// time for this animation
										int typeB,					// 2nd animation
										U32 timeB,					// 2nd animation time
										float crossFraction,		// 0: all A, 1: all B
										grinliz::Matrix4* output ) const	// At least EL_MAX_BONES
{
	bool hasFade = crossFraction > 0;

	int frameA0=0, frameA1=0, frameB0=0, frameB1=0;
	float fractionA=0, fractionB=0;

	ComputeFrame( typeA, timeA, &frameA0, &frameA1, &fractionA );
	if ( hasFade ) {
		ComputeFrame( typeB, timeB, &frameB0, &frameB1, &fractionB );
	}

	Matrix4 concat[EL_MAX_BONES];
	const BoneData& boneDataA = sequence[typeA].boneData;
	const BoneData& boneDataB = sequence[typeB].boneData;

	for( int i=0; i<EL_MAX_BONES; ++i ) {
		output[i].SetIdentity();

		if ( boneDataA.bone[i].name.empty() )
			continue;

		Vector3F	positionA, positionB, position;
		Quaternion	rotationA, rotationB, rotation;

		for( int k=0; k<3; ++k ) {
			positionA.X(k) = Lerp( boneDataA.bone[i].position[frameA0].X(k), 
								   boneDataA.bone[i].position[frameA1].X(k), 
								   fractionA );
			if ( hasFade ) {
				positionB.X(k) = Lerp( boneDataB.bone[i].position[frameB0].X(k), 
									   boneDataB.bone[i].position[frameB1].X(k), 
									   fractionB );
				position.X(k) = Lerp( positionA.X(k), positionB.X(k), crossFraction );
			}
		}
		if ( !hasFade ) {
			position = positionA;
		}

		if ( hasFade ) {
			Quaternion::SLERP( boneDataA.bone[i].rotation[frameA0],
							   boneDataA.bone[i].rotation[frameA1],
							   fractionA,
							   &rotationA );
			Quaternion::SLERP( boneDataB.bone[i].rotation[frameB0],
							   boneDataB.bone[i].rotation[frameB1],
							   fractionB,
							   &rotationB );
			Quaternion::SLERP( rotationA, rotationB, crossFraction, &rotation );
		}
		else {
			Quaternion::SLERP( boneDataA.bone[i].rotation[frameA0],
							   boneDataA.bone[i].rotation[frameA1],
							   fractionA,
							   &rotation );
		}
			

		// The 'inv' takes the transform back to the origin
		// then out transformed place. It is pure translation
		// which is very nice.

		Matrix4 inv;
		inv.SetTranslation( -boneDataA.bone[i].refConcat );	// very easy inverse xform

		Matrix4		m;
		Vector3F	t;

		// This is a hack, or a bug, or a mis-understanding of the file
		// format. (Eek. All bad.) Sometimes the position is set, 
		// and sometimes it is not. So use the position if specified,
		// else use the reference position.
		static const Vector3F ZERO = { 0, 0, 0 };
		if ( position.Equal( ZERO, 0.0001f )) {
			t = boneDataA.bone[i].refPos;
		}
		else {
			t = position;
		}
		rotation.ToMatrix( &m );
		m.SetCol( 3, t.x, t.y, t.z, 1 );

		int parentIndex = boneDataA.bone[i].parent; 
		if ( parentIndex >= 0 ) {
			output[i] = concat[parentIndex] * m * inv;
			concat[i] = concat[parentIndex] * m;
		}
		else {
			output[i] = m * inv;
			concat[i] = m;
		}
	}
}


int AnimationResource::GetMetaData(	int type, U32 t0, U32 t1 ) const
{
	if ( sequence[type].metaDataID == 0 )
		return 0;

	U32 t0Mod = t0 % sequence[type].totalDuration;
	U32 delta = t1 - t0;

	U32 tEvent = sequence[type].metaDataTime;

	if ( tEvent >= t0Mod && tEvent < t0Mod + delta ) {
		return sequence[type].metaDataID;
	}
	return 0;
}

