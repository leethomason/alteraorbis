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

#include "model.h"
#include "texture.h"
#include "loosequadtree.h"
#include "renderQueue.h"
#include "engineshaders.h"
#include "shadermanager.h"
#include "animation.h"
#include "serialize.h"
#include "particle.h"

#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glperformance.h"
#include "../xarchive/glstreamer.h"

#include <float.h>

using namespace grinliz;


void ModelResource::Free()
{
	for( unsigned i=0; i<header.nAtoms; ++i ) {
		atom[i].Free();
	}
}


void ModelResource::DeviceLoss()
{
	for( unsigned i=0; i<header.nAtoms; ++i ) {
		atom[i].DeviceLoss();
	}
}


int ModelResource::Intersect(	const grinliz::Vector3F& point,
								const grinliz::Vector3F& dir,
								grinliz::Vector3F* intersect ) const
{
	float t;
	int result = IntersectRayAABB( point, dir, header.bounds, intersect, &t );
	if ( result == grinliz::INTERSECT || result == grinliz::INSIDE ) {
		float close2 = FLT_MAX;
		Vector3F test;

		for( unsigned i=0; i<header.nAtoms; ++i ) {
			for( unsigned j=0; j<atom[i].nIndex; j+=3 ) {
				int r = IntersectRayTri( point, dir, 
										 atom[i].vertex[ atom[i].index[j+0] ].pos,
										 atom[i].vertex[ atom[i].index[j+1] ].pos,
										 atom[i].vertex[ atom[i].index[j+2] ].pos,
										 &test );
				if ( r == grinliz::INTERSECT ) {
					float c2 =  (point.x-test.x)*(point.x-test.x) +
								(point.y-test.y)*(point.y-test.y) +
								(point.z-test.z)*(point.z-test.z);
					if ( c2 < close2 ) {
						close2 = c2;
						*intersect = test;
					}
				}
			}
		}
		if ( close2 < FLT_MAX ) {
			return grinliz::INTERSECT;
		}
	}
	return grinliz::REJECT;
}


const ModelMetaData* ModelResource::GetMetaData( grinliz::IString name ) const
{
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( name == header.metaData[i].name ) {
			return &header.metaData[i];
		}
	}
	return 0;
}


void ModelLoader::Load( const gamedb::Item* item, ModelResource* res )
{
	res->header.Load( item );

	int instances = 1;
#	ifdef XENOENGINE_INSTANCING
	instances = EL_TUNE_INSTANCE_MEM / sizeof( InstVertex );
	if ( instances > EL_MAX_INSTANCE ) 
		instances = EL_MAX_INSTANCE;
	if ( instances < 1 )
		instances = 1;		// big model.
#	endif

	// compute the hit testing AABB
	float ave = grinliz::Max( res->header.bounds.SizeX(), res->header.bounds.SizeZ() )*0.5f;
	res->hitBounds.min.Set( -ave, res->header.bounds.min.y, -ave );
	res->hitBounds.max.Set( ave, res->header.bounds.max.y, ave );

	{ 
		// Calc the invariant bounds, in the 2D plane. Won't be correct for some rotations around X or Z
		// Surround the bounds with a circle. Use the circle to create new bounds
		const Rectangle3F& bnd = res->header.bounds;
		Vector2F a = { bnd.min.x, bnd.min.z };
		Vector2F b = { bnd.min.x, bnd.max.z };
		Vector2F c = { bnd.max.x, bnd.min.z };
		Vector2F d = { bnd.max.x, bnd.max.z };
		float rad2 = Max( a.LengthSquared(), b.LengthSquared(), c.LengthSquared(), d.LengthSquared() );
		float rad = sqrtf( rad2 );

		res->invariantBounds.min.Set( -rad, res->header.bounds.min.y, -rad );
		res->invariantBounds.max.Set( rad, res->header.bounds.max.y, rad );
	}

	for( int i=0; i<EL_MAX_BONES; ++i ) {
		if ( !res->header.boneName[i].empty() ) {
			GLOUTPUT(( "Model %s bone %d=%s\n", res->header.name.c_str(), i, res->header.boneName[i].c_str() ));
		}
	}

	for( U32 i=0; i<res->header.nAtoms; ++i )
	{
		const gamedb::Item* groupItem = item->Child( i );
		LoadAtom( groupItem, i, res );
		//GLOUTPUT(( "  '%s' vertices=%d tris=%d\n", textureName, (int)res->atom[i].nVertex, (int)(res->atom[i].nIndex/3) ));
	}

	vBuffer.Clear();	
	vBuffer.PushArr( res->header.nTotalVertices );
	iBuffer.Clear();
	iBuffer.PushArr( res->header.nTotalIndices );

	item->GetData( "vertex", vBuffer.Mem(), res->header.nTotalVertices*sizeof(Vertex) );
	item->GetData( "index",  iBuffer.Mem(), res->header.nTotalIndices*sizeof(U16) );

	int iOffset = 0;
	int vOffset = 0;

	for( U32 i=0; i<res->header.nAtoms; ++i )
	{
		ModelAtom* atom = &res->atom[i];
		atom->instances = instances;
		
		atom->vertex = new InstVertex[ atom->nVertex * instances ];
		for( int inst=0; inst<instances; ++inst ) {
			for( unsigned j=0; j<atom->nVertex; ++j ) {
				atom->vertex[j+inst*atom->nVertex].From( vBuffer[j+vOffset] );
				atom->vertex[j+inst*atom->nVertex].instanceID = inst;
			}
		}
		
		atom->index  = new U16[atom->nIndex * instances ];
		for( int inst=0; inst<instances; ++inst ) {
			for( unsigned j=0; j<atom->nIndex; ++j ) {
				atom->index[j+inst*atom->nIndex] = iBuffer[j+iOffset] + inst*atom->nVertex;
			}
		}
		vOffset += atom->nVertex;
		iOffset += atom->nIndex;
	}
}


