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
	"impactlight",
	"impactheavy"
};

static const bool gAnimationLooping[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
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
	false,
	false
};

static const bool gAnimationMotion[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
	false,
	true,
	true
};

bool AnimationResource::Looping( AnimationType type ) { 
	return gAnimationLooping[type]; 
}

bool AnimationResource::Synchronized( AnimationType type ) { 
	return gAnimationSync[type]; 
}

bool AnimationResource::Motion( AnimationType type ) {
	return gAnimationMotion[type];
}


/* static */ const char* AnimationResource::TypeToName( AnimationType type )
{
	return gAnimationName[ type ];
}

/* static */ AnimationType AnimationResource::NameToType( const char* name )
{
	for( int i=0; i<ANIM_COUNT; ++i ) {
		if ( StrEqual( gAnimationName[i], name )) {
			return (AnimationType)i;
		}
	}
	return ANIM_OFF;
}


void AnimationResourceManager::Create()
{
	GLASSERT( !instance );
	instance = new AnimationResourceManager();
}


void AnimationResourceManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


AnimationResourceManager::AnimationResourceManager()
{
}


AnimationResourceManager::~AnimationResourceManager()
{
}


void AnimationResourceManager::Load( const gamedb::Reader* reader )
{
	const gamedb::Item* parent = reader->Root()->Child( "animations" );
	GLASSERT( parent );

	for( int i=0; i<parent->NumChildren(); ++i )
	{
		const gamedb::Item* node = parent->Child( i );
		
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
	return resArr.Get( name );
}


bool AnimationResourceManager::HasResource( const char* name )
{
	if ( !name || !(*name) ) {
		return false;
	}
	return resArr.Query( name, 0 );
}


/*
  animations []
    humanFemale []
      gunrun [ totalDuration=1200.000000]
        0 [ duration=200.000000]
          arm.lower.left [ angle=27.083555 dy=-0.000076 dz=0.000340]
          arm.lower.right [ angle=268.449707 dy=0.000445 dz=-0.000228]
          arm.upper.left [ angle=28.810850 dy=-0.000000 dz=0.000000]
		  ...
*/
AnimationResource::AnimationResource( const gamedb::Item* _item )
{
	item = _item;
	resName = item->Name();
	nAnimations = item->NumChildren();
	GLASSERT( nAnimations <= ANIM_COUNT );
	memset( sequence, 0, sizeof(Sequence)*ANIM_COUNT );

	for( int i=0; i<nAnimations; ++i ) {
		const gamedb::Item* animItem = item->Child(i);		// "gunrun" in the example
		AnimationType type = NameToType( animItem->Name() );
		
		sequence[type].item = animItem;
		sequence[type].totalDuration = 0;					// computed from frame durations, below

		int nFrames = animItem->NumChildren()-1;			// last child is metadata
		GLASSERT( nFrames >= 1 );
		sequence[type].nFrames = nFrames;

		for( int frame=0; frame<nFrames; ++frame ) {
			const gamedb::Item* frameItem = animItem->Child( frame );	// frame[0]
			
			sequence[type].frame[frame].duration = LRintf( frameItem->GetFloat( "duration" ));
			sequence[type].frame[frame].start = sequence[type].totalDuration;
			sequence[type].totalDuration += sequence[type].frame[frame].duration;
			sequence[type].frame[frame].end = sequence[type].totalDuration;

			int nBones = frameItem->NumChildren();
			sequence[type].nBones = nBones;

			for( int bone=0; bone<nBones; ++bone ) {
				const gamedb::Item* boneItem = frameItem->Child( bone );
				const char* boneName = boneItem->Name();

				sequence[type].frame[frame].boneName[bone] = boneName;
				sequence[type].frame[frame].boneHash[bone] = Random::Hash( boneName, strlen( boneName ));
				sequence[type].frame[frame].boneData.bone[bone].angleRadians = boneItem->GetFloat( "anglePrime" );
				sequence[type].frame[frame].boneData.bone[bone].dy = boneItem->GetFloat( "dy" );
				sequence[type].frame[frame].boneData.bone[bone].dz = boneItem->GetFloat( "dz" );
			}
		}

		// The extra frame.
		const gamedb::Item* metaItem = animItem->Child( "metaData" );
		for( int j=0; j<metaItem->NumChildren(); ++j ) {
			const gamedb::Item* dataItem = metaItem->Child( j );

			sequence[type].metaData[j].time = LRintf( dataItem->GetFloat( "time" ));
			sequence[type].metaData[j].name = dataItem->Name();
		}
	}
}


const char* AnimationResource::AnimationName( int index ) const
{
	GLASSERT( index >= 0 && index < nAnimations );
	return item->Child( index )->Name();
}


bool AnimationResource::HasAnimation( AnimationType type ) const
{
	const char* name = TypeToName( type );
	return item->Child( name ) != 0;
}


U32 AnimationResource::Duration( AnimationType type ) const
{
	GLASSERT( type >= 0 && type < ANIM_COUNT );
	return sequence[type].totalDuration;
}


U32 AnimationResource::TimeInRange( AnimationType type, U32 t ) const
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


void AnimationResource::ComputeFrame( AnimationType type,
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


void AnimationResource::ComputeBone( AnimationType type,
									 int frame0, int frame1, float fraction,
									 int i,
									 BoneData::Bone* bone ) const
{
	const BoneData::Bone* bone0 = &sequence[type].frame[frame0].boneData.bone[i];
	const BoneData::Bone* bone1 = &sequence[type].frame[frame1].boneData.bone[i];

	float angle0 = bone0->angleRadians;
	float angle1 = bone1->angleRadians;

	if ( fabsf( angle0-angle1 ) > 180.0f ) {
		if ( angle1 < angle0 ) angle1 += 360.0f;
		else				   angle1 -= 360.0f;
	}
	float angle = Lerp( angle0, angle1, fraction );

	float dy0 = bone0->dy;
	float dy1 = bone1->dy;
	float dy  = Lerp( dy0, dy1, fraction );

	float dz0 = bone0->dz;
	float dz1 = bone1->dz;
	float dz  = Lerp( dz0, dz1, fraction );

	bone->angleRadians	= ToRadian( angle );
	bone->dy			= dy;
	bone->dz			= dz;
}


bool AnimationResource::GetTransform(	AnimationType type,	// which animation to play: "reference", "gunrun", etc.
										const char* boneName,
										const ModelHeader& header,	// used to get the bone IDs
										U32 time,					// time for this animation
										BoneData::Bone* bone ) const
{
	int i=0;
	for( ; i<sequence[type].nBones; ++i ) {
		if ( StrEqual( sequence[type].frame[0].boneName[i], boneName )) {
			break;
		}
	}
	if ( i < sequence[type].nBones ) {
		float fraction = 0;
		int frame0 = 0;
		int frame1 = 0;
		ComputeFrame( type, time, &frame0, &frame1, &fraction );
		ComputeBone( type, frame0, frame1, fraction, i, bone );
		return true;
	}
	return false;
}


bool AnimationResource::GetTransform(	AnimationType type, 
										const ModelHeader& header, 
										U32 timeClock, 
										BoneData* boneData ) const
{
	memset( boneData, 0, sizeof( *boneData ));

	float fraction = 0;
	int frame0 = 0;
	int frame1 = 0;
	ComputeFrame( type, timeClock, &frame0, &frame1, &fraction );

	for( int i=0; i<sequence[type].nBones; ++i ) {
		GLASSERT( i < EL_MAX_BONES );

		// The index and the boneName don't match up. Patch here.
		// Can't patch at load time because the 'header' is required,
		// so that animation can be multiply assigned.
		const char* boneName = sequence[type].frame[frame0].boneName[i];
		int index = header.BoneIDFromName( boneName );

		ComputeBone( type, frame0, frame1, fraction, i, boneData->bone + index );
	}
	return true;
}


void AnimationResource::GetMetaData(	AnimationType type,
										U32 t0, U32 t1,				// t1 > t0
										grinliz::CArray<const AnimationMetaData*, EL_MAX_METADATA>* data ) const
{
	data->Clear();

	GLASSERT( t1 >= t0 );
	U32 delta = t1 - t0;
	
	t0 = TimeInRange( type, t0 );
	t1 = t0 + delta;

	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( sequence[type].metaData[i].name ) {
			U32 t = sequence[type].metaData[i].time;
			if ( t < t0 )
				t += Duration(type);
			if ( t >= t0 && t < t1 ) {
				data->Push( &sequence[type].metaData[i]);
			}
		}
	}
}

