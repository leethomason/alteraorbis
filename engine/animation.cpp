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
	"reference",
	"walk",
	"gunrun",
	"gunstand",
	"melee",
	"impactheavy"
};

static const bool gAnimationLooping[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
	false,
	false
};

static const bool gAnimationSync[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
	false,
	false
};

static const bool gAnimationMotion[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
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
	return gAnimationName[ type ];
}

/* static */ int AnimationResource::NameToType( const char* name )
{
	for( int i=0; i<ANIM_COUNT; ++i ) {
		if ( StrEqual( gAnimationName[i], name )) {
			return i;
		}
	}
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
      walk [ totalDuration=833]
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
			
				sequence[type].frame[frame].start = frameItem->GetInt( "time" );
			
				// Set up and perform a recursive walk to generate a list
				// of bones from the tree. Dependent bones are further
				// down the list than their parents.
				int boneIndex = 0;
				const gamedb::Item* boneItem = frameItem->ChildAt( 0 );
				RecBoneWalk( boneItem, &boneIndex, &sequence[type].frame[frame].boneData );
				sequence[type].nBones = boneIndex;

				// Walk and compute the reference matrices.
				FILE* fp = fopen( "animout.txt", "w" );
				for( int i=0; i<sequence[type].nBones; ++i ) {
					BoneData::Bone* bone = &sequence[type].frame[frame].boneData.bone[i];
					BoneData::Bone* parentBone = 0;
					if ( bone->parent >= 0 ) {
						GLASSERT( bone->parent < i );
						parentBone = &sequence[type].frame[frame].boneData.bone[bone->parent];
					}
					if ( parentBone ) {
						bone->refConcat = parentBone->refConcat + bone->refPos;
					}
					else {
						bone->refConcat = bone->refPos;
					}
				}
				for( int i=0; i<sequence[type].nBones; ++i ) {
					BoneData::Bone* bone = &sequence[type].frame[frame].boneData.bone[i];
					GLLOG(( "An %s Sq %d Fr %d Bn %s Ref %.2f,%.2f,%.2f\n",
						resName, type, frame, bone->name.c_str(), bone->refConcat.x, bone->refConcat.y, bone->refConcat.z ));
				}
			}
			for( int frame=0; frame<nFrames; ++frame ) {
				if ( frame+1 < nFrames )
					sequence[type].frame[frame].end = sequence[type].frame[frame+1].start;
				else
					sequence[type].frame[frame].end = sequence[type].totalDuration;
			}
		}
	}
	else {
		// Set up a null animation.
		item = 0;
		resName = "nullAnimation";
		nAnimations = ANIM_COUNT;
		memset( sequence, 0, sizeof(Sequence)*ANIM_COUNT );
		
		static const int DURATION[ANIM_COUNT] = { 
			1000, // reference
			1000, // walk
			1000, // gun walk
			1000, // gun stand
			600,  // melee,
			200,  // count
		};

		for( int i=0; i<ANIM_COUNT; ++i ) {
			sequence[i].totalDuration = DURATION[i];
			sequence[i].item = 0;
			sequence[i].nFrames = 1;
			sequence[i].nBones = 0;
			sequence[i].frame[0].start = 0;
			sequence[i].frame[0].end   = DURATION[i];
		}

		sequence[ANIM_MELEE].metaDataID = ANIM_META_IMPACT;
		sequence[ANIM_MELEE].metaDataTime = DURATION[ANIM_MELEE]/2;
	}
}