void ModelLoader::LoadAtom( const gamedb::Item* item, int i, ModelResource* res )
{	
	ModelGroup group;
	group.Load( item );

	const char* textureName = group.textureName.c_str();
	if ( !textureName[0] ) {
		textureName = "white";
	}

	GLString base, texname, extension;
	StrSplitFilename( GLString( textureName ), &base, &texname, &extension );
	Texture* t = TextureManager::Instance()->GetTexture( texname.c_str() );

	GLASSERT( t );        
	res->atom[i].Init();
	res->atom[i].texture = t;
	res->atom[i].nVertex = group.nVertex;
	res->atom[i].nIndex = group.nIndex;
}


Model::Model()		
{	
	// WARNING: in the normal case, the constructor isn't called. Models usually come from a pool!
	// new/delete and Init/Free are both valid uses.
	Init( 0, 0 ); 
}

Model::~Model()	
{	
	// WARNING: in the normal case, the destructor isn't called. Models usually come from a pool!
	// new/delete and Init/Free are both valid uses.
	Free();
}


void Model::Init( const ModelResource* resource, SpaceTree* tree )
{
	this->resource = resource; 
	this->tree = tree;
	this->aux = 0;

	color.Set(1,1,1,1);
	boneFilter.Set(0,0,0,0);
	control.Set( 1, 1, 1, 1 );

	animationResource = 0;
	if ( resource->header.animation != "create" ) {
		animationResource = AnimationResourceManager::Instance()->GetResource( resource->header.animation.c_str() );
	}

	debugScale = 1.0f;
	pos.Set( 0, 0, 0 );
	rot.Zero();
	Modify();

	if ( tree ) {
		tree->Update( this );
	}

	flags = 0;
	if ( resource && (resource->header.flags & ModelHeader::RESOURCE_NO_SHADOW ) ) {
		flags |= MODEL_NO_SHADOW;
	}
	userData = 0;

	animationRate = 1.0f;

	crossFadeTime = 0;
	totalCrossFadeTime = 0;
	currentAnim.Init();
	prevAnim.Init();

	hasParticles = false;
	for( int i=0; i<EL_MAX_MODEL_EFFECTS; ++i ) {
		const ModelParticleEffect& effect = resource->header.effectData[i];
		if ( !effect.name.empty() ) {
			hasParticles = true;
		}
	}
}


void Model::Free()
{
	ModelResourceManager::Instance()->modelAuxPool.Delete( aux );
	aux = 0;
}


