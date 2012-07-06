#include "animation.h"
#include "../grinliz/glstringutil.h"
#include "../shared/gamedbreader.h"

using namespace grinliz;

AnimationResourceManager* AnimationResourceManager::instance = 0;

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
	for( int i=0; i<resArr.Size(); ++i ) {
		delete resArr[i];
	}
}


void AnimationResourceManager::Load( const gamedb::Reader* reader )
{
	const gamedb::Item* parent = reader->Root()->Child( "animations" );
	GLASSERT( parent );

	for( int i=0; i<parent->NumChildren(); ++i )
	{
		const gamedb::Item* node = parent->Child( i );
		
		AnimationResource* ar = new AnimationResource( node );
		resArr.Push( ar );
	}
}


const AnimationResource* AnimationResourceManager::GetResource( const char* name )
{
	if ( !name || !(*name) ) {
		return 0;
	}
	for( int i=0; i<resArr.Size(); ++i ) {
		if ( StrEqual( resArr[i]->Name(), name ) ) {
			return resArr[i];
		}
	}
	GLASSERT( 0 );
	return 0;
}


AnimationResource::AnimationResource( const gamedb::Item* _item )
{
	item = _item;
	name = item->Name();
	nAnimations = item->NumChildren();
}


const char* AnimationResource::AnimationName( int index ) const
{
	GLASSERT( index >= 0 && index < nAnimations );
	return item->Child( index )->Name();
}


bool AnimationResource::HasAnimation( const char* name ) const
{
	return item->Child( name ) != 0;
}


bool AnimationResource::GetTransform( const char* animationName, U32 timeClock, BoneData* boneData ) const
{
	const gamedb::Item* animItem = item->Child( animationName );
	GLASSERT( animItem );
	memset( boneData, 0, sizeof( *boneData ));
	
	float totalTimeF = animItem->GetFloat( "totalDuration" );
	int totalTime = (int)(totalTimeF*1000.0);
	int time = timeClock % totalTime;
	float fraction = 0;
	const gamedb::Item* frameItem = item->Child( 0 );

	for( int frame=0; frame<item->NumChildren(); ++frame ) {
		frameItem = item->Child( frame );
		GLASSERT( frameItem );

		int frameDuration = frameItem->GetInt( "duration" );
		if ( time < frameDuration ) {
			// We found the frame!
			fraction = (float)time / (float)frameDuration;
			break;
		}
		time -= frameDuration;
	}
	
	for( int i=0; i<frameItem->NumChildren(); ++i ) {
		GLASSERT( i < EL_MAX_BONES );
		const gamedb::Item* boneItem = animItem->Child( i );
		boneData->bone[i].angleRadians	= ToRadian( boneItem->GetFloat( "angle" ));
		boneData->bone[i].dy			= ToRadian( boneItem->GetFloat( "dy" ));
		boneData->bone[i].dz			= ToRadian( boneItem->GetFloat( "dz" ));
	}
	return true;
}

