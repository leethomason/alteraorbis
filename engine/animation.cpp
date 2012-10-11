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
	//"impactlight",
	"impactheavy"
};

static const bool gAnimationLooping[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
	false,
	//false,
	false
};

static const bool gAnimationSync[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
	false,
	//false,
	false
};

static const bool gAnimationMotion[ ANIM_COUNT ] = {
	false,
	true,
	true,
	false,
	false,
	//true,
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
		sequence[type].totalDuration = animItem->GetInt( "totalDuration" );

		int nFrames = animItem->NumChildren();
		GLASSERT( nFrames >= 1 );
		sequence[type].nFrames = nFrames;

		for( int frame=0; frame<nFrames; ++frame ) {
			const gamedb::Item* frameItem = animItem->Child( frame );	// frame[0]
			
			sequence[type].frame[frame].start = frameItem->GetInt( "time" );
			
			static const char* events[EL_MAX_METADATA] = { "event0", "event1", "event2", "event3" };
			static const char* metaNames[ANIM_META_COUNT] = { "none", "impact" };

			for( int k=0; k<EL_MAX_METADATA; ++k ) {
				if ( frameItem->HasAttribute( events[k] ) ) {
					const char* metaName = frameItem->GetString( events[k] );
					for( int n=0; n<ANIM_META_COUNT; ++n ) {
						if ( StrEqual( metaNames[n], metaName ) ) {
							sequence[type].frame[frame].metaData[k] = n;
							break;
						}
					}
				}
			}

			int nBones = frameItem->NumChildren();
			sequence[type].nBones = nBones;

			for( int bone=0; bone<nBones; ++bone ) {
				const gamedb::Item* boneItem = frameItem->Child( bone );
				IString boneName = StringPool::Intern( boneItem->Name(), true );

				sequence[type].frame[frame].boneData.bone[bone].name = boneName;
				sequence[type].frame[frame].boneData.bone[bone].angleRadians = boneItem->GetFloat( "anglePrime" );
				sequence[type].frame[frame].boneData.bone[bone].dy = boneItem->GetFloat( "dy" );
				sequence[type].frame[frame].boneData.bone[bone].dz = boneItem->GetFloat( "dz" );
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
									 IString boneName,
									 BoneData::Bone* bone ) const
{
	const BoneData::Bone* bone0 = sequence[type].frame[frame0].boneData.GetBone( boneName );
	const BoneData::Bone* bone1 = sequence[type].frame[frame1].boneData.GetBone( boneName );

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
										IString boneName,
										const ModelHeader& header,	// used to get the bone IDs
										U32 time,					// time for this animation
										BoneData::Bone* bone ) const
{
	float fraction = 0;
	int frame0 = 0;
	int frame1 = 0;
	ComputeFrame( type, time, &frame0, &frame1, &fraction );
	ComputeBone( type, frame0, frame1, fraction, boneName, bone );
	return true;
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

		IString boneName = sequence[type].frame[frame0].boneData.bone[i].name;
		boneData->bone[i].name = boneName;
		ComputeBone( type, frame0, frame1, fraction, boneName, boneData->bone + i );
	}
	return true;
}


void AnimationResource::GetMetaData(	AnimationType type,
										U32 t0, U32 t1,				// t1 > t0
										grinliz::CArray<int, EL_MAX_METADATA>* data ) const
{
	data->Clear();

	GLASSERT( t1 >= t0 );
	int delta = int(t1 - t0);
	
	t0 = TimeInRange( type, t0 );

	int i=0;
	while( delta > 0 ) {
		if (    t0 >= sequence[type].frame[i].start 
			 && t0 < sequence[type].frame[i].end ) 
		{
			for( int j=0; j<EL_MAX_METADATA;  ++j ) {
				int m = sequence[type].frame[i].metaData[j];
				if ( m > 0 ) {
					data->Push( m ); 
				}
			}
			delta -= sequence[type].frame[i].end - t0;
			t0 = sequence[type].frame[i].end;
		}
		++i;
		if ( i==sequence[type].nFrames ) {
			i = 0;
			t0 = 0;
		}
	}
}

