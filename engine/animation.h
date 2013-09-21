/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef XENOENGINE_ANIMATION_INCLUDED
#define XENOENGINE_ANIMATION_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"

#include "../shared/gamedbreader.h"
#include "../engine/enginelimits.h"

#include "vertex.h"

struct ModelHeader;

enum  {
	ANIM_OFF = -1,

	ANIM_STAND,
	ANIM_WALK,
	ANIM_GUN_STAND,
	ANIM_GUN_WALK,
	ANIM_MELEE,
	ANIM_IMPACT,
	ANIM_COUNT
};

enum {
	ANIM_META_NONE = 0,
	ANIM_META_IMPACT,		// strike event during melee
	ANIM_META_COUNT
};



class AnimationResource
{
public:
	AnimationResource( const gamedb::Item* animationItem );
	~AnimationResource()	{}

	// Used in the UI/debugging
	static const char* TypeToName( int type );	// Convert type to name
	static int NameToType( const char* name );

	// Name of the resource, not the animation
	const char* ResourceName() const		{ return resName; }
	int NumAnimations() const				{ return nAnimations; }

	bool		HasAnimation( int type ) const;
	const char* AnimationName( int index ) const;			// Get the name of animation at this location
	
	U32			Duration( int name ) const;

	// Compute the bones, with optional cross fade.
	void GetTransform(	int typeA,					// which animation to play: "reference", "gunrun", etc.
						U32 timeA,					// time for this animation
						int typeB,					// 2nd animation
						U32 timeB,					// 2nd animation time
						float crossFraction,		// 0: all A, 1: all B
#ifdef EL_VEC_BONES
						grinliz::Vector4F* outPos, grinliz::Quaternion* outRot ) const;
#else
						grinliz::Matrix4* out ) const;	// At least EL_MAX_BONES
#endif

	int GetMetaData(	int type,
						U32 t0, U32 t1 ) const;	// t1 > t0

	const BoneData& GetBoneData( int type ) const { return sequence[type].boneData; }

	// does this animation loop?
	static bool Looping( int type );
	// does this animation sync to the walk cycle?
	static bool Synchronized( int type );
	// does this animation represent motion?
	static bool Motion( int type );

private:
	void RecBoneWalk( const gamedb::Item* boneItem, int frame, int *boneIndex, BoneData* boneData );
	void ComputeFrame( int type,
					   U32 time,
					   int *_frame0, int* _frame1, float* _fraction ) const;

	const char* resName;
	int nAnimations;

	const gamedb::Item* item;

	struct Sequence {
		const gamedb::Item* item;
		// The animation sequence uses equally timed frames.
		U32					totalDuration;
		int					nFrames;
		int					nBones;
		BoneData			boneData;
		int					metaDataID;
		U32					metaDataTime;
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
	
	AnimationResource* nullAnimation;
	grinliz::HashTable< const char*, AnimationResource*, grinliz::CompCharPtr, grinliz::OwnedPtrSem > resArr;
};


#endif // XENOENGINE_ANIMATION_INCLUDED