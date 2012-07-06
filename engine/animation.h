#ifndef XENOENGINE_ANIMATION_INCLUDED
#define XENOENGINE_ANIMATION_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

#include "../shared/gamedbreader.h"

#include "vertex.h"


class AnimationResource
{
public:
	AnimationResource( const gamedb::Item* animationItem );
	~AnimationResource()	{}

	const char* Name() const		{ return name; }	// the name of the resource.
	int NumAnimations() const		{ return nAnimations; }

	bool		HasAnimation( const char* name ) const;
	const char* AnimationName( int index ) const;

	bool GetTransform( const char* animationName, U32 time, BoneData* boneData ) const;

private:
	const char* name;
	int nAnimations;

	const gamedb::Item* item;
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