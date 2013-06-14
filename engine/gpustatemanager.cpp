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

int GPUState::matrixDepth[3];
grinliz::Color4F	GPUState::ambient;
grinliz::Vector4F	GPUState::directionWC;
grinliz::Color4F	GPUState::diffuse;


/*static*/ GPUVertexBuffer GPUVertexBuffer::Create( const void* vertex, int size, int nVertex )
{
	GPUVertexBuffer buffer;

	if ( GPUState::SupportsVBOs() ) {
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


void GPUVertexBuffer::Upload( const Vertex* data, int count, int start )
{
	GLASSERT( GPUState::SupportsVBOs() );
	glBindBufferX( GL_ARRAY_BUFFER, id );
	// target, offset, size, data
	glBufferSubDataX( GL_ARRAY_BUFFER, start*sizeof(Vertex), count*sizeof(Vertex), data );
	CHECK_GL_ERROR;
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}


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

	if ( GPUState::SupportsVBOs() ) {
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
	GLASSERT( GPUState::SupportsVBOs() );
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

	if ( GPUState::SupportsVBOs() ) {
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
	GLASSERT( GPUState::SupportsVBOs() );
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


/*static*/ int			GPUState::primitive = GL_TRIANGLES; 
/*static*/ int			GPUState::trianglesDrawn = 0;
/*static*/ int			GPUState::quadsDrawn = 0;
/*static*/ int			GPUState::drawCalls = 0;
/*static*/ uint32_t		GPUState::uid = 0;
/*static*/ GPUState::MatrixType GPUState::matrixMode = MODELVIEW_MATRIX;
/*static*/ MatrixStack	GPUState::mvStack;
/*static*/ MatrixStack	GPUState::projStack;
/*static*/ int			GPUState::vboSupport = 0;
/*static*/ GPUState::BlendMode	GPUState::currentBlend = BLEND_NONE;
/*static*/ GPUState::DepthWrite	GPUState::currentDepthWrite = DEPTH_WRITE_TRUE;
/*static*/ GPUState::DepthTest	GPUState::currentDepthTest = DEPTH_TEST_TRUE;
/*static*/ GPUState::ColorWrite	GPUState::currentColorWrite = COLOR_WRITE_TRUE;
/*static*/ GPUState::StencilMode GPUState::currentStencilMode = STENCIL_OFF;

/*static*/ bool GPUState::SupportsVBOs()
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


/*static */ void GPUState::ResetState()
{
	// Texture unit 0
	glActiveTexture( GL_TEXTURE0 );

	// Blend/Alpha
	glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

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
	matrixMode = MODELVIEW_MATRIX;

	// General config:
	glCullFace( GL_BACK );
	glEnable( GL_CULL_FACE );

	glDisable( GL_SCISSOR_TEST );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	currentBlend = BLEND_NONE;
	currentDepthTest = DEPTH_TEST_TRUE;
	currentDepthWrite = DEPTH_WRITE_TRUE;
	currentColorWrite = COLOR_WRITE_TRUE;
	currentStencilMode = STENCIL_OFF;

	CHECK_GL_ERROR;
}


//static 
void GPUState::Weld( const GPUState& state, const GPUStream& stream, const GPUStreamData& data )
{
	CHECK_GL_ERROR;
	GLASSERT( stream.stride > 0 );

	ShaderManager* shadman = ShaderManager::Instance();

	int flags = 0;
	// State Flags
	flags |= (data.texture0 ) ? ShaderManager::TEXTURE0 : 0;
	flags |= (data.texture0 && (data.texture0->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE0_ALPHA_ONLY : 0;
	if ( flags & ShaderManager::TEXTURE0 ) GLASSERT( stream.nTexture0 );

	/*
	flags |= (state.HasTexture1() ) ? ShaderManager::TEXTURE1 : 0;
	flags |= (state.HasTexture1() && (state.texture1->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE1_ALPHA_ONLY : 0;
	*/

	flags |= state.HasLighting();

	// Stream Flags
	flags |= stream.HasColor() ? ShaderManager::COLORS : 0;
	
	// Actual shader option flags:
	flags |= state.shaderFlags;	

	shadman->ActivateShader( flags );
	shadman->ClearStream();

	const Matrix4& mv = GPUState::TopMatrix( GPUState::MODELVIEW_MATRIX );

	if ( flags & ShaderManager::INSTANCE ) {
		Matrix4 vp;
		MultMatrix4( GPUState::TopMatrix( GPUState::PROJECTION_MATRIX ), GPUState::ViewMatrix(), &vp );

		shadman->SetUniform( ShaderManager::U_MVP_MAT, vp );
		GLASSERT( data.matrix );
		shadman->SetUniformArray( ShaderManager::U_M_MAT_ARR, EL_MAX_INSTANCE, data.matrix );
		GLASSERT( stream.instanceIDOffset > 0 );
		shadman->SetStreamData( ShaderManager::A_INSTANCE_ID, 1, GL_UNSIGNED_SHORT, 
			                    stream.stride, PTR( data.streamPtr, stream.instanceIDOffset ) );
		
		if ( flags & ShaderManager::COLOR_PARAM ) {
			GLASSERT( data.colorParam );
			shadman->SetUniformArray( ShaderManager::U_COLOR_PARAM_ARR, EL_MAX_INSTANCE, data.colorParam );
		}
		if ( flags & ShaderManager::BONE_FILTER ) {
			GLASSERT( data.boneFilter );
			shadman->SetUniformArray( ShaderManager::U_FILTER_PARAM_ARR, EL_MAX_INSTANCE, data.boneFilter );
		}
		if ( flags & ShaderManager::TEXTURE0_XFORM ) {
			GLASSERT( data.texture0XForm );
			shadman->SetUniformArray( ShaderManager::U_TEXTURE0_XFORM_ARR, EL_MAX_INSTANCE, data.texture0XForm );
		}
		if ( flags & ShaderManager::TEXTURE0_CLIP ) {
			GLASSERT( data.texture0Clip );
			shadman->SetUniformArray( ShaderManager::U_TEXTURE0_CLIP_ARR, EL_MAX_INSTANCE, data.texture0Clip );
		}
		if ( flags & ShaderManager::TEXTURE0_COLORMAP ) {
			GLASSERT( data.texture0ColorMap );
			shadman->SetUniformArray( ShaderManager::U_TEXTURE0_COLORMAP_ARR, EL_MAX_INSTANCE, data.texture0ColorMap );
		}

		GLASSERT( data.controlParam );
		shadman->SetUniformArray( ShaderManager::U_CONTROL_PARAM_ARR, EL_MAX_INSTANCE, data.controlParam );
	}
	else {
		Matrix4 mvp;
		MultMatrix4( GPUState::TopMatrix( GPUState::PROJECTION_MATRIX ), mv, &mvp );
		shadman->SetUniform( ShaderManager::U_MVP_MAT, mvp );
		if ( flags & ShaderManager::COLOR_PARAM ) {
			GLASSERT( data.colorParam );
			shadman->SetUniform( ShaderManager::U_COLOR_PARAM, data.colorParam[0] );
		}
		if ( flags & ShaderManager::BONE_FILTER ) {
			GLASSERT( data.boneFilter );
			shadman->SetUniform( ShaderManager::U_FILTER_PARAM, data.boneFilter[0] );
		}
		if ( data.controlParam ) {
			shadman->SetUniform( ShaderManager::U_CONTROL_PARAM, data.controlParam[0] );
		}
		else {
			Vector4F v = { 1, 1, 1, 1 };
			shadman->SetUniform( ShaderManager::U_CONTROL_PARAM, v );
		}
		if ( flags & ShaderManager::TEXTURE0_XFORM ) {
			GLASSERT( data.texture0XForm );
			shadman->SetUniform( ShaderManager::U_TEXTURE0_XFORM, data.texture0XForm[0] );
		}
		if ( flags & ShaderManager::TEXTURE0_CLIP ) {
			GLASSERT( data.texture0Clip );
			shadman->SetUniform( ShaderManager::U_TEXTURE0_CLIP, data.texture0Clip[0] );
		}
		if ( flags & ShaderManager::TEXTURE0_COLORMAP ) {
			GLASSERT( data.texture0ColorMap );
			shadman->SetUniform( ShaderManager::U_TEXTURE0_COLORMAP, data.texture0ColorMap[0] );
		}
	}

	// Texture1
	/*
	glActiveTexture( GL_TEXTURE1 );

	if ( stream.HasTexture1() ) {
		glBindTexture( GL_TEXTURE_2D, state.texture1->GLID() );
		shadman->SetTexture( 1, state.texture1 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE1, stream.nTexture1, GL_FLOAT, stream.stride, PTR( data.streamPtr, stream.texture1Offset ) );
	}
	CHECK_GL_ERROR;
	*/

	// Texture0
	glActiveTexture( GL_TEXTURE0 );

	if ( flags & ShaderManager::TEXTURE0 ) {
		GLASSERT( data.texture0 );
		glBindTexture( GL_TEXTURE_2D, data.texture0->GLID() );
		shadman->SetTexture( 0, data.texture0 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE0, stream.nTexture0, GL_FLOAT, stream.stride, PTR( data.streamPtr, stream.texture0Offset ) );
	}
	CHECK_GL_ERROR;

	// vertex
	if ( stream.HasPos() ) {
		shadman->SetStreamData( ShaderManager::A_POS, stream.nPos, GL_FLOAT, stream.stride, PTR( data.streamPtr, stream.posOffset ) );	 
	}

	// color
	if ( stream.HasColor() ) {
		GLASSERT( stream.nColor == 4 );
		shadman->SetStreamData( ShaderManager::A_COLOR, 4, GL_FLOAT, stream.stride, PTR( data.streamPtr, stream.colorOffset ) );	 
	}

	// bones
	if ( flags & ShaderManager::BONE_XFORM ) {
		GLASSERT( stream.boneOffset );	// could be zero...but that would be odd.
		int count = EL_MAX_BONES;
		if ( flags & ShaderManager::INSTANCE ) {
			count *= EL_MAX_INSTANCE;
		}

		Vector3F d[EL_MAX_BONES*EL_MAX_INSTANCE];
		for( int i=0; i<EL_MAX_INSTANCE; ++i ) {
			for( int j=0; j<EL_MAX_BONES; ++j ) {
				const BoneData::Bone& bone = data.bones[i].bone[j];
				d[i*EL_MAX_BONES+j].Set( bone.angleRadians, bone.dy, bone.dz );
			}
		}
		shadman->SetUniformArray( ShaderManager::U_BONEXFORM, count, d );
		shadman->SetStreamData( ShaderManager::A_BONE_ID, 1, GL_UNSIGNED_SHORT, stream.stride, PTR( data.streamPtr, stream.boneOffset ) );
	}
	else if ( flags & ShaderManager::BONE_FILTER ) {
		shadman->SetStreamData( ShaderManager::A_BONE_ID, 1, GL_UNSIGNED_SHORT, stream.stride, PTR( data.streamPtr, stream.boneOffset ) );
	}

	// lighting 
	if ( flags & (ShaderManager::LIGHTING_DIFFUSE | ShaderManager::LIGHTING_HEMI) ) {

		Vector4F dirEye = GPUState::ViewMatrix() * GPUState::directionWC;
		GLASSERT( Equal( dirEye.Length(), 1.f, 0.01f ));
		Vector3F dirEye3 = { dirEye.x, dirEye.y, dirEye.z };

		// NOTE: the normal matrix can be used because the game doesn't support scaling.
		shadman->SetUniform( ShaderManager::U_NORMAL_MAT, mv );
		shadman->SetUniform( ShaderManager::U_LIGHT_DIR, dirEye3 );
		shadman->SetUniform( ShaderManager::U_AMBIENT, GPUState::ambient );
		shadman->SetUniform( ShaderManager::U_DIFFUSE, GPUState::diffuse );
		shadman->SetStreamData( ShaderManager::A_NORMAL, 3, GL_FLOAT, stream.stride, PTR( data.streamPtr, stream.normalOffset ) );	 
	}

	// color multiplier is always set
	shadman->SetUniform( ShaderManager::U_COLOR_MULT, state.color );

	// Blend
	if ( (state.stateFlags & BLEND_MASK) != currentBlend ) {
		currentBlend = BlendMode(state.stateFlags & BLEND_MASK);
		switch( state.stateFlags & BLEND_MASK ) {
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
		default:
			GLASSERT( 0 );
		}
	}

	// Depth Write
	DepthWrite depthWrite = DepthWrite(state.stateFlags & DEPTH_WRITE_MASK);
	if ( (depthWrite == DEPTH_WRITE_TRUE) && (currentDepthWrite == DEPTH_WRITE_FALSE) ) {
		glDepthMask( GL_TRUE );
		currentDepthWrite = DEPTH_WRITE_TRUE;
	}
	else if ( depthWrite == DEPTH_WRITE_FALSE && currentDepthWrite == DEPTH_WRITE_TRUE ) {
		glDepthMask( GL_FALSE );
		currentDepthWrite = DEPTH_WRITE_FALSE;
	}

	// Depth test
	DepthTest depthTest = DepthTest(state.stateFlags & DEPTH_TEST_MASK);
	if ( (depthTest == DEPTH_TEST_TRUE) && (currentDepthTest == DEPTH_TEST_FALSE) ) {
		glEnable( GL_DEPTH_TEST );
		currentDepthTest = DEPTH_TEST_TRUE;
	}
	else if ( (depthTest == DEPTH_TEST_FALSE) && (currentDepthTest == DEPTH_TEST_TRUE) ) {
		glDisable( GL_DEPTH_TEST );
		currentDepthTest = DEPTH_TEST_FALSE;
	}

	// Color test
	ColorWrite colorWrite = ColorWrite(state.stateFlags & COLOR_WRITE_MASK);
	if ( (colorWrite == COLOR_WRITE_TRUE) && (currentColorWrite == COLOR_WRITE_FALSE)) {
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		currentColorWrite = COLOR_WRITE_TRUE;
	}
	else if ( (colorWrite == COLOR_WRITE_FALSE) && (currentColorWrite == COLOR_WRITE_TRUE) ) {
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		currentColorWrite = COLOR_WRITE_FALSE;
	}

	StencilMode stencilMode = StencilMode(state.stateFlags & STENCIL_MASK);
	if ( stencilMode != currentStencilMode ) {
		currentStencilMode = stencilMode;
		switch( stencilMode ) {
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


void GPUState::Clear( float r, float g, float b, float a )
{
	GLASSERT( currentStencilMode == STENCIL_OFF );	// could be fixed, just implemented for 'off'
	glStencilMask( GL_TRUE );	// Stupid stupid opengl behavior.
	glClearStencil( 0 );
	glClearColor( r, g, b, a );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glStencilMask( GL_FALSE );
}

#if 0
/*static*/void GPUState::SetMatrixMode( MatrixType m )
{
	matrixMode = m;
}
#else

/*static*/ void GPUState::SetViewport( int w, int h )
{
	glViewport( 0, 0, w, h );
}


/*static*/ void GPUState::SetOrthoTransform( int screenWidth, int screenHeight )
{
	Matrix4 ortho;
	ortho.SetOrtho( 0, (float)screenWidth, (float)screenHeight, 0, -100.f, 100.f );
	SetMatrix( PROJECTION_MATRIX, ortho );

	Matrix4 identity;
	SetMatrix( MODELVIEW_MATRIX, identity );
	CHECK_GL_ERROR;
}


/*static*/ void GPUState::SetPerspectiveTransform( const grinliz::Matrix4& perspective )
{
	SetMatrix( PROJECTION_MATRIX, perspective );
	CHECK_GL_ERROR;
}


/*static*/ void GPUState::SetCameraTransform( const grinliz::Matrix4& camera )
{
	// In normalized coordinates.
	GLASSERT( mvStack.NumMatrix() == 1 );
	mvStack.Set( camera );
	CHECK_GL_ERROR;
}


/*static*/ void GPUState::SetScissor( int x, int y, int w, int h )
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


void GPUState::DrawQuads( const GPUStream& stream, const GPUStreamData& data, int nQuad )
{
	ClearShaderFlag( ShaderManager::INSTANCE );
	quadsDrawn += nQuad;
	++drawCalls;

	GLASSERT( data.vertexBuffer );

	glBindBufferX( GL_ARRAY_BUFFER, data.vertexBuffer );
	Weld( *this, stream, data );
	glDrawArrays( GL_QUADS, 0, nQuad*4 );
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
}


void GPUState::Draw( const GPUStream& stream, const GPUStreamData& data, int nIndex, int instances )
{
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

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	if ( data.indexPtr ) {	
		GLASSERT( data.indexBuffer == 0 );
		if ( data.vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, data.vertexBuffer );
			GLASSERT( data.streamPtr == 0 );
		}
		Weld( *this, stream, data );

		glDrawElements( primitive,	//GL_TRIANGLES except where debugging 
						nIndex*instances, GL_UNSIGNED_SHORT, data.indexPtr );

		if ( data.vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, 0 );
		}
	}
	else {
		GLASSERT( data.vertexBuffer );
		GLASSERT( data.indexBuffer );

		// This took a long time to figure. OpenGL is a state machine, except, apparently, when it isn't.
		// The VBOs impact how the SetxxxPointers work. If they aren't set, then the wrong thing gets
		// bound. What a PITA. And ugly design problem with OpenGL.
		//
		glBindBufferX( GL_ARRAY_BUFFER, data.vertexBuffer );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, data.indexBuffer );
		Weld( *this, stream, data );

		glDrawElements( primitive,	// GL_TRIANGLES except when debugging 
						nIndex*instances, GL_UNSIGNED_SHORT, 0 );

		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}
	CHECK_GL_ERROR;
}


void GPUState::Draw( const GPUStream& stream, Texture* texture, const void* vertex, int nIndex, const uint16_t* indices )
{
	GPUStreamData data;
	data.streamPtr = vertex;
	data.indexPtr = indices;
	data.texture0 = texture;
	Draw( stream, data, nIndex );
}


void GPUState::Draw( const GPUStream& stream, Texture* texture, const GPUVertexBuffer& vertex, int nIndex, const uint16_t* indices )
{
	GPUStreamData data;
	data.vertexBuffer = vertex.ID();
	data.indexPtr = indices;
	data.texture0 = texture;
	Draw( stream, data, nIndex );
}


void GPUState::Draw( const GPUStream& stream, Texture* texture, const GPUVertexBuffer& vertex,	int nIndex, const GPUIndexBuffer& index )
{
	GPUStreamData data;
	data.vertexBuffer = vertex.ID();
	data.indexBuffer = index.ID();
	data.texture0 = texture;
	Draw( stream, data, nIndex );
}


void GPUState::DrawQuad( Texture* texture, const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positive )
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

	GPUStream stream( pos[0] );
	Draw( stream, texture, pos, 6, index );
}


void GPUState::DrawLine( const grinliz::Vector3F p0, const grinliz::Vector3F p1 )
{
	Vector3F v[2] = { p0, p1 };
	static const U16 index[2] = { 0, 1 };

	GPUStream stream;
	stream.stride = sizeof(Vector3F);
	stream.nPos = 3;

	primitive = GL_LINES;
	Draw( stream, 0, v, 2, index );
	primitive = GL_TRIANGLES;
}


void GPUState::DrawArrow( const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positive, float width )
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

		GPUStream stream( pos[0] );
		Draw( stream, 0, pos, 3, index );
	}
}


void GPUState::SwitchMatrixMode( MatrixType type )
{
	matrixMode = type;
	CHECK_GL_ERROR;
}


void GPUState::PushMatrix( MatrixType type )
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
}


void GPUState::MultMatrix( MatrixType type, const grinliz::Matrix4& m )
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


void GPUState::PopMatrix( MatrixType type )
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


void GPUState::SetMatrix( MatrixType type, const Matrix4& m )
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


const grinliz::Matrix4& GPUState::TopMatrix( MatrixType type )
{
	if ( type == MODELVIEW_MATRIX ) {
		return mvStack.Top();
	}
	return projStack.Top();
}


const grinliz::Matrix4& GPUState::ViewMatrix()
{
	return mvStack.Bottom();
}


int GPUState::HasLighting() const
{
	return shaderFlags & ( ShaderManager::LIGHTING_DIFFUSE | ShaderManager::LIGHTING_HEMI );
}


void GPUStream::Clear()
{
	memset( this, 0, sizeof(*this) );
}


GPUStream::GPUStream( const Vertex& vertex )
{
	Clear();

	stride = sizeof( Vertex );
	nPos = 3;
	posOffset = Vertex::POS_OFFSET;
	nNormal = 3;
	normalOffset = Vertex::NORMAL_OFFSET;
	nTexture0 = 2;
	texture0Offset = Vertex::TEXTURE_OFFSET;
	boneOffset = Vertex::BONE_ID_OFFSET;
}


GPUStream::GPUStream( const InstVertex& vertex )
{
	Clear();

	stride = sizeof( InstVertex );
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


GPUStream::GPUStream( const PTVertex& vertex )
{
	Clear();
	stride = sizeof( PTVertex );
	nPos = 3;
	posOffset = PTVertex::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = PTVertex::TEXTURE_OFFSET;
}


GPUStream::GPUStream( const PTVertex2& vertex )
{
	Clear();
	stride = sizeof( PTVertex2 );
	nPos = 2;
	posOffset = PTVertex2::POS_OFFSET;
	nTexture0 = 2;
	texture0Offset = PTVertex2::TEXTURE_OFFSET;

}


CompositingShader::CompositingShader( BlendMode _blend )
{
	SetBlendMode( _blend );
	SetDepthWrite( false );
	SetDepthTest( false );
}


LightShader::LightShader( int flag, BlendMode blend )
{
	SetBlendMode( blend );
	SetShaderFlag( flag );
}


ParticleShader::ParticleShader() : GPUState() 
{
	SetDepthWrite( false );
	SetDepthTest( true );
	SetBlendMode( BLEND_ADD );
	SetShaderFlag( ShaderManager::PREMULT );
}

