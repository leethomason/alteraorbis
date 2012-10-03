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



#include "gpustatemanager.h"
#include "texture.h"
#include "enginelimits.h"
#include "../gamui/gamui.h"	// for auto setting up gamui stream
#include "../grinliz/glperformance.h"
#include "shadermanager.h"
#include "../grinliz/glgeometry.h"

#include "platformgl.h"

using namespace grinliz;

int GPUShader::matrixDepth[3];

/*static*/ GPUVertexBuffer GPUVertexBuffer::Create( const void* vertex, int size, int nVertex )
{
	GPUVertexBuffer buffer;

	if ( GPUShader::SupportsVBOs() ) {
		U32 dataSize  = size*nVertex;
		glGenBuffersX( 1, (GLuint*) &buffer.id );
		glBindBufferX( GL_ARRAY_BUFFER, buffer.id );
		// if vertex is null this will just allocate
		glBufferDataX( GL_ARRAY_BUFFER, dataSize, vertex, vertex ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );
		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		CHECK_GL_ERROR;
	}
	return buffer;
}

/*
void GPUVertexBuffer::Upload( const Vertex* data, int count, int start )
{
	GLASSERT( GPUShader::SupportsVBOs() );
	glBindBufferX( GL_ARRAY_BUFFER, id );
	// target, offset, size, data
	glBufferSubDataX( GL_ARRAY_BUFFER, start*sizeof(Vertex), count*sizeof(Vertex), data );
	CHECK_GL_ERROR;
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}
*/

void GPUVertexBuffer::Destroy() 
{
	if ( id ) {
		glDeleteBuffersX( 1, (GLuint*) &id );
		id = 0;
	}
}


/*static*/ GPUIndexBuffer GPUIndexBuffer::Create( const U16* index, int nIndex )
{
	GPUIndexBuffer buffer;

	if ( GPUShader::SupportsVBOs() ) {
		U32 dataSize  = sizeof(U16)*nIndex;
		glGenBuffersX( 1, (GLuint*) &buffer.id );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, buffer.id );
		glBufferDataX( GL_ELEMENT_ARRAY_BUFFER, dataSize, index, GL_STATIC_DRAW );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
		CHECK_GL_ERROR;
	}
	return buffer;
}

/*
void GPUIndexBuffer::Upload( const uint16_t* data, int count, int start )
{
	GLASSERT( GPUShader::SupportsVBOs() );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, id );
	// target, offset, size, data
	glBufferSubDataX( GL_ELEMENT_ARRAY_BUFFER, start*sizeof(uint16_t), count*sizeof(uint16_t), data );
	CHECK_GL_ERROR;
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}
*/



void GPUIndexBuffer::Destroy() 
{
	if ( id ) {
		glDeleteBuffersX( 1, (GLuint*) &id );
		id = 0;
	}
}


/*static*/ GPUInstanceBuffer GPUInstanceBuffer::Create( const uint8_t* data, int nData )
{
	GPUInstanceBuffer buffer;

	if ( GPUShader::SupportsVBOs() ) {
		U32 dataSize  = nData;
		glGenBuffersX( 1, (GLuint*) &buffer.id );
		glBindBufferX( GL_ARRAY_BUFFER, buffer.id );
		// if vertex is null this will just allocate
		glBufferDataX( GL_ARRAY_BUFFER, dataSize, data, data ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );
		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		CHECK_GL_ERROR;
	}
	return buffer;
}


/*
void GPUInstanceBuffer::Upload( const uint8_t* data, int count, int start )
{
	GLASSERT( GPUShader::SupportsVBOs() );
	glBindBufferX( GL_ARRAY_BUFFER, id );
	// target, offset, size, data
	glBufferSubDataX( GL_ARRAY_BUFFER, start*sizeof(uint8_t), count*sizeof(uint8_t), data );
	CHECK_GL_ERROR;
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}
*/

void GPUInstanceBuffer::Destroy() 
{
	if ( id ) {
		glDeleteBuffersX( 1, (GLuint*) &id );
		id = 0;
	}
}


MatrixStack::MatrixStack() : index( 0 )
{
	stack[0].SetIdentity();
}


MatrixStack::~MatrixStack()
{
	GLASSERT( index == 0 );
}


void MatrixStack::Push()
{
	GLASSERT( index < MAX_DEPTH-1 );
	if ( index < MAX_DEPTH-1 ) {
		stack[index+1] = stack[index];
		++index;
	}
}


