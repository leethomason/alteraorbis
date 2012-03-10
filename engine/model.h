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
#include "../shared/glmap.h"
#include "../shared/gamedbreader.h"
#include "vertex.h"
#include "enginelimits.h"
#include "serialize.h"
#include "ufoutil.h"
#include "gpustatemanager.h"

class Texture;
class SpaceTree;
class RenderQueue;
class GPUShader;

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
	grinliz::Vector3F		trigger;			// location for gun
	float					eye;				// location model "looks from"
	float					target;				// height of chest shot

	void Set( const char* name, int nGroups, int nTotalVertices, int nTotalIndices,
			  const grinliz::Rectangle3F& bounds );

	void Load(	const gamedb::Item* );	// database connection
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


	ModelHeader header;						// loaded

	grinliz::Rectangle3F	hitBounds;		// for picking - a bounds approximation
//	int						instances;		// # of times the model is repeated.
//	U16*					allIndex;		// memory store for vertices and indices. Used for hit-testing.
//	Vertex*					allVertex;

	ModelAtom atom[EL_MAX_MODEL_GROUPS];
};


struct AuxTextureXForm {
	void Init()	{ for( int i=0; i<EL_MAX_MODEL_GROUPS; ++i ) m[i].SetIdentity(); }
	grinliz::Matrix4 m[EL_MAX_MODEL_GROUPS];
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

	AuxTextureXForm* Alloc()		{ return (AuxTextureXForm*) auxPool.Alloc(); }
	void Free( AuxTextureXForm* a )	{ auxPool.Free( a ); }

private:
	enum { 
		MAX_MODELS = 200	// just pointers
	};
	ModelResourceManager();
	~ModelResourceManager();

	static ModelResourceManager* instance;
	CArray< ModelResource*, MAX_MODELS > modelResArr;
	CStringMap<	ModelResource* > map;
	grinliz::MemoryPool auxPool;
};


class ModelLoader
{
public:
	ModelLoader() 	{}
	~ModelLoader()	{}

	void Load( const gamedb::Item*, ModelResource* res );

private:
	void LoadAtom( const gamedb::Item* item, int index, ModelResource* res );
	CDynArray<Vertex> vBuffer;
	CDynArray<U16> iBuffer;
};


class Model
{
public:
	Model();
	~Model();

	void Init( const ModelResource* resource, SpaceTree* tree );
	void Free();

	void Queue( RenderQueue* queue, GPUShader* opaque, GPUShader* transparent, Texture* replaceAllTextures );

	enum {
		MODEL_SELECTABLE			= 0x01,
		MODEL_HIDDEN_FROM_TREE		= 0x02,
		MODEL_OWNED_BY_MAP			= 0x04,
		MODEL_NO_SHADOW				= 0x08,
		MODEL_INVISIBLE				= 0x10,
		MODEL_METADATA				= 0x80,		// mapmaker data that isn't displayed in-game

		// RESERVED!
		// MODEL_local				= 0x00010000 and higher.

		MODEL_TEXTURE_MATS			= 2
	};

	int IsFlagSet( int f ) const	{ return flags & f; }
	void SetFlag( int f )			{ flags |= f; }
	void ClearFlag( int f )			{ flags &= (~f); }
	int Flags()	const				{ return flags; }

	// Can this be put in a render cache? Yes if owned by the
	// rarely-changing map.
	// FIXME: also not cacheable if contains any alpha texture
	bool Cacheable() const			{ return IsFlagSet( MODEL_OWNED_BY_MAP ) != 0; };

	const grinliz::Vector3F& Pos() const			{ return pos; }
	void SetPos( const grinliz::Vector3F& pos );
	void SetPos( float x, float y, float z )		{ grinliz::Vector3F vec = { x, y, z }; SetPos( vec ); }
	float X() const { return pos.x; }
	float Y() const { return pos.y; }
	float Z() const { return pos.z; }

	void SetRotation( float rot, int axis=1 );
	float GetRotation( int axis=1 ) const			{ return rot[axis]; }
	
	// Set the skin texture (which is a special texture xform)
	//void SetSkin(int gender, int armor, int appearance);
	// Set the texture xform for rendering tricks
	//void SetTexXForm( int index, float a=1.0f, float d=1.0f, float x=0.0f, float y=0.0f );

	// Set the texture.
	void SetTexture( Texture* t )	{ setTexture = t; }

	// AABB for user selection (bigger than the true AABB)
	void CalcHitAABB( grinliz::Rectangle3F* aabb ) const;

	// The bounding box
	const grinliz::Rectangle3F& AABB() const;

	void CalcTrigger( grinliz::Vector3F* trigger, const float* alternateYRotation=0 ) const;
	void CalcTarget( grinliz::Vector3F* target ) const;
	void CalcTargetSize( float* width, float* height ) const;

	// Returns grinliz::INTERSECT or grinliz::REJECT
	int IntersectRay(	const grinliz::Vector3F& origin, 
						const grinliz::Vector3F& dir,
						grinliz::Vector3F* intersect ) const;

	const ModelResource* GetResource() const	{ return resource; }
	bool Sentinel()	const						{ return resource==0 && tree==0; }

	Model* next;			// used by the SpaceTree query
	Model* next0;			// used by the Engine sub-sorting
	
	// Set by the engine. Any xform change will set this
	// to (-1,-1)-(-1,-1) to then be set by the engine.
	//grinliz::Rectangle2I mapBoundsCache;

	const grinliz::Matrix4& XForm() const;
	bool HasTextureXForm( int i ) const;


/*	void AddIndices( CDynArray<U16>* indexArr, int atomIndex ) const;
	enum {
		CACHE_UNINITIALIZED = -1,
		DO_NOT_CACHE = -2
	};
	int cacheStart[EL_MAX_MODEL_GROUPS];
*/

private:
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

	AuxTextureXForm		*auxTexture;	// if allocated, then this has texture xforms. Comes from the ModelResourceManager MemoryPool.
	Texture				*setTexture;	// changes the texture, based on texBehavior


	int flags;

	mutable bool xformValid;
	mutable bool invValid;

	mutable grinliz::Rectangle3F aabb;
	mutable grinliz::Matrix4 _xform;
	mutable grinliz::Matrix4 _invXForm;
};


#endif // UFOATTACK_MODEL_INCLUDED