#ifndef XENOENGINE_ANIMATION_INCLUDED
#define XENOENGINE_ANIMATION_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

#include "../shared/gamedbreader.h"

#include "vertex.h"

struct ModelHeader;


// POD
struct AnimationMetaData
{
	const char* name;
	U32			time;
};


class AnimationResource
{
public:
	AnimationResource( const gamedb::Item* animationItem );
	~AnimationResource()	{}

	const char* Name() const		{ return name; }	// the name of the resource.
	int NumAnimations() const		{ return nAnimations; }

	bool		HasAnimation( const char* name ) const;
	const char* AnimationName( int index ) const;

	bool GetTransform(	const char* animationName,	// which animation to play: "reference", "gunrun", etc.
						const ModelHeader& header,	// used to get the bone IDs
						U32 time,					// time for this animation
						BoneData* boneData ) const;

	bool GetTransform(	const char* animationName,	// which animation to play: "reference", "gunrun", etc.
						const char* boneName,
						const ModelHeader& header,	// used to get the bone IDs
						U32 time,					// time for this animation
						BoneData::Bone* bone ) const;

	void GetMetaData(	const char* animationName,
						U32 t0, U32 t1,				// t1 > t0
						grinliz::CArray<AnimationMetaData, EL_MAX_METADATA>* data ) const;

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