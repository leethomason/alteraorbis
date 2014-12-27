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

#ifndef UFOATTACK_MODEL_INCLUDED
#define UFOATTACK_MODEL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glmemorypool.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "gpustatemanager.h"
#include "animation.h"

class Texture;
class SpaceTree;
class RenderQueue;
class GPUState;
struct GPUStream;
struct GPUStreamData;
class EngineShaders;
class Chit;
class AnimationResource;
class XStream;
class ParticleSystem;


// The smallest draw unit: texture, vertex, index.
struct ModelAtom 
{
	void Init() {
		texture = 0;
		nVertex = nIndex = 0;
		index = 0;
		vertex = 0;
		vertexBuffer = 0;
		indexBuffer = 0;
	}

	void Free() {
		DeviceLoss();
		delete [] index;
		delete [] vertex;
		delete vertexBuffer;
		delete indexBuffer;
		Init();
	}

	void DeviceLoss() {
		delete vertexBuffer;
		delete indexBuffer;
		vertexBuffer = 0;
		indexBuffer = 0;
	}

	Texture* texture;
	mutable GPUVertexBuffer* vertexBuffer;		// created on demand, hence 'mutable'
	mutable GPUIndexBuffer*  indexBuffer;

	void Bind( GPUStream* stream, GPUStreamData* data ) const;

	U32		nVertex;
	U32		nIndex;
	U16*	index;
	Vertex* vertex;
};


struct ModelMetaData {
	bool InUse() const { return !pos.IsZero(); }
	grinliz::Vector3F		pos;		// position (0,0,0) is not in use.
	grinliz::Vector3F		axis;		// axis of rotation (if any)
	float					rotation;	// amount of rotation (if any)
	grinliz::IString		boneName;	// name of the bone this metadata is attached to (if any)
};


struct ModelParticleEffect {
	int					metaData;	// id of the metadata
	grinliz::IString	name;		// name of the particle effect.
};

struct ModelHeader
{
	// flags
	enum {
		RESOURCE_NO_SHADOW	= 0x08,		// model casts no shadow
	};
	bool NoShadow() const			{ return (flags&RESOURCE_NO_SHADOW) ? true : false; }

	grinliz::IString		name;
	grinliz::IString		animation;			// the name of the animation, if exists, for this model.
	float					animationSpeed;
	U16						nTotalVertices;		// in all atoms
	U16						nTotalIndices;
	U16						flags;
	U16						nAtoms;
	grinliz::Rectangle3F	bounds;
	ModelMetaData			metaData[EL_NUM_METADATA];
	ModelParticleEffect		effectData[EL_MAX_MODEL_EFFECTS];

	/* The model bone names: these are the names (and the ID)
	   actually found in the model creation. For example,
	   if 'torso' is at index 5, then the ID for torso
	   is 5, and the uniform for the torso transform should
	   be at uniformMat[5].
	*/
	grinliz::IString		modelBoneName[EL_MAX_BONES];

	/*	The armature uses different IDs. This maps from
	    the model IDs to the armature IDs so the armature
		mats can be re-ordered.
	*/
	int						modelToArmatureMap[EL_MAX_BONES];

	void Set( const grinliz::IString& name, int nGroups, int nTotalVertices, int nTotalIndices,
			  const grinliz::Rectangle3F& bounds );

	void Load(	const gamedb::Item* );		// database connection
	void Save(	gamedb::WItem* parent );	// database connection
};


class ModelResource
{
public:
	ModelResource()		{ memset( &header, 0, sizeof( header ) ); }
	~ModelResource()	{ Free(); }

	const grinliz::Rectangle3F& AABB() const	{ return header.bounds; }

	void Free();
	void DeviceLoss();

	// In the coordinate space of the resource! (Object space.)
	int Intersect(	const grinliz::Vector3F& point,
					const grinliz::Vector3F& dir,
					grinliz::Vector3F* intersect ) const;

	const ModelMetaData* GetMetaData( int i ) const { GLASSERT( i>= 0 && i < EL_NUM_METADATA ); return &header.metaData[i]; }

	const char*			Name() const	{ return header.name.c_str(); }
	grinliz::IString	IName() const	{ return header.name; }

	ModelHeader				header;				// loaded
	grinliz::Rectangle3F	hitBounds;			// for picking - a bounds approximation
	grinliz::Rectangle3F	invariantBounds;	// build y rotation in, so that bounds can
												// be generated with simple addition, without
												// causing the call to Model::XForm 
	/* Matches the bone names; the average location. */
	grinliz::Vector3F		boneCentroid[EL_MAX_BONES];

	ModelAtom atom[EL_MAX_MODEL_GROUPS];
};


struct ModelAuxBone {
	ModelAuxBone() {
		for( int i=0; i<EL_MAX_BONES; ++i ) {
			animToModelMap[i] = 0;
		}
	}

	int					animToModelMap[EL_MAX_BONES];
	grinliz::Matrix4	boneMats[EL_MAX_BONES];
};


