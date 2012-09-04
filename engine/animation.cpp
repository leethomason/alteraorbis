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


AnimationResource::AnimationResource( const gamedb::Item* _item )
{
	item = _item;
	resName = item->Name();
	nAnimations = item->NumChildren();
	GLASSERT( nAnimations <= ANIM_COUNT );
	memset( sequence, 0, sizeof(Sequence)*ANIM_COUNT );

	for( int i=0; i<nAnimations; ++i ) {
		const gamedb::Item* subItem = item->Child(i);
		AnimationType type = NameToType( subItem->Name() );
		
		sequence[type].item = subItem;
		sequence[type].totalDuration = (U32) subItem->GetFloat( "totalDuration" );
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
	const char* name = TypeToName( type );
	const gamedb::Item* animItem = item->Child( name );
	GLASSERT( animItem );
	U32 totalTime = (U32)animItem->GetFloat( "totalDuration" );
	return totalTime;
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


bool AnimationResource::GetTransform(	AnimationType type, 
										const ModelHeader& header, 
										U32 timeClock, 
										BoneData* boneData ) const
{
	const char* animationName = TypeToName( type );
	const gamedb::Item* animItem = item->Child( animationName );
	GLASSERT( animItem );
	memset( boneData, 0, sizeof( *boneData ));
	
	// Use doubles, which have a great enough range to not overflow from U32
	double totalDuration = animItem->GetFloat( "totalDuration" );
	double time = 0;
	if ( AnimationResource::Looping( type )) {
		time = fmod( (double)timeClock, totalDuration );
	}
	else {
		time = Min( (double)timeClock, (double)(totalDuration-1) );
	}
	float fraction = 0;

	const gamedb::Item* frameItem0 = 0;
	const gamedb::Item* frameItem1 = 0;

	int frame0 = 0;
	int frame1 = 0;
	int nFrames = animItem->NumChildren()-1;
	GLASSERT( nFrames >= 1 );

	for( frame0=0; frame0<nFrames; ++frame0 ) {
		frameItem0 = animItem->Child( frame0 );
		GLASSERT( frameItem0 );
		//GLOUTPUT(( "frameItem %s\n", frameItem->Name() ));

		double frameDuration = frameItem0->GetFloat( "duration" );
		if ( (double)time < frameDuration ) {
			// We found the frame!
			fraction = (float)((double)time / (double)frameDuration);
			frame1 = (frame0 + 1) % nFrames;
			frameItem1 = animItem->Child( frame1 );

			break;
		}
		time -= frameDuration;
	}
	GLASSERT( frameItem0 );
	GLASSERT( frameItem1 );

	for( int i=0; i<frameItem0->NumChildren(); ++i ) {
		GLASSERT( i < EL_MAX_BONES );

		const gamedb::Item* boneItem0 = frameItem0->Child( i );
		const gamedb::Item* boneItem1 = frameItem1->Child( i );

		const char* boneName = boneItem0->Name();
		int index = header.BoneIDFromName( boneName );

		float angle0 = boneItem0->GetFloat( "anglePrime" );
		float angle1 = boneItem1->GetFloat( "anglePrime" );

		if ( fabsf( angle0-angle1 ) > 180.0 ) {
			if ( angle1 < angle0 ) angle1 += 360.0;
			else				   angle1 -= 360.0;
		}
		float angle = Lerp( angle0, angle1, fraction );

		float dy0 = boneItem0->GetFloat( "dy" );
		float dy1 = boneItem1->GetFloat( "dy" );
		float dy  = Lerp( dy0, dy1, fraction );

		float dz0 = boneItem0->GetFloat( "dz" );
		float dz1 = boneItem1->GetFloat( "dz" );
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
	int delta = t1 - t0;
	
	double totalTime = animItem->GetFloat( "totalDuration" );
	double t0f = fmod( (double)t0, totalTime );
	double t1f = t0f + Min( (double)delta, totalTime );

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