void Model::Serialize( XStream* xs, SpaceTree* tree )
{
	XarcOpen( xs, "Model" );

	static const Vector4F WHITE = { 1, 1, 1, 1 };
	static const Vector4F ONE   = { 1, 1, 1, 1 };

	XARC_SER_DEF( xs, debugScale, 1.0f );
	XARC_SER_DEF( xs, animationRate, 1.0f );
	XARC_SER_DEF( xs, totalCrossFadeTime, 0 );
	XARC_SER_DEF( xs, crossFadeTime, 0 );
	XARC_SER_DEF( xs, hasParticles, false );
	XARC_SER( xs, flags );
	XARC_SER_DEF( xs, currentAnim.time, 0 );
	XARC_SER_DEF( xs, currentAnim.id, ANIM_OFF );
	XARC_SER_DEF( xs, prevAnim.time, 0 );
	XARC_SER_DEF( xs, prevAnim.id, ANIM_OFF );
	XARC_SER( xs, pos );
	XARC_SER( xs, rot );
	XARC_SER_DEF( xs, color, WHITE );
	XARC_SER( xs, boneFilter );
	XARC_SER_DEF( xs, control, ONE );

	if ( xs->Saving() ) {
		StreamWriter* save = xs->Saving();
		XarcSet( xs, "resource", resource->header.name );
		if ( animationResource ) {
			save->Set( "animationResource", animationResource->ResourceName() );
		}
		if ( aux ) {
			save->OpenElement( "aux" );
			XARC_SER_KEY( xs, "texture0XForm", aux->texture0XForm );
			XARC_SER_KEY( xs, "texture0Clip", aux->texture0Clip );
			XARC_SER_KEY( xs, "texture0ColorMap", aux->texture0ColorMap );
			aux->boneData.Serialize( xs );
			save->CloseElement();
		}
	}
	else {
		StreamReader* load = xs->Loading();
		IString name;
		XarcGet( xs, "resource", name );
		resource = ModelResourceManager::Instance()->GetModelResource( name.c_str() );
		
		const StreamReader::Attribute* attr = load->Get( "animationResource" );
		if ( attr ) {
			const char* name = load->Value( attr, 0 );
			animationResource = AnimationResourceManager::Instance()->GetResource( name );
			GLASSERT( animationResource );
		}
		if ( load->HasChild() ) {
			load->OpenElement();	// aux
			aux = ModelResourceManager::Instance()->modelAuxPool.New();
			XARC_SER_KEY( xs, "texture0XForm", aux->texture0XForm );
			XARC_SER_KEY( xs, "texture0Clip", aux->texture0Clip );
			XARC_SER_KEY( xs, "texture0ColorMap", aux->texture0ColorMap );
			aux->boneData.Serialize( xs );
			load->CloseElement();
		}
	}

	if ( tree ) {
		tree->Update( this );
	}
	XarcClose( xs );
}


void Model::SetPosAndYRotation( const grinliz::Vector3F& pos, float r )
{
	static const Vector3F UP = { 0, 1, 0 };
	Quaternion q;
	q.FromAxisAngle( UP, r );

	if ( pos != this->pos || q != rot ) {
		this->pos = pos;
		this->rot = q;
		Modify();
		if ( tree ) {
			tree->Update( this );
		}
	}
}


void Model::SetTransform( const grinliz::Matrix4& tr )
{
	Quaternion q;
	Vector3F v;
	Quaternion::Decompose( tr, &v, &q );

	if ( v != pos || q != rot ) {
		pos = v;
		rot = q;
		Modify();
		if ( tree ) {
			tree->Update( this );
		}
	}
}


void Model::SetPos( const grinliz::Vector3F& pos )
{ 
	if ( pos != this->pos ) {
		Modify();
		this->pos = pos;	
		if ( tree ) {
			tree->Update( this ); 
		}
	}
}


void Model::SetScale( float s )
{
	if ( debugScale != s ) {
		debugScale = s;
		Modify();
	}
}


void Model::SetRotation( const Quaternion& q )
{
	if ( q != rot ) {
		Modify();
		this->rot = q;		
		if ( tree )
			tree->Update( this );	// call because bound computation changes with rotation
	}
}