struct ModelAuxTex {
	ModelAuxTex() {
		texture0XForm.Zero();
		texture0Clip.Zero();
	}

	grinliz::Vector4F	texture0XForm;
	grinliz::Vector4F	texture0Clip;
	grinliz::Matrix4	texture0ColorMap;
};



class ModelResourceManager
{
public:
	static ModelResourceManager* Instance()	{ GLASSERT( instance ); return instance; }
	
	void  AddModelResource( ModelResource* res );
	const ModelResource* GetModelResource( const char* name, bool errorIfNotFound=true );
	void DeviceLoss();

	static void Create();
	static void Destroy();

	const ModelResource* const* GetAllResources( int *count ) const {
		*count = modelResArr.Size();
		return modelResArr.Mem();
	}

	grinliz::MemoryPoolT< ModelAuxBone > modelAuxBonePool;
	grinliz::MemoryPoolT< ModelAuxTex > modelAuxTexPool;

private:
	enum { 
		MAX_MODELS = 200	// just pointers
	};
	ModelResourceManager();
	~ModelResourceManager();

	static ModelResourceManager* instance;
	grinliz::CArray< ModelResource*, MAX_MODELS > modelResArr;
	grinliz::HashTable<	const char*, ModelResource*, grinliz::CompCharPtr > map;
};


class ModelLoader
{
public:
	ModelLoader() 	{}
	~ModelLoader()	{}

	void Load( const gamedb::Item*, ModelResource* res );

private:
	void LoadAtom( const gamedb::Item* item, int index, ModelResource* res );
	grinliz::CDynArray<Vertex> vBuffer;
	grinliz::CDynArray<U16> iBuffer;
};


class Model
{
public:
	Model();
	~Model();

	void Init( const ModelResource* resource, SpaceTree* tree );
	void Free();

	void Serialize( XStream* xs, SpaceTree* tree );

	void Queue( RenderQueue* queue, EngineShaders* shaders, int requiredShaderFlag, int excludedShaderFlag );

	enum {
		MODEL_HAS_COLOR				= (1<<1),
		MODEL_HAS_BONE_FILTER		= (1<<2),
		MODEL_NO_SHADOW				= (1<<3),
		MODEL_INVISIBLE				= (1<<4),
		MODEL_TEXTURE0_XFORM		= (1<<5),
		MODEL_TEXTURE0_CLIP			= (1<<6),
		MODEL_TEXTURE0_COLORMAP		= (1<<7),
		MODEL_UI_TRACK				= (1<<8),

		MODEL_USER					= (1<<16)		// reserved for user code.
	};

	bool CastsShadow() const		{ return (flags & MODEL_NO_SHADOW) == 0; }
	int IsFlagSet( int f ) const	{ return flags & f; }
	void SetFlag( int f )			{ flags |= f; }
	void ClearFlag( int f )			{ flags &= (~f); }
	int Flags()	const				{ return flags; }

	const grinliz::Vector3F& Pos() const			{ return pos; }
	void SetPos( const grinliz::Vector3F& pos );
	void SetPos( float x, float y, float z )		{ grinliz::Vector3F vec = { x, y, z }; SetPos( vec ); }
	float X() const { return pos.x; }
	float Y() const { return pos.y; }
	float Z() const { return pos.z; }

	void SetRotation( const grinliz::Quaternion& rot );
	void SetYRotation( float yRot ) {
		grinliz::Quaternion q;
		static const grinliz::Vector3F UP = {0,1,0};
		q.FromAxisAngle( UP, yRot );
		SetRotation( q );
	}
	const grinliz::Quaternion& GetRotation() const			{ return rot; }
	void SetPosAndYRotation( const grinliz::Vector3F& pos, float yRot );
	void SetTransform( const grinliz::Matrix4& mat );

	// Get the animation resource, if it exists.
	const AnimationResource* GetAnimationResource() const	{ return animationResource; }
	// Set the current animation, null or "reference" turns off animation.
	// Does nothing if the 'id' is the currently playing animation
	void SetAnimation( int id, U32 crossFade, bool restart );
	int GetAnimation() const						{ return currentAnim.id; }
	bool AnimationDone() const;

	// Get the bone name or index from the *model*, not the animation.
	grinliz::IString GetBoneName( int i ) const;
	int GetBoneIndex( grinliz::IString name ) const;

	// Update the time and animation rendering.
	void DeltaAnimation(	U32 time, 
							int *metaData,
							bool *done );
	// debugging
	void SetAnimationTime( U32 time );

	void SetAnimationRate( float rate )						{ animationRate = rate; }
	float GetAnimationRate() const							{ return animationRate; }
	bool HasAnimation() const								{ return animationResource && (currentAnim.id>=0); }

	// WARNING: not really supported. Just for debug rendering. May break:
	// normals, lighting, bounds, picking, etc. etc.
	void SetScale( float s );
	float GetScale() const							{ return debugScale; }
	
	void SetColor( const grinliz::Vector4F& color ) {
		SetFlag( MODEL_HAS_COLOR );
		this->color = color;
	}
	bool HasColor() const {
		return IsFlagSet( MODEL_HAS_COLOR ) != 0;
	}

