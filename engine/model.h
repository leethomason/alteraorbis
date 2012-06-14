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
#include "../shared/glmap.h"
#include "../shared/gamedbreader.h"
#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "gpustatemanager.h"

class Texture;
class SpaceTree;
class RenderQueue;
class GPUShader;
class EngineShaders;
class Chit;

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


struct ModelHeader
{
	struct MetaData {
		grinliz::CStr< EL_METADATA_NAME_LEN > name;
		grinliz::Vector3F					  value;
	};
	struct BoneDesc {
		grinliz::CStr< EL_BONE_NAME_LEN > name;
		int								  id;
	};

	// flags
	enum {
		RESOURCE_NO_SHADOW	= 0x08,		// model casts no shadow
	};
	bool NoShadow() const			{ return (flags&RESOURCE_NO_SHADOW) ? true : false; }

	grinliz::CStr<EL_FILE_STRING_LEN>	name;
	U16						nTotalVertices;		// in all atoms
	U16						nTotalIndices;
	U16						flags;
	U16						nAtoms;
	grinliz::Rectangle3F	bounds;
	MetaData				metaData[EL_MAX_METADATA];
	BoneDesc				boneName[EL_MAX_BONES];		// ordered by name, not ID

	const char* BoneNameFromID( int id ) const {
		for( int i=0; i<EL_MAX_BONES; ++i ) {
			if ( boneName[i].id == id ) {
				return boneName[i].name.c_str();
			}
		}
		return 0;
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

	bool GetMetaData( const char* name, grinliz::Vector3F* value ) const;


	ModelHeader header;						// loaded
	grinliz::Rectangle3F	hitBounds;		// for picking - a bounds approximation
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

private:
	enum { 
		MAX_MODELS = 200	// just pointers
	};
	ModelResourceManager();
	~ModelResourceManager();

	static ModelResourceManager* instance;
	grinliz::CArray< ModelResource*, MAX_MODELS > modelResArr;
	CStringMap<	ModelResource* > map;
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

	void SetRotation( float rot, int axis=1 );
	float GetRotation( int axis=1 ) const			{ return rot[axis]; }

	void SetPosAndYRotation( const grinliz::Vector3F& pos, float yRot );

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

	void CalcMeta( const char* name, grinliz::Vector3F* meta ) const;
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

	// Set by the engine. Any xform change will set this
	// to (-1,-1)-(-1,-1) to then be set by the engine.
	//grinliz::Rectangle2I mapBoundsCache;

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

	SpaceTree* tree;
	const ModelResource* resource;
	grinliz::Vector3F pos;
	float rot[3];
	float debugScale;

	grinliz::Vector4F	param[EL_MAX_MODEL_GROUPS];

	int flags;

	mutable bool xformValid;
	mutable bool invValid;

	mutable grinliz::Rectangle3F aabb;
	mutable grinliz::Matrix4 _xform;
	mutable grinliz::Matrix4 _invXForm;
};


#endif // UFOATTACK_MODEL_INCLUDED