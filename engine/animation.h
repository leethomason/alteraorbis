#ifndef XENOENGINE_ANIMATION_INCLUDED
#define XENOENGINE_ANIMATION_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

#include "../shared/gamedbreader.h"

#include "vertex.h"

struct ModelHeader;

enum AnimationType {
	ANIM_OFF = -1,

	ANIM_REFERENCE,
	ANIM_STAND = ANIM_REFERENCE,

	ANIM_WALK,
	ANIM_GUN_WALK,
	ANIM_GUN_STAND,
	ANIM_MELEE,
	ANIM_LIGHT_IMPACT,
	ANIM_HEAVY_IMPACT,
	ANIM_COUNT
};


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

	// Used in the UI/debugging
	static const char* TypeToName( AnimationType type );	// Convert type to name
	static AnimationType NameToType( const char* name );

	// Name of the resource, not the animation
	const char* ResourceName() const		{ return resName; }
	int NumAnimations() const				{ return nAnimations; }

	bool		HasAnimation( AnimationType type ) const;
	const char* AnimationName( int index ) const;			// Get the name of animation at this location
	
	U32			Duration( AnimationType name ) const;

	bool GetTransform(	AnimationType type,			// which animation to play: "reference", "gunrun", etc.
						const ModelHeader& header,	// used to get the bone IDs
						U32 time,					// time for this animation
						BoneData* boneData ) const;

	bool GetTransform(	AnimationType type,			// which animation to play: "reference", "gunrun", etc.
						const char* boneName,
						const ModelHeader& header,	// used to get the bone IDs
						U32 time,					// time for this animation
						BoneData::Bone* bone ) const;

	void GetMetaData(	AnimationType type,
						U32 t0, U32 t1,				// t1 > t0
						grinliz::CArray<AnimationMetaData, EL_MAX_METADATA>* data ) const;

	static bool Looping( AnimationType type );
	// does this animation sync to the loop cycle?
	static bool Synchronized( AnimationType type );

private:
	U32 TimeInRange( AnimationType type, U32 t ) const;	
	void ComputeFrame( AnimationType type,
					   U32 time,
					   int *_frame0, int* _frame1, float* _fraction ) const;
	void ComputeBone( AnimationType type,
					  int frame0, int frame1, float fraction,
					  int i,
					  BoneData::Bone* bone ) const;

	const char* resName;
	int nAnimations;

	const gamedb::Item* item;

	struct Frame {
		U32			duration;
		U32			start;
		U32			end;
		const char* boneName[EL_MAX_BONES];
		U32			boneHash[EL_MAX_BONES];
		BoneData	boneData;
	};

	struct Sequence {
		U32					totalDuration;
		const gamedb::Item* item;
		int					nFrames;
		int					nBones;
		Frame				frame[EL_MAX_ANIM_FRAMES];
	};
	Sequence sequence[ANIM_COUNT];
};


class AnimationResourceManager
{
public:
	static AnimationResourceManager* Instance() { return instance; }

	static void Create();
	static void Destroy();

	void Load( const gamedb::Reader* reader );

	const AnimationResource* GetResource( const char* name );
	bool HasResource( const char* name );

private:
	AnimationResourceManager();
	~AnimationResourceManager();

	static AnimationResourceManager* instance;
	
	grinliz::HashTable< const char*, AnimationResource*, grinliz::CompCharPtr, grinliz::OwnedPtrSem > resArr;
};


#endif // XENOENGINE_ANIMATION_INCLUDED