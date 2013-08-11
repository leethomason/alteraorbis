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

#include "vertex.h"

struct ModelHeader;

enum  {
	ANIM_OFF = -1,

	ANIM_REFERENCE,
	ANIM_STAND = ANIM_REFERENCE,

	ANIM_WALK,
	ANIM_GUN_WALK,
	ANIM_GUN_STAND,
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

	bool GetTransform(	int type,			// which animation to play: "reference", "gunrun", etc.
						const ModelHeader& header,	// used to get the bone IDs
						U32 time,					// time for this animation
						BoneData* boneData ) const;

	bool GetTransform(	int type,			// which animation to play: "reference", "gunrun", etc.
						grinliz::IString boneName,
						const ModelHeader& header,	// used to get the bone IDs
						U32 time,					// time for this animation
						BoneData::Bone* bone ) const;

	int GetMetaData(	int type,
						U32 t0, U32 t1 ) const;	// t1 > t0

	// does this animation loop?
	static bool Looping( int type );
	// does this animation sync to the walk cycle?
	static bool Synchronized( int type );
	// does this animation represent motion?
	static bool Motion( int type );

private:
	void RecBoneWalk( const gamedb::Item* boneItem, int *boneIndex, BoneData* boneData );
	U32 TimeInRange( int type, U32 t ) const;	
	void ComputeFrame( int type,
					   U32 time,
					   int *_frame0, int* _frame1, float* _fraction ) const;
	void ComputeBone( int type,
					  int frame0, int frame1, float fraction,
					  int boneIndex,
					  BoneData::Bone* bone ) const;

	const char* resName;
	int nAnimations;

	const gamedb::Item* item;

	struct Frame {
		U32			start;
		U32			end;
		BoneData	boneData;
	};

	struct Sequence {
		U32					totalDuration;
		const gamedb::Item* item;
		int					nFrames;
		int					nBones;
		Frame				frame[EL_MAX_ANIM_FRAMES];
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