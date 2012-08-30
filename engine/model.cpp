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

#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glperformance.h"

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


const ModelMetaData* ModelResource::GetMetaData( const char* name ) const
{
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( StrEqual( name, header.metaData[i].name.c_str() )) {
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

	float greater = grinliz::Max( res->header.bounds.SizeX(), res->header.bounds.SizeZ() );
	greater *= 1.41f;	// turns 45deg - FIXME: check this on paper
	greater *= 0.5f;
	res->invariantBounds.min.Set( -greater, res->header.bounds.min.y, -greater );
	res->invariantBounds.max.Set( greater, res->header.bounds.max.y, greater );

	for( int i=0; i<EL_MAX_BONES; ++i ) {
		if ( !res->header.boneName[i].name.empty() ) {
			GLOUTPUT(( "Model %s bone %s id=%d\n", res->header.name.c_str(), res->header.boneName[i].name.c_str(), res->header.boneName[i].id ));
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
	animationTime = 0;
	animationRate = 1.0f;
	crossFadeTime = 0;
	totalCrossFadeTime = 0;
	animationID = ANIM_OFF;

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


void Model::SetAnimation( AnimationType id, U32 crossFade )
{
	GLASSERT( id >= ANIM_OFF && id < ANIM_COUNT );

	if ( id >= 0 ) {
		if ( animationID != id ) {
			totalCrossFadeTime = crossFade;
			crossFadeTime = 0;
			prevAnimationID = animationID;

			animationID = id;
			GLASSERT( animationResource );
			GLASSERT( animationResource->HasAnimation( id ));

			U32 duration = animationResource->Duration( id );
			totalCrossFadeTime = Min( totalCrossFadeTime, duration / 2 );
		}
	}
	else {
		animationID = ANIM_OFF;
	}
}


void Model::DeltaAnimation( U32 time, grinliz::CArray<AnimationMetaData, EL_MAX_METADATA> *metaData, bool *looped )
{
	if ( !HasAnimation() )
		return;

	if ( animationRate != 1.0f ) {
		time = (U32)((float)time*animationRate);
	}

	if ( metaData ) {
		animationResource->GetMetaData( animationID, animationTime, animationTime+time, metaData );
	}

	if ( looped ) {
		*looped = false;
		U32 duration = animationResource->Duration( animationID );
		U32 frame0 = animationTime % duration;
		U32 frame1 = (animationTime+time) % duration;
		if ( frame1 < frame0 ) 
			*looped = true;
	}

	animationTime += time;
	crossFadeTime += time;
}


void Model::CalcHitAABB( Rectangle3F* aabb ) const
{
	// This is already an approximation - ignore rotation.
	aabb->min = pos + resource->hitBounds.min;
	aabb->max = pos + resource->hitBounds.max;
}


void Model::CalcAnimation( BoneData* boneData ) const
{
	GLASSERT( HasAnimation() );
	animationResource->GetTransform( animationID, resource->header, animationTime, boneData );

	if ( (crossFadeTime < totalCrossFadeTime) && (prevAnimationID >= 0) ) {
		BoneData boneData2;
		animationResource->GetTransform( prevAnimationID, resource->header, animationTime, &boneData2 );
		float fraction = (float)crossFadeTime / (float)totalCrossFadeTime;

		for( int i=0; i<EL_MAX_BONES; ++i ) {
			float angle1 = boneData->bone[i].angleRadians;
			float angle2 = boneData2.bone[i].angleRadians;
			if ( fabsf( angle1-angle2 ) > PI ) {
				if ( angle1 < angle2 ) angle2 -= TWO_PI;
				else				   angle2 += TWO_PI;
			}

			boneData->bone[i].angleRadians = Lerp( angle2, angle1, fraction ); 
			boneData->bone[i].dy = Lerp( boneData2.bone[i].dy, boneData->bone[i].dy, fraction ); 
			boneData->bone[i].dz = Lerp( boneData2.bone[i].dz, boneData->bone[i].dz, fraction ); 
		}
	}
}


void Model::CalcAnimation( BoneData::Bone* bone, const char* boneName ) const
{
	GLASSERT( HasAnimation() );
	animationResource->GetTransform( animationID, boneName, resource->header, animationTime, bone );

	if ( crossFadeTime < totalCrossFadeTime && (prevAnimationID >= 0) ) {
		BoneData::Bone bone2;
		animationResource->GetTransform( prevAnimationID, boneName, resource->header, animationTime, &bone2 );
		float fraction = (float)crossFadeTime / (float)totalCrossFadeTime;

		float angle1 = bone->angleRadians;
		float angle2 = bone2.angleRadians;
		if ( fabsf( angle1-angle2 ) > PI ) {
			if ( angle1 < angle2 ) angle2 -= TWO_PI;
			else				   angle2 += TWO_PI;
		}

		bone->angleRadians	= Lerp( angle2, angle1, fraction ); 
		bone->dy			= Lerp( bone2.dy, bone->dy, fraction ); 
		bone->dz			= Lerp( bone2.dz, bone->dz, fraction ); 
	}
}


void Model::CalcMetaData( const char* name, grinliz::Matrix4* meta ) const
{
	const Matrix4& xform = XForm();	// xform of this model
	const ModelMetaData* data = resource->GetMetaData( name );
	GLASSERT( data );

	if ( !HasAnimation() ) {
		Matrix4 local;
		local.SetTranslation( data->pos );

		*meta = xform * local;
	}
	else {
		BoneData::Bone bone;
		CalcAnimation( &bone, data->boneName.c_str() );

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


void Model::EmitParticles( ParticleSystem* system, const Vector3F* eyeDir, U32 deltaTime ) const
{
	if ( flags & MODEL_INVISIBLE )
		return;

	for( int i=0; i<EL_MAX_MODEL_EFFECTS; ++i ) {
		const ModelParticleEffect& effect = resource->header.effectData[i];
		if ( !effect.name.empty() ) {
			Matrix4 xform;
			CalcMetaData( effect.metaData.c_str(), &xform );
			Vector3F pos = xform.Col(3);
			static const Vector3F UP = { 0, 1, 0 };
			system->EmitPD( effect.name.c_str(), pos, UP, eyeDir, deltaTime );
		}
	}
}


void Model::CalcTargetSize( float* width, float* height ) const
{
	*width  = ( resource->AABB().SizeX() +resource->AABB().SizeZ() )*0.5f;
	*height = resource->AABB().SizeY();
}


void Model::Queue( RenderQueue* queue, EngineShaders* engineShaders )
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

		int mod = 0;
		if ( HasTextureXForm(i) ) {
			mod = ShaderManager::TEXTURE0_TRANSFORM;
		}
		else if ( HasColor() ) {
			mod = ShaderManager::COLOR_PARAM;
		}
		else if ( HasBoneFilter() ) {
			mod = ShaderManager::BONE_FILTER;
		}

		if ( HasAnimation() ) {
			mod |= ShaderManager::BONE_XFORM;
		}

		GPUShader* shader = engineShaders->GetShader( base, mod );

		BoneData* pBD = 0;
		BoneData boneData;
		if ( HasAnimation() ) {
			CalcAnimation( &boneData ); 
			pBD = &boneData;
		}

		queue->Add( this,									// reference back
					&resource->atom[i],						// model atom to render
					shader,
					param[i],								// parameter to the shader
					pBD );								
	}
}


void ModelAtom::Bind( GPUShader* shader ) const
{
	GPUStream stream( vertex );

#ifdef EL_USE_VBO
	if ( GPUShader::SupportsVBOs() && !vertexBuffer.IsValid() ) {
		GLASSERT( !indexBuffer.IsValid() );

		vertexBuffer = GPUVertexBuffer::Create( vertex, sizeof(*vertex), nVertex*instances );
		indexBuffer  = GPUIndexBuffer::Create(  index,  nIndex*instances );
	}

	if ( vertexBuffer.IsValid() && indexBuffer.IsValid() ) {
		shader->SetStream( stream, vertexBuffer, nIndex, indexBuffer );
	}
	else
#endif
	{
		shader->SetStream( stream, vertex, nIndex, index );
	}
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