	// 4 ids. Null to clear.
	void SetBoneFilter( const int* boneID ) {
		if ( boneID 
			&& (boneID[0] >= 0 || boneID[1] >= 0 || boneID[2] >= 0 || boneID[3] >= 0 ) ) 
		{
			boneFilter.Set(  (float)boneID[0], (float)boneID[1], (float)boneID[2], (float)boneID[3] );
			SetFlag( MODEL_HAS_BONE_FILTER );
		}
		else {
			ClearFlag( MODEL_HAS_BONE_FILTER );
		}
	}
	// [4]
	void SetBoneFilter( const grinliz::IString* names, const bool* filter );

	bool HasBoneFilter() const {
		return IsFlagSet( MODEL_HAS_BONE_FILTER ) != 0;
	}

	void SetTextureXForm( float a=1, float d=1, float tx=0, float ty=0 );
	void SetTextureXForm( const grinliz::Vector4F& v ) {
		SetTextureXForm( v.x, v.y, v.z, v.w );
	}

	void SetTextureClip( float x0=0, float y0=0, float x1=1, float y1=1 );
	void SetColorMap(	const grinliz::Vector4F& red, 
						const grinliz::Vector4F& green, 
						const grinliz::Vector4F& blue,
						float alpha );	// 0: use passed in channels, 1: use sampler
	void SetColorMap( const grinliz::Matrix4& mat );

	// 1.0 is normal color
	void SetFadeFX( float v )						{ control.x = v; }
	void SetSaturation( float v )					{ control.y = v; }

	// AABB for user selection (bigger than the true AABB)
	void CalcHitAABB( grinliz::Rectangle3F* aabb ) const;

	// The bounding box
	// Accurate, but can be expensive to call a lot.
	const grinliz::Rectangle3F& AABB() const;

	// Bigger bounding box, cheaper call.
	grinliz::Rectangle3F GetInvariantAABB( float xPerY, float zPerY ) const {
		grinliz::Rectangle3F b = resource->invariantBounds;
		b.min += pos;
		b.max += pos;
		
		if ( CastsShadow() ) {
			if ( xPerY > 0 )
				b.max.x += xPerY * b.max.y;
			else
				b.min.x += xPerY * b.max.y;

			if ( zPerY > 0 )
				b.max.z += zPerY * b.max.y;
			else
				b.min.z += zPerY * b.max.y;
		}
		return b;
	}

	void CalcMetaData( int id, grinliz::Matrix4* xform );
	bool HasParticles() const {  return hasParticles; }
	void EmitParticles( ParticleSystem* system, const grinliz::Vector3F* eyeDir, U32 deltaTime );
	void CalcTargetSize( float* width, float* height ) const;

	// Returns grinliz::INTERSECT or grinliz::REJECT
	int IntersectRay(	bool testTriangles,					// true: test each triangle, false: bounding box only
						const grinliz::Vector3F& origin, 
						const grinliz::Vector3F& dir,
						grinliz::Vector3F* intersect ) const;

	const ModelResource* GetResource() const	{ return resource; }
	bool Sentinel()	const						{ return resource==0 && tree==0; }

	Model* next;			// used by the SpaceTree query
	Model* next0;			// used by the Engine sub-sorting
	Chit*  userData;		// really should be void* - but types are nice.

	const grinliz::Matrix4& XForm() const;

	void Modify() 
	{			
		xformValid = false; 
		invValid = false; 
	}

private:
	const grinliz::Matrix4& InvXForm() const;

	void CalcAnimation();
	//void CalcAnimation( BoneData::Bone* bone, grinliz::IString boneName ) const;	// compute the animition, accounting for crossfade, etc.
	//void CrossFade( float fraction, BoneData::Bone* inOut, const BoneData::Bone& prev ) const;

	SpaceTree* tree;
	const ModelResource* resource;

	grinliz::Vector3F	pos;
	grinliz::Quaternion rot;
	
	float debugScale;
	
	struct AnimationState {
		void Init() { time=0; id=ANIM_OFF; }
		bool operator==(const AnimationState& rhs ) const { return time == rhs.time && id == rhs.id; }

		U32				time;
		int				id;
	};
	AnimationState	currentAnim;
	AnimationState	prevAnim;
	AnimationState	cachedAnim;

	float animationRate;
	U32 totalCrossFadeTime;	// how much time the crossfade will use
	U32 crossFadeTime;

	const AnimationResource* animationResource;
	bool hasParticles;
	int flags;

	grinliz::Vector4F	color;
	grinliz::Vector4F	boneFilter;
	grinliz::Vector4F	control;
	ModelAuxBone* auxBone;
	ModelAuxTex*  auxTex;


	mutable bool xformValid;
	mutable bool invValid;

	mutable grinliz::Rectangle3F aabb;
	mutable grinliz::Matrix4 _xform;
	mutable grinliz::Matrix4 _invXForm;
};


#endif // UFOATTACK_MODEL_INCLUDED