void MatrixStack::Pop()
{
	GLASSERT( index > 0 );
	if ( index > 0 ) {
		index--;
	}
}


void MatrixStack::Multiply( const grinliz::Matrix4& m )
{
	GLASSERT( index > 0 );
	stack[index] = stack[index] * m;
}


/*static*/ int			GPUShader::primitive = GL_TRIANGLES; 
/*static*/ int			GPUShader::trianglesDrawn = 0;
/*static*/ int			GPUShader::drawCalls = 0;
/*static*/ uint32_t		GPUShader::uid = 0;
/*static*/ GPUShader::MatrixType GPUShader::matrixMode = MODELVIEW_MATRIX;
/*static*/ MatrixStack	GPUShader::mvStack;
/*static*/ MatrixStack	GPUShader::projStack;
/*static*/ int			GPUShader::vboSupport = 0;
/*static*/ GPUShader::BlendMode	GPUShader::currentBlend = BLEND_NONE;
/*static*/ bool			GPUShader::currentDepthWrite = true;
/*static*/ bool			GPUShader::currentDepthTest = true;
/*static*/ bool			GPUShader::currentColorWrite = true;
/*static*/ GPUShader::StencilMode GPUShader::currentStencilMode = STENCIL_OFF;

/*static*/ bool GPUShader::SupportsVBOs()
{
#ifdef EL_USE_VBO
	if ( vboSupport == 0 ) {
		const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
		const char* vbo = strstr( extensions, "ARB_vertex_buffer_object" );
		vboSupport = (vbo) ? 1 : -1;
	}
	return (vboSupport > 0);
#else
	return false;
#endif
}


/*static */ void GPUShader::ResetState()
{
	// Texture unit 1
	glActiveTexture( GL_TEXTURE1 );
	glClientActiveTexture( GL_TEXTURE1 );
	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// Texture unit 0
	glActiveTexture( GL_TEXTURE0 );
	glClientActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// Client state
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// Blend/Alpha
	glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );
	glAlphaFunc( GL_GREATER, 0.5f );

	// Ligting
	glDisable( GL_LIGHTING );

	// Depth
	glDepthMask( GL_TRUE );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	// Stencil
	glDisable( GL_STENCIL_TEST );
	glStencilMask( GL_FALSE );
	glStencilFunc( GL_ALWAYS, 0xff, 0xff );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	// Matrix
	glMatrixMode( GL_MODELVIEW );
	matrixMode = MODELVIEW_MATRIX;

	// Color
	glColor4f( 1, 1, 1, 1 );

	// General config:
	glCullFace( GL_BACK );
	glEnable( GL_CULL_FACE );

	glDisable( GL_SCISSOR_TEST );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	currentBlend = BLEND_NONE;
	currentDepthTest = true;
	currentDepthWrite = true;
	currentColorWrite = true;
	currentStencilMode = STENCIL_OFF;

	CHECK_GL_ERROR;

#ifdef DEBUG
	// fixme: get the # of stencil buffer bits to check. fallback if 0?
#endif
}


