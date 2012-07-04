#ifndef XENOENGINE_ANIMATION_INCLUDED
#define XENOENGINE_ANIMATION_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../shared/gamedbreader.h"


struct AnimationXForm
{
	int boneID;
	const char* boneName;

	float dx;
	float dy;
	float angle;
};


class AnimationResource
{
public:
	AnimationResource( const gamedb::Item* animationItem );
	~AnimationResource();

	const char* Name();	// the name of the resource.

	bool GetTransform( const char* animationName, U32 time, int bone, AnimationXForm* xform );
};


#endif // XENOENGINE_ANIMATION_INCLUDED