void Model::SetTextureXForm( float a, float d, float tx, float ty )
{
	if ( a != 1 || d != 1 || tx != 0 || ty != 0 ) {
		SetFlag( MODEL_TEXTURE0_XFORM );
		if ( !aux ) {
			aux = ModelResourceManager::Instance()->modelAuxPool.New();
		}
		aux->texture0XForm.Set( a, d, tx, ty );
	}
	else {
		ClearFlag( MODEL_TEXTURE0_XFORM );
	}
}


void Model::SetTextureClip( float x0, float y0, float x1, float y1 )
{
	if ( x0 != 0 || y0 != 0 || x1 != 1 || y1 != 1 ) {
		SetFlag( MODEL_TEXTURE0_CLIP );
		if ( !aux ) {
			aux = ModelResourceManager::Instance()->modelAuxPool.New();
		}
		aux->texture0Clip.Set( x0, y0, x1, y1 );
	}
	else {
		ClearFlag( MODEL_TEXTURE0_CLIP );
	}
}


void Model::SetColorMap( bool enable, const grinliz::Matrix4& mat )
{
	if ( enable ) {
		SetFlag( MODEL_TEXTURE0_COLORMAP );
		if ( !aux ) {
			aux = ModelResourceManager::Instance()->modelAuxPool.New();
		}
		aux->texture0ColorMap = mat;
	}
	else {
		ClearFlag( MODEL_TEXTURE0_COLORMAP );
	}
}


void Model::SetColorMap( bool enable, const Vector4F& red, const Vector4F& green, const Vector4F& blue, float alpha )
{
	if ( enable ) {
		SetFlag( MODEL_TEXTURE0_COLORMAP );
		if ( !aux ) {
			aux = ModelResourceManager::Instance()->modelAuxPool.New();
		}
		aux->texture0ColorMap.SetCol( 0, red );
		aux->texture0ColorMap.SetCol( 1, green );
		aux->texture0ColorMap.SetCol( 2, blue );
		aux->texture0ColorMap.m44 = alpha;
	}
	else {
		ClearFlag( MODEL_TEXTURE0_COLORMAP );
	}
}


bool Model::AnimationDone() const
{
	if ( currentAnim.id >= 0 && currentAnim.time >= animationResource->Duration( currentAnim.id ) )
		return true;
	return false;
}


void Model::SetAnimation( int id, U32 crossFade, bool restart )
{
	if ( !animationResource )
		return;
	
	GLASSERT( id >= ANIM_OFF && id < ANIM_COUNT );

	if ( id >= 0 ) {
		if ( restart || currentAnim.id != id ) {
			totalCrossFadeTime = crossFade;
			crossFadeTime = 0;

			// Only push the previous if it's a good animation
			// for blending.
			if ( currentAnim.time > 0 || prevAnim.id == ANIM_OFF ) {
				prevAnim = currentAnim;
				currentAnim.id = id;

				if (    AnimationResource::Synchronized( id )
					 && AnimationResource::Synchronized( prevAnim.id ) ) 
				{
					currentAnim.time = prevAnim.time;
				}
				else {
					currentAnim.time = 0;
				}
			}

			U32 duration = animationResource->Duration( currentAnim.id );
			totalCrossFadeTime = Min( totalCrossFadeTime, duration / 2 );

			GLASSERT( animationResource );
			GLASSERT( animationResource->HasAnimation( currentAnim.id ));
		}
	}
	else {
		currentAnim.id = ANIM_OFF;
		currentAnim.time = 0;
	}
}


void Model::DeltaAnimation( U32 _time, int *metaData, bool *done )
{
	if ( !HasAnimation() )
		return;

	if ( animationRate != 1.0f ) {
		_time = (U32)((float)_time*animationRate);
	}

	if ( metaData ) {
		*metaData = animationResource->GetMetaData( currentAnim.id, currentAnim.time, currentAnim.time+_time );
	}

	if ( done && !AnimationResource::Looping( currentAnim.id) ) {
		*done = false;
		U32 duration = animationResource->Duration( currentAnim.id );
		if ( currentAnim.time >= duration ) {
			*done = true;
		}
	}

	currentAnim.time	+= _time;
	prevAnim.time		+= _time;
	crossFadeTime		+= _time;
}


