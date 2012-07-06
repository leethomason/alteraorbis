#include "animation.h"
#include "../grinliz/glstringutil.h"
#include "../shared/gamedbreader.h"
#include "model.h"

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


bool AnimationResource::GetTransform( const char* animationName, const ModelHeader& header, U32 timeClock, BoneData* boneData ) const
{
	const gamedb::Item* animItem = item->Child( animationName );
	GLASSERT( animItem );
	memset( boneData, 0, sizeof( *boneData ));
	
	float totalTime = animItem->GetFloat( "totalDuration" );
	//GLOUTPUT(( "animItem %s\n", animItem->Name() ));

	float time = fmodf( (float)timeClock, totalTime );
	float fraction = 0;

	const gamedb::Item* frameItem = 0;
	for( int frame=0; frame<animItem->NumChildren(); ++frame ) {
		frameItem = animItem->Child( frame );
		GLASSERT( frameItem );
		//GLOUTPUT(( "frameItem %s\n", frameItem->Name() ));

		float frameDuration = frameItem->GetFloat( "duration" );
		if ( (float)time < frameDuration ) {
			// We found the frame!
			fraction = (float)time / (float)frameDuration;
			break;
		}
		time -= frameDuration;
	}
	GLASSERT( frameItem );

	for( int i=0; i<frameItem->NumChildren(); ++i ) {
		GLASSERT( i < EL_MAX_BONES );
		const gamedb::Item* boneItem = frameItem->Child( i );

		const char* boneName = boneItem->Name();
		int index = header.BoneIDFromName( boneName );
		//GLASSERT( index >= 0 );

		boneData->name[index] = boneName;
		//boneData->bone[index].angleRadians	= ToRadian( boneItem->GetFloat( "angle" ));
		boneData->bone[index].angleRadians = 0;
		boneData->bone[index].dy			= boneItem->GetFloat( "dy" )*200.f;
		boneData->bone[index].dz			= boneItem->GetFloat( "dz" )*200.f;
	}
	return true;
}