void AnimationResource::RecBoneWalk( const gamedb::Item* boneItem, int *boneIndex, BoneData* boneData )
{
	int index = *boneIndex;
	*boneIndex += 1;
	BoneData::Bone* bone = &boneData->bone[index];
	
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

	boneItem->GetFloatArray( "rotation", 4, &bone->rotation.x );
	bone->rotation.Normalize();
	boneItem->GetFloatArray( "position", 3, &bone->position.x );

	for( int i=0; i<boneItem->NumChildren(); ++i ) {
		const gamedb::Item* child = boneItem->ChildAt( i );
		RecBoneWalk( child, boneIndex, boneData );
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


U32 AnimationResource::TimeInRange( int type, U32 t ) const
{
	GLASSERT( type >= 0 && type < ANIM_COUNT );
	U32 total = sequence[type].totalDuration;
	U32 result = 0;

	if ( Looping( type )) {
		result = t % total;
	}
	else {
		result = Min( total-1, t );
	}
	return result;
}


void AnimationResource::ComputeFrame( int type,
									  U32 timeClock,
									  int *_frame0, int* _frame1, float* _fraction ) const
{
	int frame0=0, frame1=0;
	float fraction=0;

	U32 time = TimeInRange( type, timeClock );

	for( frame0=0; frame0<sequence[type].nFrames; ++frame0 ) {
		U32 start = sequence[type].frame[frame0].start;
		U32 end   = sequence[type].frame[frame0].end;
		if (    time >= start
				&& time <  end ) 
		{
			// We found the frame!
			fraction = (float)((double)(time-start) / (double)(end-start));
			frame1 = (frame0 + 1) % sequence[type].nFrames;
			break;
		}
	}
	GLASSERT( frame0 < sequence[type].nFrames );
	GLASSERT( frame1 < sequence[type].nFrames );
	*_frame0 = frame0;
	*_frame1 = frame1;
	*_fraction = fraction;
}


void AnimationResource::ComputeBone( int type,
									 int frame0, int frame1, float fraction,
									 int index0,
									 BoneData::Bone* bone ) const
{
	int index1 = (index0+1)%sequence[type].nBones;
	const BoneData::Bone* bone0 = &sequence[type].frame[frame0].boneData.bone[index0];
	const BoneData::Bone* bone1 = &sequence[type].frame[frame1].boneData.bone[index1];

	Quaternion angle0 = bone0->rotation;
	Quaternion angle1 = bone1->rotation;
	Quaternion angle = angle0;
	//Quaternion::SLERP( angle0, angle1, fraction, &angle );

	Vector3F pos;
	for( int i=0; i<3; ++i ) {
		pos.X(i) = Lerp( bone0->position.X(i), bone1->position.X(i), fraction );
	}

	bone->rotation	= angle;
	bone->position	= pos;
}


bool AnimationResource::GetTransform(	int type,					// which animation to play: "reference", "gunrun", etc.
										IString boneName,
										const ModelHeader& header,	// used to get the bone IDs
										U32 time,					// time for this animation
										BoneData::Bone* bone ) const
{
	float fraction = 0;
	int frame0 = 0;
	int frame1 = 0;
	int boneIndex = sequence[type].frame[0].boneData.GetBoneIndex( boneName );	// order is the same for all frames
	ComputeFrame( type, time, &frame0, &frame1, &fraction );
	ComputeBone( type, frame0, frame1, fraction, boneIndex, bone );
	return true;
}


/*	This copies the resource BoneData to the Model bone data,
	doing interpolation in the process.
*/
bool AnimationResource::GetTransform(	int type, 
										const ModelHeader& header, 
										U32 timeClock, 
										BoneData* boneData ) const
{
	*boneData = this->sequence[type].frame[0].boneData;	// The basics are the same for every frame.
														// The part that changes is written below.

	float fraction = 0;
	int frame0 = 0;
	int frame1 = 0;
	ComputeFrame( type, timeClock, &frame0, &frame1, &fraction );

	for( int i=0; i<sequence[type].nBones; ++i ) {
		GLASSERT( i < EL_MAX_BONES );
		ComputeBone( type, frame0, frame1, fraction, i, boneData->bone + i );
	}
	return true;
}


int AnimationResource::GetMetaData(	int type, U32 t0, U32 t1 ) const
{
	if ( sequence[type].metaDataID == 0 )
		return 0;

	GLASSERT( t1 >= t0 );
	int delta = int(t1 - t0);
	
	t0 = TimeInRange( type, t0 );

	U32 tEvent = sequence[type].metaDataTime;

	if ( tEvent >= t0 && tEvent < t0 + delta ) {
		return sequence[type].metaDataID;
	}
	if ( t0 + delta >= sequence[type].totalDuration ) {
		U32 t = (t0+delta) - sequence[type].totalDuration;
		if ( tEvent < t ) {
			return sequence[type].metaDataID;
		}
	}
	return 0;
}