void Model::CalcHitAABB( Rectangle3F* aabb ) const
{
	// This is already an approximation - ignore rotation.
	aabb->min = pos + resource->hitBounds.min;
	aabb->max = pos + resource->hitBounds.max;
}


void Model::CrossFade( float fraction, BoneData::Bone* inOut, const BoneData::Bone& prev ) const
{
	Quaternion angle1 = inOut->rot;
	Quaternion angle2 = prev.rot;
	Quaternion angle;
	Quaternion::SLERP( angle1, angle2, fraction, &angle );

	inOut->rot = angle;
	for( int i=0; i<3; ++i ) {
		inOut->pos.X(i) = Lerp( prev.pos.X(i), inOut->pos.X(i), fraction );
	}
}


void Model::CalcAnimation( BoneData* boneData ) const
{
	GLASSERT( HasAnimation() );
	animationResource->GetTransform( currentAnim.id, resource->header, currentAnim.time, boneData );

	if ( (crossFadeTime < totalCrossFadeTime) && (prevAnim.id >= 0) ) {
		BoneData prevBoneData;
		animationResource->GetTransform( prevAnim.id, resource->header, prevAnim.time, &prevBoneData );
		float fraction = (float)crossFadeTime / (float)totalCrossFadeTime;

		for( int i=0; i<EL_MAX_BONES; ++i ) {
			CrossFade( fraction, &boneData->bone[i], prevBoneData.bone[i] );
		}
	}
}



void Model::CalcAnimation( BoneData::Bone* bone, IString boneName ) const
{
	GLASSERT( HasAnimation() );
	//IString boneName = StringPool::Intern( _boneName );
	animationResource->GetTransform( currentAnim.id, boneName, resource->header, currentAnim.time, bone );

	if ( crossFadeTime < totalCrossFadeTime && (prevAnim.id >= 0) ) {
		BoneData::Bone bone2;
		animationResource->GetTransform( prevAnim.id, boneName, resource->header, prevAnim.time, &bone2 );
		float fraction = (float)crossFadeTime / (float)totalCrossFadeTime;
		CrossFade( fraction, bone, bone2 );
	}
}


void Model::CalcMetaData( grinliz::IString name, grinliz::Matrix4* meta ) const
{
	const Matrix4& xform = XForm();	// xform of this model
	const ModelMetaData* data = resource->GetMetaData( name );
	GLASSERT( data );

	if ( !HasAnimation() || data->boneName.empty() ) {
		Matrix4 local;
		local.SetTranslation( data->pos );

		*meta = xform * local;
	}
	else {
		BoneData::Bone bone;
		CalcAnimation( &bone, data->boneName );

		Matrix4 local;
		bone.ToMatrix( &local );

		Matrix4 t;
		t.SetTranslation( data->pos );

		Matrix4 r;
		if ( data->axis.Length() ) {
			GLASSERT( Equal( data->axis.Length(), 1.0f, 0.0001f ));
			r.SetAxisAngle( data->axis, data->rotation );
		}

		*meta = xform * local * t * r;
	}
}


grinliz::IString Model::GetBoneName( int i ) const
{
	GLASSERT( i >= 0 && i < EL_MAX_BONES );
	return resource->header.boneName[i];
}


int Model::GetBoneID( grinliz::IString name ) const
{
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		if ( resource->header.boneName[i] == name )
			return i;
	}
	return -1;
}


void Model::SetBoneFilter( const grinliz::IString* names, const bool* filter )
{
	int id[4] = { -1, -1, -1, -1 };
	for( int i=0; i<4; ++i ) {
		if ( filter[i] ) {
			id[i] = GetBoneID( names[i] );
		}
	}
	SetBoneFilter( id );
}



void Model::EmitParticles( ParticleSystem* system, const Vector3F* eyeDir, U32 deltaTime ) const
{
	if ( flags & MODEL_INVISIBLE )
		return;

	for( int i=0; i<EL_MAX_MODEL_EFFECTS; ++i ) {
		const ModelParticleEffect& effect = resource->header.effectData[i];
		if ( !effect.name.empty() ) {
			Matrix4 xform;
			CalcMetaData( effect.metaData, &xform );
			Vector3F pos = xform.Col(3);
			static const Vector3F UP = { 0, 1, 0 };
			system->EmitPD( effect.name.c_str(), pos, UP, deltaTime );
		}
	}
}