//static 
void GPUShader::SetState( const GPUShader& ns )
{
	//GRINLIZ_PERFTRACK
	CHECK_GL_ERROR;
	GLASSERT( ns.stream.stride > 0 );

	ShaderManager* shadman = ShaderManager::Instance();

	int flags = 0;
	flags |= ( ns.HasTexture0() ) ? ShaderManager::TEXTURE0 : 0;
	flags |= (ns.HasTexture0() && (ns.texture0->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE0_ALPHA_ONLY : 0;
	GLASSERT( !ns.HasTexture0() || ns.stream.nTexture0 == 2 );

	flags |= ( ns.HasTexture1() ) ? ShaderManager::TEXTURE1 : 0;
	flags |= (ns.HasTexture1() && (ns.texture1->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE1_ALPHA_ONLY : 0;
	GLASSERT( !ns.HasTexture1() || ns.stream.nTexture0 == 2 );

	flags |= ns.stream.HasColor() ? ShaderManager::COLORS : 0;
	
	if ( ns.HasLighting( 0, 0, 0 )) {
		if ( ns.hemisphericalLighting )
			flags |= ShaderManager::LIGHTING_HEMI;
		else
			flags |= ShaderManager::LIGHTING_DIFFUSE;
	}

	flags |= ns.shaderFlags;	// Includes: TEXTUREx_TRANSFORM, PREMULT, etc.

	shadman->ActivateShader( flags );
	shadman->ClearStream();

	const Matrix4& mv = ns.TopMatrix( GPUShader::MODELVIEW_MATRIX );

	bool paramNeeded = shadman->ParamNeeded();
	bool bonesNeeded = shadman->BonesNeeded();

	if ( flags & ShaderManager::INSTANCE ) {
		Matrix4 vp;
		MultMatrix4( ns.TopMatrix( GPUShader::PROJECTION_MATRIX ), ns.ViewMatrix(), &vp );
		shadman->SetUniform( ShaderManager::U_MVP_MAT, vp );
		shadman->SetUniformArray( ShaderManager::U_M_MAT_ARR, EL_MAX_INSTANCE, ns.instanceMatrix );
		GLASSERT( ns.stream.instanceIDOffset > 0 );
		shadman->SetStreamData( ShaderManager::A_INSTANCE_ID, 1, GL_UNSIGNED_SHORT, 
			                    ns.stream.stride, PTR( ns.streamPtr, ns.stream.instanceIDOffset ) );
		if ( paramNeeded ) {
			shadman->SetUniformArray( ShaderManager::U_PARAM_ARR, EL_MAX_INSTANCE, ns.instanceParam );
		}
	}
	else {
		Matrix4 mvp;
		MultMatrix4( ns.TopMatrix( GPUShader::PROJECTION_MATRIX ), mv, &mvp );
		shadman->SetUniform( ShaderManager::U_MVP_MAT, mvp );
		if ( paramNeeded ) {
			shadman->SetUniform( ShaderManager::U_PARAM, ns.instanceParam[0] );
		}
	}

	// Texture1
	glActiveTexture( GL_TEXTURE1 );

	if (  ns.HasTexture1() ) {
		glBindTexture( GL_TEXTURE_2D, ns.texture1->GLID() );
		shadman->SetTexture( 1, ns.texture1 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE1, ns.stream.nTexture1, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.texture1Offset ) );
	}
	CHECK_GL_ERROR;

	// Texture0
	glActiveTexture( GL_TEXTURE0 );

	if (  ns.HasTexture0() ) {
		glBindTexture( GL_TEXTURE_2D, ns.texture0->GLID() );
		shadman->SetTexture( 0, ns.texture0 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE0, ns.stream.nTexture0, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.texture0Offset ) );
	}
	CHECK_GL_ERROR;

	// vertex
	if ( ns.stream.HasPos() ) {
		shadman->SetStreamData( ShaderManager::A_POS, ns.stream.nPos, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.posOffset ) );	 
	}

	// color
	if ( ns.stream.HasColor() ) {
		GLASSERT( ns.stream.nColor == 4 );
		shadman->SetStreamData( ShaderManager::A_COLOR, 4, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.colorOffset ) );	 
	}

	// bones
	if ( bonesNeeded ) {
		GLASSERT( ns.stream.boneOffset );	// could be zero...but that would be odd.
		int count = EL_MAX_BONES;
		if ( flags & ShaderManager::INSTANCE ) {
			count *= EL_MAX_INSTANCE;
		}

		shadman->SetUniformArray( ShaderManager::U_BONEXFORM, count, (const Vector3F*) ns.instanceBones );
		shadman->SetStreamData( ShaderManager::A_BONE_ID, 1, GL_UNSIGNED_SHORT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.boneOffset ) );
	}

	// lighting 
	if ( flags & (ShaderManager::LIGHTING_DIFFUSE | ShaderManager::LIGHTING_HEMI) ) {
		Vector4F dirWC, d, a;
		ns.HasLighting( &dirWC, &a, &d );

		Vector4F dirEye = GPUShader::ViewMatrix() * dirWC;
		GLASSERT( Equal( dirEye.Length(), 1.f, 0.01f ));
		Vector3F dirEye3 = { dirEye.x, dirEye.y, dirEye.z };

		// NOTE: the normal matrix can be used because the game doesn't support scaling.
		shadman->SetUniform( ShaderManager::U_NORMAL_MAT, mv );
		shadman->SetUniform( ShaderManager::U_LIGHT_DIR, dirEye3 );
		shadman->SetUniform( ShaderManager::U_AMBIENT, a );
		shadman->SetUniform( ShaderManager::U_DIFFUSE, d );
		shadman->SetStreamData( ShaderManager::A_NORMAL, 3, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.normalOffset ) );	 
	}

	// color multiplier is always set
	shadman->SetUniform( ShaderManager::U_COLOR_MULT, ns.color );

	// Blend
	if ( ns.blend != currentBlend ) {
		currentBlend = ns.blend;
		switch( ns.blend ) {
		case BLEND_NONE:
			glDisable( GL_BLEND );
				break;
		case BLEND_NORMAL:
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			break;
		case BLEND_ADD:
			glEnable( GL_BLEND );
			glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
			break;
		}
	}

	// Depth Write
	if ( ns.depthWrite && !currentDepthWrite ) {
		glDepthMask( GL_TRUE );
		currentDepthWrite = true;
	}
	else if ( !ns.depthWrite && currentDepthWrite ) {
		glDepthMask( GL_FALSE );
		currentDepthWrite = false;
	}

	// Depth test
	if ( ns.depthTest && !currentDepthTest ) {
		glEnable( GL_DEPTH_TEST );
		currentDepthTest = true;
	}
	else if ( !ns.depthTest && currentDepthTest ) {
		glDisable( GL_DEPTH_TEST );
		currentDepthTest = false;
	}

	// Color test
	if ( ns.colorWrite && !currentColorWrite ) {
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		currentColorWrite = true;
	}
	else if ( !ns.colorWrite && currentColorWrite ) {
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		currentColorWrite = false;
	}

	if ( ns.stencilMode != currentStencilMode ) {
		currentStencilMode = ns.stencilMode;
		switch( ns.stencilMode ) {
		case STENCIL_OFF:
			glDisable( GL_STENCIL_TEST );
			glStencilMask( GL_FALSE );
			glStencilFunc( GL_NEVER, 0, 0 );
			glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
			break;
		case STENCIL_WRITE:
			glEnable( GL_STENCIL_TEST );	// this does need to be on.
			glStencilMask( GL_TRUE );
			glStencilFunc( GL_ALWAYS, 1, 1 );
			glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
			break;
		case STENCIL_SET:
			glEnable( GL_STENCIL_TEST );
			glStencilMask( GL_FALSE );
			glStencilFunc( GL_EQUAL, 1, 1 );
			glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
			break;
		case STENCIL_CLEAR:
			glEnable( GL_STENCIL_TEST );
			glStencilMask( GL_FALSE );
			glStencilFunc( GL_NOTEQUAL, 1, 1 );
			glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
			break;
		default:
			GLASSERT( 0 );
			break;
		}
	}
	CHECK_GL_ERROR;
}


