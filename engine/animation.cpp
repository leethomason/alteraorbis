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


bool AnimationResource::Looping( AnimationType type ) { 
	return gAnimationLooping[type]; 
}

bool AnimationResource::Synchronized( AnimationType type ) { 
	return gAnimationSync[type]; 
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


bool AnimationResource::GetTransform(	AnimationType type,	// which animation to play: "reference", "gunrun", etc.
										const char* boneName,
										const ModelHeader& header,	// used to get the bone IDs
										U32 time,					// time for this animation
										BoneData::Bone* bone ) const
{
	// FIXME optimize to calc only one bone?
	BoneData boneData;
	GetTransform( type, header, time, &boneData );
	int index = header.BoneIDFromName( boneName );
	if ( index >= 0 ) {
		*bone = boneData.bone[index];		
		return true;
	}
	return false;
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
		result = Max( total-1, t );
	}
	return result;
}


bool AnimationResource::GetTransform(	AnimationType type, 
										const ModelHeader& header, 
										U32 timeClock, 
										BoneData* boneData ) const
{
	memset( boneData, 0, sizeof( *boneData ));

	// Use doubles, which have a great enough range to not overflow from U32
	U32 time = TimeInRange( type, timeClock );
	float fraction = 0;

	int frame0 = 0;
	int frame1 = 0;

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

	for( int i=0; i<sequence[type].nBones; ++i ) {
		GLASSERT( i < EL_MAX_BONES );

		// The index and the boneName don't match up. Patch here.
		// Can't patch at load time because the 'header' is required,
		// so that animation can be multiply assigned.
		const char* boneName = sequence[type].frame[frame0].boneName[i];
		int index = header.BoneIDFromName( boneName );

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

		boneData->bone[index].angleRadians	= ToRadian( angle );
		boneData->bone[index].dy			= dy;
		boneData->bone[index].dz			= dz;
	}
	return true;
}


void AnimationResource::GetMetaData(	AnimationType type,
										U32 t0, U32 t1,				// t1 > t0
										grinliz::CArray<AnimationMetaData, EL_MAX_METADATA>* data ) const
{
	const char* animationName = TypeToName( type );
	const gamedb::Item* animItem = item->Child( animationName );
	GLASSERT( animItem );
	const gamedb::Item* metaItem = animItem->Child( "metaData" );
	data->Clear();

	GLASSERT( t1 >= t0 );
	U32 delta = t1 - t0;
	
	double totalTime = (double)Duration( type );
	double t0f = (double)TimeInRange( type, t0 );
	double t1f = (double)TimeInRange( type, t0 + Min( delta, Duration( type )));

	for( int pass=0; pass<2; ++pass ) {
		for( int i=0; i<metaItem->NumChildren(); ++i ) {
			const gamedb::Item* dataItem = metaItem->Child( i );
			double t = dataItem->GetFloat( "time" );

			if ( t >= t0f && t < t1f ) {
				AnimationMetaData amd;
				amd.name = dataItem->Name();
				amd.time = (U32)t;
				data->Push( amd );
			}
		}
		if ( t1f > totalTime ) {
			t0f -= totalTime;
			t1f -= totalTime;
			// go around again.
		}
		else {
			break;
		}
	}
}