void Model::CalcTargetSize( float* width, float* height ) const
{
	*width  = ( resource->AABB().SizeX() +resource->AABB().SizeZ() )*0.5f;
	*height = resource->AABB().SizeY();
}


void Model::Queue( RenderQueue* queue, EngineShaders* engineShaders, int required, int excluded )
{
	if ( flags & MODEL_INVISIBLE )
		return;

	for( U32 i=0; i<resource->header.nAtoms; ++i ) 
	{
		Texture* t = resource->atom[i].texture;

		int base = EngineShaders::LIGHT;
		if ( t->Alpha() ) {
			if ( t->Emissive() )
				base = EngineShaders::EMISSIVE;
			else
				base = EngineShaders::BLEND;
		}

		if ( (( base & required ) == required ) && (( base & excluded ) == 0 ) ) {
			int mod = 0;

			if ( HasColor() ) {
				mod |= ShaderManager::COLOR_PARAM;
			}
			if ( HasBoneFilter() ) {
				mod |= ShaderManager::BONE_FILTER;
			}
			if ( HasAnimation() ) {
				mod |= ShaderManager::BONE_XFORM;
			}

			if ( flags & MODEL_TEXTURE0_XFORM )		
				mod |= ShaderManager::TEXTURE0_XFORM;
			if ( flags & MODEL_TEXTURE0_CLIP )		
				mod |= ShaderManager::TEXTURE0_CLIP;
			// test both the existance of a color map, and that
			// this particular texture supports it.
			if ( (flags & MODEL_TEXTURE0_COLORMAP) && t->ColorMap() )	
				mod |= ShaderManager::TEXTURE0_COLORMAP;

			if ( control.y != 1 ) {
				mod |= ShaderManager::SATURATION;
			}

			GPUState state;
			engineShaders->GetState( base, mod, &state );

			const ModelAux* modelAux = 0;

			if ( HasAnimation() ) {
				if ( !aux ) {
					aux = ModelResourceManager::Instance()->modelAuxPool.New();
				}
				CalcAnimation( &aux->boneData ); 
				modelAux = aux;
			}
			if (     (flags & MODEL_TEXTURE0_XFORM)
				  || (flags & MODEL_TEXTURE0_CLIP)
				  || (flags & MODEL_TEXTURE0_COLORMAP)
				  || (flags & MODEL_TEXTURE0_XFORM) )
			{
				GLASSERT( aux );
				modelAux = aux;
			}
			queue->Add( this,									// reference back
						&resource->atom[i],						// model atom to render
						state,
						color,
						boneFilter,
						control,								// parameter to the shader #2
						modelAux );
		}
	}
}


void ModelAtom::Bind( GPUStream* stream, GPUStreamData* data ) const
{
	GPUStream vertexStream( *vertex );
	*stream = vertexStream;

#ifdef EL_USE_VBO
	if ( GPUState::SupportsVBOs() && !vertexBuffer.IsValid() ) {
		GLASSERT( !indexBuffer.IsValid() );

		vertexBuffer = GPUVertexBuffer::Create( vertex, sizeof(*vertex), nVertex*instances );
		indexBuffer  = GPUIndexBuffer::Create(  index,  nIndex*instances );
	}

	if ( vertexBuffer.IsValid() && indexBuffer.IsValid() ) {
		data->indexBuffer = indexBuffer.ID();
		data->vertexBuffer = vertexBuffer.ID();
	}
	else
#endif
	{
		data->indexPtr = index;
		data->streamPtr = vertex;
	}
	data->texture0 = this->texture;
}


const grinliz::Rectangle3F& Model::AABB() const
{
	XForm();	// just makes sure the cache is good.
	return aabb;
}