void GPUShader::Clear()
{
	GLASSERT( currentStencilMode == STENCIL_OFF );	// could be fixed, just implemented for 'off'
	glStencilMask( GL_TRUE );	// Stupid stupid opengl behavior.
	glClearStencil( 0 );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glStencilMask( GL_FALSE );
}

#if 0
/*static*/void GPUShader::SetMatrixMode( MatrixType m )
{
	matrixMode = m;
}
#else

/*static*/ void GPUShader::SetViewport( int w, int h )
{
	glViewport( 0, 0, w, h );
}


/*static*/ void GPUShader::SetOrthoTransform( int screenWidth, int screenHeight )
{
	Matrix4 ortho;
	ortho.SetOrtho( 0, (float)screenWidth, (float)screenHeight, 0, -100.f, 100.f );
	SetMatrix( PROJECTION_MATRIX, ortho );

	Matrix4 identity;
	SetMatrix( MODELVIEW_MATRIX, identity );
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetPerspectiveTransform( const grinliz::Matrix4& perspective )
{
	SetMatrix( PROJECTION_MATRIX, perspective );
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetCameraTransform( const grinliz::Matrix4& camera )
{
	glMatrixMode(GL_MODELVIEW);
	// In normalized coordinates.
	GLASSERT( mvStack.NumMatrix() == 1 );
	mvStack.Set( camera );
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetScissor( int x, int y, int w, int h )
{
	if ( w > 0 && h > 0 ) {
		glEnable( GL_SCISSOR_TEST );
		glScissor( x, y, w, h );
	}
	else {
		glDisable( GL_SCISSOR_TEST );
	}
}
#endif

GPUShader::~GPUShader()
{
	GLASSERT( matrixDepth[0] >= 0 );
	GLASSERT( matrixDepth[1] >= 0 );
	GLASSERT( matrixDepth[2] >= 0 );
}


void GPUShader::Draw( int instances )
{
	//GRINLIZ_PERFTRACK

	if ( nIndex == 0 )
		return;

	bool useInstancing = instances > 0;
	if ( instances == 0 ) instances = 1;
	if ( useInstancing ) {
		SetShaderFlag( ShaderManager::INSTANCE );
	}
	else {
		ClearShaderFlag( ShaderManager::INSTANCE );
	}
	GLASSERT( (primitive != GL_TRIANGLES ) || (nIndex % 3 == 0 ));

	trianglesDrawn += instances * nIndex / 3;
	++drawCalls;

	if ( indexPtr ) {	
		if ( vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, vertexBuffer );
		}
		SetState( *this );

		GLASSERT( !indexBuffer );
		glDrawElements( primitive,	//GL_TRIANGLES except where debugging 
						nIndex*instances, GL_UNSIGNED_SHORT, indexPtr );

		if ( vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, 0 );
		}
	}
	else {
		GLASSERT( vertexBuffer );
		GLASSERT( indexBuffer );

		// This took a long time to figure. OpenGL is a state machine, except, apparently, when it isn't.
		// The VBOs impact how the SetxxxPointers work. If they aren't set, then the wrong thing gets
		// bound. What a PITA. And ugly design problem with OpenGL.
		//
		glBindBufferX( GL_ARRAY_BUFFER, vertexBuffer );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
		SetState( *this );

		glDrawElements( primitive,	// GL_TRIANGLES except when debugging 
						nIndex*instances, GL_UNSIGNED_SHORT, 0 );

		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}
	CHECK_GL_ERROR;
}


void GPUShader::DrawQuad( const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positive )
{
	PTVertex pos[4] = { 
		{ { p0.x, p0.y, p0.z }, { 0, 0 } },
		{ { p1.x, p0.y, p0.z }, { 1, 0 } },
		{ { p1.x, p1.y, p1.z }, { 1, 1 } },
		{ { p0.x, p1.y, p1.z }, { 0, 1 } },
	};
	static const U16 indexPos[6] = { 0, 1, 2, 0, 2, 3 };
	static const U16 indexNeg[6] = { 0, 2, 1, 0, 3, 2 };
	const U16* index = positive ? indexPos : indexNeg;

	DebugDraw( pos, 6, index );
}


void GPUShader::DrawLine( const grinliz::Vector3F p0, const grinliz::Vector3F p1 )
{
	Vector3F v[2] = { p0, p1 };
	static const U16 index[2] = { 0, 1 };

	GPUStream stream;
	stream.stride = sizeof(Vector3F);
	stream.nPos = 3;

	primitive = GL_LINES;
	SetStream( stream, v, 2, index );
	Draw();
	primitive = GL_TRIANGLES;
}


void GPUShader::DrawArrow( const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positive, float width )
{
	const Vector3F UP = { 0, 1, 0 };
	Vector3F normal; 
	CrossProduct( UP, p1-p0, &normal );
	if ( normal.Length() > 0.001f ) {
		normal.Normalize();
		normal = normal * width;

		PTVertex pos[4] = { 
			{ p0-normal, { 0, 0 } },
			{ p0+normal, { 1, 0 } },
			{ p1, { 1, 1 } },
		};
		static const U16 indexPos[6] = { 0, 1, 2 };
		static const U16 indexNeg[6] = { 0, 2, 1 };
		const U16* index = positive ? indexPos : indexNeg;

		DebugDraw( pos, 3, index );
	}
}


void GPUShader::DebugDraw( const PTVertex* v, int nIndex, const U16* index )
{
	GPUStream stream;
	stream.stride = sizeof(PTVertex);
	stream.nPos = 3;
	stream.posOffset = PTVertex::POS_OFFSET;
	stream.nTexture0 = 2;
	stream.texture0Offset = PTVertex::TEXTURE_OFFSET;

	SetStream( stream, v, nIndex, index );
	Draw();
}


void GPUShader::SwitchMatrixMode( MatrixType type )
{
	matrixMode = type;
	CHECK_GL_ERROR;
}


void GPUShader::PushMatrix( MatrixType type )
{
	SwitchMatrixMode( type );

	switch( type ) {
	case MODELVIEW_MATRIX:
		mvStack.Push();
		break;
	case PROJECTION_MATRIX:
		projStack.Push();
		break;
	default:
		GLASSERT( 0 );
		return;
	}
	matrixDepth[(int)type] += 1;

#ifdef DEBUG
	GLenum error = glGetError();
	if ( error  != GL_NO_ERROR ) 
	{	
		GLOUTPUT(( "GL Error: %x matrixMode=%d depth=%d\n", error, (int)type, matrixDepth[(int)type] ));	
		GLASSERT( 0 );							
	}
#endif
}


void GPUShader::MultMatrix( MatrixType type, const grinliz::Matrix4& m )
{
	// A lot of identities seem to get through...
	if ( m.IsIdentity() )
		return;

	SwitchMatrixMode( type );

	switch( type ) {
	case MODELVIEW_MATRIX:
		mvStack.Multiply( m );
		break;
	case PROJECTION_MATRIX:
		projStack.Multiply( m );
		break;
	default:
		GLASSERT( 0 );
		return;
	}	
	CHECK_GL_ERROR;
}


void GPUShader::PopMatrix( MatrixType type )
{
	CHECK_GL_ERROR;

	SwitchMatrixMode( type );
	GLASSERT( matrixDepth[(int)type] > 0 );
	switch( type ) {
	case MODELVIEW_MATRIX:
		mvStack.Pop();
		break;
	case PROJECTION_MATRIX:
		projStack.Pop();
		break;
	default:
		GLASSERT( 0 );
		return;
	}	
	
	matrixDepth[(int)type] -= 1;

	CHECK_GL_ERROR;
}


void GPUShader::SetMatrix( MatrixType type, const Matrix4& m )
{
	CHECK_GL_ERROR;

	SwitchMatrixMode( type );
	GLASSERT( matrixDepth[(int)type] == 0 );

	if ( type == PROJECTION_MATRIX ) {
		GLASSERT( projStack.NumMatrix() == 1 );
		projStack.Set( m ); 
	}
	else if ( type == MODELVIEW_MATRIX ) {
		GLASSERT( mvStack.NumMatrix() == 1 );
		mvStack.Set( m );
	}
}


const grinliz::Matrix4& GPUShader::TopMatrix( MatrixType type )
{
	if ( type == MODELVIEW_MATRIX ) {
		return mvStack.Top();
	}
	return projStack.Top();
}


const grinliz::Matrix4& GPUShader::ViewMatrix()
{
	return mvStack.Bottom();
}


void GPUStream::Clear()
{
	memset( this, 0, sizeof(*this) );
}


GPUStream::GPUStream( const Vertex* vertex )
{
	Clear();

	stride = sizeof( *vertex );
	nPos = 3;
	posOffset = Vertex::POS_OFFSET;
	nNormal = 3;
	normalOffset = Vertex::NORMAL_OFFSET;
	nTexture0 = 2;
	texture0Offset = Vertex::TEXTURE_OFFSET;
	boneOffset = Vertex::BONE_ID_OFFSET;
}


GPUStream::GPUStream( const InstVertex* vertex )
{
	Clear();

	stride = sizeof( *vertex );
	nPos = 3;
	posOffset = InstVertex::POS_OFFSET;
	nNormal = 3;
	normalOffset = InstVertex::NORMAL_OFFSET;
	nTexture0 = 2;
	texture0Offset = InstVertex::TEXTURE_OFFSET;
	instanceIDOffset = InstVertex::INSTANCE_OFFSET;
	boneOffset = InstVertex::BONE_ID_OFFSET;
}


GPUStream::GPUStream( GamuiType )
{
	Clear();
	stride = sizeof( gamui::Gamui::Vertex );
	nPos = 2;
	posOffset = gamui::Gamui::Vertex::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = gamui::Gamui::Vertex::TEX_OFFSET;
}


GPUStream::GPUStream( const PTVertex2* vertex )
{
	Clear();
	stride = sizeof( *vertex );
	nPos = 2;
	posOffset = PTVertex2::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = PTVertex2::TEXTURE_OFFSET;

}


CompositingShader::CompositingShader( BlendMode _blend )
{
	blend = _blend;
	depthWrite = false;
	depthTest = false;
}


LightShader::LightShader( const Color4F& ambient, const grinliz::Vector4F& direction, const Color4F& diffuse, BlendMode blend )
{
	this->blend = blend;
	this->ambient = ambient;
	this->direction = direction;
	this->diffuse = diffuse;
}


LightShader::~LightShader()
{
}


ParticleShader::ParticleShader() : GPUShader() 
{
	depthWrite = false;
	depthTest = true;
	SetShaderFlag( ShaderManager::PREMULT );
	blend = BLEND_ADD;
	//blend = BLEND_NORMAL;
}

