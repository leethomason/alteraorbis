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

	const gamedb::Item* frameItem0 = 0;
	const gamedb::Item* frameItem1 = 0;

	int frame0 = 0;
	int frame1 = 0;

	for( frame0=0; frame0<animItem->NumChildren(); ++frame0 ) {
		frameItem0 = animItem->Child( frame0 );
		GLASSERT( frameItem0 );
		//GLOUTPUT(( "frameItem %s\n", frameItem->Name() ));

		float frameDuration = frameItem0->GetFloat( "duration" );
		if ( (float)time < frameDuration ) {
			// We found the frame!
			fraction = (float)time / (float)frameDuration;
			frame1 = (frame0 + 1) % animItem->NumChildren();
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
		//const gamedb::Item* boneItem1 = boneItem0;	// disable interpolation

		const char* boneName = boneItem0->Name();
		int index = header.BoneIDFromName( boneName );
		//GLASSERT( index >= 0 );

		boneData->name[index] = boneName;

		float angle0 = boneItem0->GetFloat( "anglePrime" );
		float angle1 = boneItem1->GetFloat( "anglePrime" );

		float add = 0.0f;
		if ( fabsf( angle0-angle1 ) > 180.0f ) {
			if ( angle1 < angle0 ) angle1 += 360.0f;
			else				   angle1 -= 360.0f;
		}
		float angle = Lerp( angle0, angle1, fraction );
		//if ( angle < 0 ) angle += 360.0f;
		//if ( angle >= 360.0f ) angle -= 360.0f;

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