const grinliz::Matrix4& Model::XForm() const
{
	if ( !xformValid ) {
		Matrix4 t;
		t.SetTranslation( pos );

		Matrix4 s;
		if ( debugScale != 1.0 ) {
			s.SetScale( debugScale );
		}
		Matrix4 r;
		rot.ToMatrix( &r );

		_xform = t*r*s;

		// compute the AABB.
		MultMatrix4( _xform, resource->header.bounds, &aabb );
		xformValid = true;
	}
	return _xform;
}


const grinliz::Matrix4& Model::InvXForm() const
{
	if ( !invValid ) {
		const Matrix4& xform = XForm();

		const Vector3F& u = *((const Vector3F*)&xform.x[0]);
		const Vector3F& v = *((const Vector3F*)&xform.x[4]);
		const Vector3F& w = *((const Vector3F*)&xform.x[8]);

		_invXForm.m11 = u.x;	_invXForm.m12 = u.y;	_invXForm.m13 = u.z;	_invXForm.m14 = -DotProduct( u, pos );
		_invXForm.m21 = v.x;	_invXForm.m22 = v.y;	_invXForm.m23 = v.z;	_invXForm.m24 = -DotProduct( v, pos );
		_invXForm.m31 = w.x;	_invXForm.m32 = w.y;	_invXForm.m33 = w.z;	_invXForm.m34 = -DotProduct( w, pos );
		_invXForm.m41 = 0;		_invXForm.m42 = 0;		_invXForm.m43 = 0;		_invXForm.m44 = 1;

		invValid = true;
	}
	return _invXForm;
}


// TestHitTest(PC)
// 1.4 fps (eek!)
// sphere test: 4.4
// tweaking: 4.5
// AABB testing in model: 8.0
// cache the xform: 8.4
// goal: 30. Better but ouchie.

int Model::IntersectRay(	const Vector3F& _origin, 
							const Vector3F& _dir,
							Vector3F* intersect ) const
{
	Vector4F origin = { _origin.x, _origin.y, _origin.z, 1.0f };
	Vector4F dir    = { _dir.x, _dir.y, _dir.z, 0.0f };
	int result = grinliz::REJECT;

	Vector3F dv;
	float dt;
	int initTest = IntersectRayAABB( _origin, _dir, AABB(), &dv, &dt );

	if ( initTest == grinliz::INTERSECT || initTest == grinliz::INSIDE )
	{
		const Matrix4& inv = InvXForm();

		Vector4F objOrigin4 = inv * origin;
		Vector4F objDir4    = inv * dir;

		Vector3F objOrigin = { objOrigin4.x, objOrigin4.y, objOrigin4.z };
		Vector3F objDir    = { objDir4.x, objDir4.y, objDir4.z };
		Vector3F objIntersect;

		result = resource->Intersect( objOrigin, objDir, &objIntersect );
		if ( result == grinliz::INTERSECT ) {
			// Back to this coordinate system. What a pain.
			const Matrix4& xform = XForm();

			Vector4F objIntersect4 = { objIntersect.x, objIntersect.y, objIntersect.z, 1.0f };
			Vector4F intersect4 = xform*objIntersect4;
			intersect->Set( intersect4.x, intersect4.y, intersect4.z );
		}
	}
	return result;
}


void ModelResourceManager::AddModelResource( ModelResource* res )
{
	modelResArr.Push( res );
	map.Add( res->header.name.c_str(), res );
}


const ModelResource* ModelResourceManager::GetModelResource( const char* name, bool errorIfNotFound )
{
	GLASSERT( name );

	ModelResource* res = 0;
	bool found = map.Query( name, &res );

	if ( found ) 
		return res;
	if ( errorIfNotFound ) {
		GLASSERT( 0 );
	}
	return 0;
}


/*static*/ void ModelResourceManager::Create()
{
	GLASSERT( instance == 0 );
	instance = new ModelResourceManager();
}

/*static*/ void ModelResourceManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}

ModelResourceManager* ModelResourceManager::instance = 0;

ModelResourceManager::ModelResourceManager()
{
}
	

ModelResourceManager::~ModelResourceManager()
{
	for( int i=0; i<modelResArr.Size(); ++i )
		delete modelResArr[i];
}


void ModelResourceManager::DeviceLoss()
{
	for( int i=0; i<modelResArr.Size(); ++i )
		modelResArr[i]->DeviceLoss();

}