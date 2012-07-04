#ifndef XENOENGINE_ANIMATION_INCLUDED
#define XENOENGINE_ANIMATION_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

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
	~AnimationResource()	{}

	const char* Name() const { return name; }	// the name of the resource.

	bool GetTransform( const char* animationName, U32 time, int bone, AnimationXForm* xform ) const;

private:
	const char* name;
};


class AnimationResourceManager
{
public:
	static AnimationResourceManager* Instance() { return instance; }

	static void Create();
	static void Destroy();

	void Load( const gamedb::Reader* reader );
	const AnimationResource* GetResource( const char* name );

private:
	AnimationResourceManager();
	~AnimationResourceManager();

	static AnimationResourceManager* instance;

	grinliz::CDynArray< AnimationResource* > resArr;
};


#endif // XENOENGINE_ANIMATION_INCLUDED