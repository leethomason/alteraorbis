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
#include "../shared/gamedbreader.h"
#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "gpustatemanager.h"
#include "animation.h"

class Texture;
class SpaceTree;
class RenderQueue;
class GPUShader;
class EngineShaders;
class Chit;
class AnimationResource;


/*
	v0 v1 v2 v0 v1 v2									vertex (3 points x 2 instances)
	i0 i1 i2 i0 i0 i0 i0+3 i1+3 i2+3 i0+3 i0+3 i0+3		index (2 triangle x 2 instances)

	vertex size *= nInstance
	index size *= nInstance
	instance size = index size * nInstance
*/

// The smallest draw unit: texture, vertex, index.
struct ModelAtom 
{
	void Init() {
		texture = 0;
		nVertex = nIndex = 0;
		index = 0;
		vertex = 0;
		instances = 0;
	}

	void Free() {
		DeviceLoss();
		delete [] index;
		delete [] vertex;
		Init();
	}

	void DeviceLoss() {
		vertexBuffer.Destroy();
		indexBuffer.Destroy();
	}

	Texture* texture;
#	ifdef EL_USE_VBO
	mutable GPUVertexBuffer vertexBuffer;		// created on demand, hence 'mutable'
	mutable GPUIndexBuffer  indexBuffer;
#	endif

	void Bind( GPUShader* shader ) const;

	U32 nVertex;
	U32 nIndex;
	U32 instances;

	U16* index;
	InstVertex* vertex;
};


struct ModelMetaData {
	grinliz::CStr< EL_RES_NAME_LEN >	name;		// name of the metadata
	grinliz::Vector3F					pos;		// position
	grinliz::Vector3F					axis;		// axis of rotation (if any)
	float								rotation;	// amount of rotation (if any)
	grinliz::CStr< EL_RES_NAME_LEN >	boneName;	// name of the bone this metadat is attached to (if any)
};


struct ModelParticleEffect {
	int									metaData;	// index of the metadata
	grinliz::CStr< EL_RES_NAME_LEN >	name;		// name of the particle effect.
};

struct ModelHeader
{
	struct BoneDesc {
		grinliz::CStr< EL_RES_NAME_LEN >	name;
		int									id;
	};

	// flags
	enum {
		RESOURCE_NO_SHADOW	= 0x08,		// model casts no shadow
	};
	bool NoShadow() const			{ return (flags&RESOURCE_NO_SHADOW) ? true : false; }

	grinliz::CStr<EL_FILE_STRING_LEN>	name;
	grinliz::CStr<EL_FILE_STRING_LEN>	animation;		// the name of the animation, if exists, for this model.
	U16						nTotalVertices;				// in all atoms
	U16						nTotalIndices;
	U16						flags;
	U16						nAtoms;
	grinliz::Rectangle3F	bounds;
	ModelMetaData			metaData[EL_MAX_METADATA];
	ModelParticleEffect		effects[EL_MAX_MODEL_EFFECTS];
	BoneDesc				boneName[EL_MAX_BONES];		// ordered by name, not ID

	const char* BoneNameFromID( int id ) const {
		for( int i=0; i<EL_MAX_BONES; ++i ) {
			if ( boneName[i].id == id ) {
				return boneName[i].name.c_str();
			}
		}
		return 0;
	}

	int BoneIDFromName( const char* name ) const {
		for( int i=0; i<EL_MAX_BONES; ++i ) {
			if ( boneName[i].name == name ) {
				return boneName[i].id;
			}
		}
		return -1;
	}

	void Set( const char* name, int nGroups, int nTotalVertices, int nTotalIndices,
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

	const ModelMetaData* GetMetaData( const char* name ) const;
	const ModelMetaData* GetMetaData( int i ) const { GLASSERT( i>= 0 && i < EL_MAX_METADATA ); return &header.metaData[i]; }


	ModelHeader				header;				// loaded
	grinliz::Rectangle3F	hitBounds;			// for picking - a bounds approximation
	grinliz::Rectangle3F	invariantBounds;	// build y rotation in, so that bounds can
												// be generated with simple addition, without
												// causing the call to Model::XForm 
	ModelAtom atom[EL_MAX_MODEL_GROUPS];
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

	void Queue( RenderQueue* queue, EngineShaders* shaders );

	enum {
		MODEL_SELECTABLE			= 0x0001,
		MODEL_PARAM_IS_TEX_XFORM	= 0x0002,
		MODEL_PARAM_IS_COLOR		= 0x0004,
		MODEL_PARAM_IS_BONE_FILTER	= 0x0008,
		MODEL_PARAM_MASK			= MODEL_PARAM_IS_TEX_XFORM | MODEL_PARAM_IS_COLOR | MODEL_PARAM_IS_BONE_FILTER,
		MODEL_NO_SHADOW				= 0x0100,
		MODEL_INVISIBLE				= 0x0200,

		MODEL_USER					= 0x1000		// reserved for user code.
	};

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
	void SetAnimation( AnimationType id, U32 crossFade );
	AnimationType GetAnimation() const						{ return animationID; }
	// Update the time and animation rendering.
	void DeltaAnimation(	U32 time, 
							grinliz::CArray<AnimationMetaData, EL_MAX_METADATA> *metaData,
							bool *looped );

	void SetAnimationRate( float rate )						{ animationRate = rate; }
	bool HasAnimation() const								{ return animationResource && (animationID>=0); }

	// WARNING: not really supported. Just for debug rendering. May break:
	// normals, lighting, bounds, picking, etc. etc.
	void SetScale( float s );
	float GetScale() const							{ return debugScale; }
	
	// <PARAMS> Only one can be in use at a time.
	void ClearParam() {
		ClearFlag( MODEL_PARAM_MASK );
		for( int i=0; i<EL_MAX_MODEL_GROUPS; ++i ) {
			param[i].Zero();
		}
	}

	void SetTexXForm( int index, float xScale, float yScale, float dx, float dy ) { 
		grinliz::Vector4F v = { xScale, yScale, dx, dy };
		SetParam( MODEL_PARAM_IS_TEX_XFORM, index, v );
	}
	bool HasTextureXForm( int index ) const
	{
		static const grinliz::Vector4F zero = { 0, 0, 0, 0 };
		return IsFlagSet( MODEL_PARAM_IS_TEX_XFORM ) && param[index] != zero;
	}

	void SetColor( const grinliz::Vector4F& color ) {
		SetParam( MODEL_PARAM_IS_COLOR, -1, color );
	}
	bool HasColor() const {
		return IsFlagSet( MODEL_PARAM_IS_COLOR ) != 0;
	}

	void SetBoneFilter( int boneID ) {
		grinliz::Vector4F v = { (float)boneID, 0, 0, 0 };
		SetParam( MODEL_PARAM_IS_BONE_FILTER, -1, v );
	}
	bool HasBoneFilter() const {
		return IsFlagSet( MODEL_PARAM_IS_BONE_FILTER ) != 0;
	}
	//// </PARAMS>

	// AABB for user selection (bigger than the true AABB)
	void CalcHitAABB( grinliz::Rectangle3F* aabb ) const;

	// The bounding box
	// Accurate, but can be expensive to call a lot.
	const grinliz::Rectangle3F& AABB() const;

	// Bigger bounding box, cheaper call.
	grinliz::Rectangle3F GetInvariantAABB() const {
		grinliz::Rectangle3F b = resource->invariantBounds;
		b.min += pos;
		b.max += pos;
		return b;
	}

	void CalcMetaData( const char* name, grinliz::Matrix4* xform ) const;
	void CalcTargetSize( float* width, float* height ) const;

	// Returns grinliz::INTERSECT or grinliz::REJECT
	int IntersectRay(	const grinliz::Vector3F& origin, 
						const grinliz::Vector3F& dir,
						grinliz::Vector3F* intersect ) const;

	const ModelResource* GetResource() const	{ return resource; }
	bool Sentinel()	const						{ return resource==0 && tree==0; }

	Model* next;			// used by the SpaceTree query
	Model* next0;			// used by the Engine sub-sorting
	Chit*  userData;		// really should be void* - but types are nice.

	const grinliz::Matrix4& XForm() const;

private:
	void SetParam( int flag, int index, const grinliz::Vector4F& v ) {
		GLASSERT( flag & MODEL_PARAM_MASK );
		GLASSERT( index >= -1 && index < EL_MAX_MODEL_GROUPS );

		ClearFlag( MODEL_PARAM_MASK );
		SetFlag( flag ); 

		for( int i=0; i<EL_MAX_MODEL_GROUPS; ++i ) {
			param[i].Zero();
			if ( index < 0 || index == i ) {
				param[i] = v;
			}
		}
	}

	void Modify() 
	{			
		xformValid = false; 
		invValid = false; 
		//mapBoundsCache.Set( -1, -1, -1, -1 ); 
	}
	const grinliz::Matrix4& InvXForm() const;
	void CalcAnimation( BoneData* boneData ) const;	// compute the animition, accounting for crossfade, etc.
	void CalcAnimation( BoneData::Bone* bone, const char* boneName ) const;	// compute the animition, accounting for crossfade, etc.

	SpaceTree* tree;
	const ModelResource* resource;

	grinliz::Vector3F	pos;
	grinliz::Quaternion rot;
	
	float debugScale;

	U32 animationTime;
	float animationRate;
	U32 totalCrossFadeTime;
	U32 crossFadeTime;
	const AnimationResource* animationResource;
	AnimationType animationID;
	AnimationType prevAnimationID;

	grinliz::Vector4F	param[EL_MAX_MODEL_GROUPS];

	int flags;

	mutable bool xformValid;
	mutable bool invValid;

	mutable grinliz::Rectangle3F aabb;
	mutable grinliz::Matrix4 _xform;
	mutable grinliz::Matrix4 _invXForm;
};


#endif // UFOATTACK_MODEL_INCLUDED