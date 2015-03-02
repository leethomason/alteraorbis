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
#include "shadermanager.h"
#include "../grinliz/glgeometry.h"

#include "platformgl.h"

using namespace grinliz;

GPUVertexBuffer::GPUVertexBuffer( const void* vertex, int size )
{
	this->sizeInBytes = size;

	CHECK_GL_ERROR;
	glGenBuffersX( 1, (GLuint*) &id );
	glBindBufferX( GL_ARRAY_BUFFER, id );
	glBufferDataX( GL_ARRAY_BUFFER, size, vertex, vertex ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}


GPUVertexBuffer::~GPUVertexBuffer() 
{
	glDeleteBuffersX( 1, (GLuint*) &id );
}


void GPUVertexBuffer::Upload( const void* data, int nBytes, int start )
{
	CHECK_GL_ERROR;
	glBindBufferX( GL_ARRAY_BUFFER, id );
	glBufferSubDataX( GL_ARRAY_BUFFER, start, nBytes, data );
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}


GPUIndexBuffer::GPUIndexBuffer( const U16* index, int nIndex )
{
	this->nIndex = nIndex;
	U32 dataSize  = sizeof(U16)*nIndex;

	CHECK_GL_ERROR;
	glGenBuffersX( 1, (GLuint*) &id );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, id );
	glBufferDataX( GL_ELEMENT_ARRAY_BUFFER, dataSize, index, index ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
}


GPUIndexBuffer::~GPUIndexBuffer() 
{
	if ( id ) {
		glDeleteBuffersX( 1, (GLuint*) &id );
		id = 0;
	}
}


void GPUIndexBuffer::Upload( const uint16_t* data, int count, int start )
{
	CHECK_GL_ERROR;
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, id );
	glBufferSubDataX( GL_ELEMENT_ARRAY_BUFFER, start*sizeof(uint16_t), count*sizeof(uint16_t), data );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	CHECK_GL_ERROR;
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


GPUDevice* GPUDevice::instance = 0;

GPUDevice::GPUDevice()
{
	matrixMode			= MODELVIEW_MATRIX;
	currentBlend		= BLEND_NONE;
	currentDepthTest	= DEPTH_TEST_TRUE;
	currentDepthWrite	= DEPTH_WRITE_TRUE;
	currentColorWrite	= COLOR_WRITE_TRUE;
	currentStencilMode	= STENCIL_OFF;
	primitive			= GL_TRIANGLES;
	trianglesDrawn		= 0;
	drawCalls			= 0;
	uid					= 0;
	for( int i=0; i<3; ++i ) matrixDepth[i] = 0;

	ambient.Set( 0.3f, 0.3f, 0.3f, 1.0f );
	directionWC.Set( 1, 1, 1, 0 );
	directionWC.Normalize();
	diffuse.Set( 0.7f, 0.7f, 0.7f, 1.0f );

	for( int i=0; i<NUM_UI_LAYERS; ++i ) {
		// How big?
		// Allow 32k indices -> 2*indices/3 vertices
		static const int INDEX_COUNT = 32*1024;			// 32k indices-> 64k buffer
		static const int VERTEX_MEM  = 16*1024 * 16;	//				 256k buffer	
		vertexBuffer[i] = new GPUVertexBuffer( 0, VERTEX_MEM );
		indexBuffer[i]  = new GPUIndexBuffer( 0,  INDEX_COUNT );
		uiVBOInUse[i] = false;
	}

	currentQuadBuf = 0;
	for( int i=0; i<NUM_QUAD_BUFFERS; ++i ) {
		quadBuffer[i] = new GPUVertexBuffer( 0, 1024 );
	}
	ResetState();
	quadIndexBuffer = 0;
}

GPUDevice::~GPUDevice()
{
	for( int i=0; i<NUM_UI_LAYERS; ++i ) {
		delete vertexBuffer[i];
		delete indexBuffer[i];
	}
	for( int i=0; i<NUM_QUAD_BUFFERS; ++i ) {
		delete quadBuffer[i];
	}
	delete quadIndexBuffer;
	instance = 0;
}


void GPUDevice::ResetState()
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


int GPUDevice::Upload( const GPUState& state, const GPUStream& stream, const GPUStreamData& data, int start, int instanceTotal )
{
	ShaderManager* shadman = ShaderManager::Instance();
	int nInstance = instanceTotal - start;
	nInstance = Min( nInstance, shadman->MaxInstances() );
	GLASSERT( nInstance > 0 );

	int flags = shadman->ShaderFlags();

	const Matrix4& mv = TopMatrix( MODELVIEW_MATRIX );

	Matrix4 mvp;
    MultMatrix4( TopMatrix( PROJECTION_MATRIX ), mv, &mvp );
    shadman->SetUniform( ShaderManager::U_MVP_MAT, mvp );
 
	shadman->SetUniformArray( ShaderManager::U_M_MAT_ARR, nInstance, 
							  data.matrix ? &data.matrix[start] : identity );

	if ( flags & ShaderManager::COLOR_PARAM ) {
		GLASSERT( data.colorParam );
		shadman->SetUniformArray( ShaderManager::U_COLOR_PARAM_ARR, nInstance, &data.colorParam[start] );
	}
	if ( flags & ShaderManager::BONE_FILTER ) {
		GLASSERT( data.boneFilter );
		shadman->SetUniformArray( ShaderManager::U_FILTER_PARAM_ARR, nInstance, &data.boneFilter[start] );
	}
	if ( flags & ShaderManager::TEXTURE0_XFORM ) {
		GLASSERT( data.texture0XForm );
		shadman->SetUniformArray( ShaderManager::U_TEXTURE0_XFORM_ARR, nInstance, &data.texture0XForm[start] );
	}
	if ( flags & ShaderManager::TEXTURE0_CLIP ) {
		GLASSERT( data.texture0Clip );
		shadman->SetUniformArray( ShaderManager::U_TEXTURE0_CLIP_ARR, nInstance, &data.texture0Clip[start] );
	}
	if ( flags & ShaderManager::TEXTURE0_COLORMAP ) {
		GLASSERT( data.texture0ColorMap );
		shadman->SetUniformArray( ShaderManager::U_TEXTURE0_COLORMAP_ARR, nInstance, &data.texture0ColorMap[start] );
	}

	for( int i=0; i<EL_MAX_INSTANCE; ++i ) {
		defaultControl[i].Init();
	}
	shadman->SetUniformArray(ShaderManager::U_CONTROL_PARAM_ARR, nInstance,
		data.controlParam ? data.controlParam[start].Mem() 
							: defaultControl[0].Mem());

	// Texture0
	glActiveTexture( GL_TEXTURE0 );

	if ( flags & ShaderManager::TEXTURE0 ) {
		if ( start == 0 ) {
			GLASSERT( data.texture0 );
			glBindTexture( GL_TEXTURE_2D, data.texture0->GLID() );
			shadman->SetTexture( 0, data.texture0 );

			shadman->SetStreamData( ShaderManager::A_TEXTURE0, 2, GL_FLOAT, stream.stride, PTR( 0, stream.texture0Offset ) );
		}
	}
	CHECK_GL_ERROR;

	// vertex
	if ( stream.HasPos() ) {
		if ( start == 0 ) {
			shadman->SetStreamData( ShaderManager::A_POS, stream.nPos, GL_FLOAT, stream.stride, PTR( 0, stream.posOffset ) );	 
		}
	}

	// color
	if ( stream.HasColor() ) {
		GLASSERT( stream.nColor == 4 );
		if ( start == 0 ) {
			shadman->SetStreamData( ShaderManager::A_COLOR, 4, GL_FLOAT, stream.stride, PTR( 0, stream.colorOffset ) );	 
		}
	}

	// bones
	if ( flags & ShaderManager::BONE_XFORM ) {
		GLASSERT( stream.boneOffset );	// could be zero...but that would be odd.
		int count = EL_MAX_BONES * nInstance;

		// This could be much better packed (by a factor of 2) by using pos/rot as 2 vector4 instead
		// of expanding to a matrix. And less work for the CPU as well. Disadvantages to keep in mind:
		// - easier to debug a set of matrices
		// - simpler shader code
		shadman->SetUniformArray( ShaderManager::U_BONEXFORM, count, &data.bones[start*EL_MAX_BONES] );
		if ( start == 0 ) {
			shadman->SetStreamData(ShaderManager::A_BONE_ID, 1, GL_FLOAT, stream.stride, PTR(0, stream.boneOffset));
		}
	}
	else if ( flags & ShaderManager::BONE_FILTER ) {
		if ( start == 0 ) {
			shadman->SetStreamData(ShaderManager::A_BONE_ID, 1, GL_FLOAT, stream.stride, PTR(0, stream.boneOffset));
		}
	}

	// lighting 
	if ( flags & ShaderManager::LIGHTING ) {

		Vector4F dirEye = ViewMatrix() * directionWC;
		GLASSERT( Equal( dirEye.Length(), 1.f, 0.01f ));
		Vector3F dirEye3 = { dirEye.x, dirEye.y, dirEye.z };

		// NOTE: the normal matrix can be used because the game doesn't support scaling.
		shadman->SetUniform( ShaderManager::U_NORMAL_MAT, mv );
		shadman->SetUniform( ShaderManager::U_LIGHT_DIR, dirEye3 );
		shadman->SetUniform( ShaderManager::U_AMBIENT, ambient );
		shadman->SetUniform( ShaderManager::U_DIFFUSE, diffuse );
		if ( start == 0 ) {
			shadman->SetStreamData( ShaderManager::A_NORMAL, 3, GL_FLOAT, stream.stride, PTR( 0, stream.normalOffset ) );	 
		}
	}

	// color multiplier is always set
	shadman->SetUniform( ShaderManager::U_COLOR_MULT, state.Color() );

	return nInstance;
}


void GPUDevice::Weld( const GPUState& state, const GPUStream& stream, const GPUStreamData& data )
{
	CHECK_GL_ERROR;
	GLASSERT( stream.stride > 0 );

	ShaderManager* shadman = ShaderManager::Instance();

	int flags = 0;
	// State Flags
	flags |= (data.texture0 ) ? ShaderManager::TEXTURE0 : 0;

	flags |= state.HasLighting();

	// Stream Flags
	flags |= stream.HasColor() ? ShaderManager::COLORS : 0;

	// Actual shader option flags:
	flags |= state.ShaderFlags();	

	shadman->ActivateShader( flags );
	shadman->ClearStream();

	// Blend
	if ( (state.StateFlags() & BLEND_MASK) != currentBlend ) {
		currentBlend = BlendMode(state.StateFlags() & BLEND_MASK);
		switch( state.StateFlags() & BLEND_MASK ) {
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
	DepthWrite depthWrite = DepthWrite(state.StateFlags() & DEPTH_WRITE_MASK);
	if ( (depthWrite == DEPTH_WRITE_TRUE) && (currentDepthWrite == DEPTH_WRITE_FALSE) ) {
		glDepthMask( GL_TRUE );
		currentDepthWrite = DEPTH_WRITE_TRUE;
	}
	else if ( depthWrite == DEPTH_WRITE_FALSE && currentDepthWrite == DEPTH_WRITE_TRUE ) {
		glDepthMask( GL_FALSE );
		currentDepthWrite = DEPTH_WRITE_FALSE;
	}

	// Depth test
	DepthTest depthTest = DepthTest(state.StateFlags() & DEPTH_TEST_MASK);
	if ( (depthTest == DEPTH_TEST_TRUE) && (currentDepthTest == DEPTH_TEST_FALSE) ) {
		glEnable( GL_DEPTH_TEST );
		currentDepthTest = DEPTH_TEST_TRUE;
	}
	else if ( (depthTest == DEPTH_TEST_FALSE) && (currentDepthTest == DEPTH_TEST_TRUE) ) {
		glDisable( GL_DEPTH_TEST );
		currentDepthTest = DEPTH_TEST_FALSE;
	}

	// Color test
	ColorWrite colorWrite = ColorWrite(state.StateFlags() & COLOR_WRITE_MASK);
	if ( (colorWrite == COLOR_WRITE_TRUE) && (currentColorWrite == COLOR_WRITE_FALSE)) {
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		currentColorWrite = COLOR_WRITE_TRUE;
	}
	else if ( (colorWrite == COLOR_WRITE_FALSE) && (currentColorWrite == COLOR_WRITE_TRUE) ) {
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		currentColorWrite = COLOR_WRITE_FALSE;
	}

	StencilMode stencilMode = StencilMode(state.StateFlags() & STENCIL_MASK);
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


GPUIndexBuffer* GPUDevice::GetUIIBO( int id )
{
	GLASSERT( id >= 0 && id < NUM_UI_LAYERS );
	return indexBuffer[id];
}


GPUVertexBuffer*  GPUDevice::GetUIVBO( int id )
{
	GLASSERT( id >= 0 && id < NUM_UI_LAYERS );
	GLASSERT( uiVBOInUse[id] == false );
	uiVBOInUse[id] = true;
	return vertexBuffer[id];
}

void GPUDevice::Clear( float r, float g, float b, float a )
{
	GLASSERT( currentStencilMode == STENCIL_OFF );	// could be fixed, just implemented for 'off'
	glStencilMask( GL_TRUE );	// Stupid stupid opengl behavior.
	glClearStencil( 0 );
	glClearColor( r, g, b, a );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glStencilMask( GL_FALSE );

	for( int i=0; i<NUM_UI_LAYERS; ++i ) {
		uiVBOInUse[i] = false;
	}
	currentQuadBuf = 0;
}


void GPUDevice::SetViewport( int w, int h )
{
	glViewport( 0, 0, w, h );
}


void GPUDevice::SetOrthoTransform( int screenWidth, int screenHeight )
{
	Matrix4 ortho;
	ortho.SetOrtho( 0, (float)screenWidth, (float)screenHeight, 0, -100.f, 100.f );
	SetMatrix( PROJECTION_MATRIX, ortho );

	Matrix4 identity;
	SetMatrix( MODELVIEW_MATRIX, identity );
	CHECK_GL_ERROR;
}


void GPUDevice::SetPerspectiveTransform( const grinliz::Matrix4& perspective )
{
	SetMatrix( PROJECTION_MATRIX, perspective );
	CHECK_GL_ERROR;
}


void GPUDevice::SetCameraTransform( const grinliz::Matrix4& camera )
{
	// In normalized coordinates.
	GLASSERT( mvStack.NumMatrix() == 1 );
	mvStack.Set( camera );
	CHECK_GL_ERROR;
}


void GPUDevice::DrawQuads( const GPUState& state, const GPUStream& stream, const GPUStreamData& data, int nQuads )
{
	static const int N = 6 * (96 * 1024 / 6);
	if (!quadIndexBuffer) {
		U16* mem = new U16[N];
		U16 b = 0;
		for ( int i = 0; i < N; i+=6, b+=4) {
			// 0 1 2
			// 0 2 3
			mem[i + 0] = b+0;
			mem[i + 1] = b+1;
			mem[i + 2] = b+2;
			mem[i + 3] = b+0;
			mem[i + 4] = b+2;
			mem[i + 5] = b+3;
		}
		quadIndexBuffer = new GPUIndexBuffer(mem, N);
		delete[] mem;
	}
	GLASSERT(data.indexBuffer == 0);
	GPUStreamData d2 = data;
	d2.indexBuffer = quadIndexBuffer->ID();

	int nIndex = nQuads * 6;
	GLASSERT(nIndex < N);
	trianglesDrawn += nIndex / 3;
	++drawCalls;

	glBindBufferX(GL_ARRAY_BUFFER, d2.vertexBuffer);
	glBindBufferX(GL_ELEMENT_ARRAY_BUFFER, d2.indexBuffer);

	Weld(state, stream, d2);
	Upload(state, stream, d2, 0, 1);
	glDrawElements(GL_TRIANGLES, nIndex, GL_UNSIGNED_SHORT, 0);

	glBindBufferX(GL_ARRAY_BUFFER, 0);
	glBindBufferX(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void GPUDevice::Draw( const GPUState& state, const GPUStream& stream, const GPUStreamData& data, int first, int nIndex, int instances )
{
	if ( nIndex == 0 )
		return;
	if ( instances == 0 ) instances = 1;	// fix errors in old code

	GLASSERT( (primitive != GL_TRIANGLES ) || (nIndex % 3 == 0 ));

	trianglesDrawn += instances * nIndex / 3;

	GLASSERT( data.vertexBuffer );
	GLASSERT( data.indexBuffer );

	// This took a long time to figure. OpenGL is a state machine, except, apparently, when it isn't.
	// The VBOs impact how the SetxxxPointers work. If they aren't set, then the wrong thing gets
	// bound. What a PITA. And ugly design problem with OpenGL.
	//
	glBindBufferX( GL_ARRAY_BUFFER, data.vertexBuffer );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, data.indexBuffer );
	Weld( state, stream, data );

	int start = 0;
	while ( start < instances ) {
		int n = Upload( state, stream, data, start, instances );
		start += n;

		++drawCalls;
		glDrawElementsInstanced( primitive,	// GL_TRIANGLES except when debugging 
								 nIndex, GL_UNSIGNED_SHORT, (const void*)(first*2), n );
	}
	glBindBufferX( GL_ARRAY_BUFFER, 0 );
	glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );

	CHECK_GL_ERROR;
}


void GPUDevice::DrawQuad( const GPUState& state, Texture* texture, const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positive )
{
	PTVertex pos[4] = { 
		{ { p0.x, p0.y, p0.z }, { 0, 0 } },
		{ { p1.x, p0.y, p0.z }, { 1, 0 } },
		{ { p1.x, p1.y, p1.z }, { 1, 1 } },
		{ { p0.x, p1.y, p1.z }, { 0, 1 } },
	};
	
	if ( !positive ) {
		pos[1].pos.Set(p0.x, p1.y, p1.z);
		pos[2].pos.Set(p1.x, p1.y, p1.z);
		pos[3].pos.Set(p1.x, p0.y, p0.z);
		
		pos[1].tex.Set(0, 1);
		pos[2].tex.Set(1, 1);
		pos[3].tex.Set(1, 0);
	}

	quadBuffer[currentQuadBuf]->Upload( pos, 4*sizeof(PTVertex), 0 );

	GPUStream stream( pos[0] );
	GPUStreamData data;
	data.texture0 = texture;
	data.vertexBuffer = quadBuffer[currentQuadBuf]->ID();

	DrawQuads( state, stream, data, 1 );
	++currentQuadBuf;
	if ( currentQuadBuf == NUM_QUAD_BUFFERS ) {
//		GLASSERT( 0 ); // we are re-using VBOs and possibly stalling GPU.
		currentQuadBuf = 0;
	}
}


void GPUDevice::DrawArrow( const GPUState& state, const grinliz::Vector3F p0, const grinliz::Vector3F p1, float width )
{
	const Vector3F UP = { 0, 1, 0 };
	Vector3F normal; 
	CrossProduct( UP, p1-p0, &normal );
	if ( normal.Length() > 0.001f ) {
		normal.Normalize();
		normal = normal * width;

		PTVertex v[4] = { 
			{ p0-normal, { 0, 0 } },
			{ p1, { 1, 1 } },
			{ p1, { 1, 1 } },
			{ p0 + normal, { 1, 0 } },
		};
		quadBuffer[currentQuadBuf]->Upload(v, 4 * sizeof(PTVertex), 0);
		GPUStream stream(v[0]);
		GPUStreamData data;
		data.vertexBuffer = quadBuffer[currentQuadBuf]->ID();

		DrawQuads(state, stream, data, 1);
		++currentQuadBuf;
		if ( currentQuadBuf == NUM_QUAD_BUFFERS ) {
			//GLASSERT( 0 ); // we are re-using VBOs and possibly stalling GPU
			currentQuadBuf = 0;
		}
	}
}


void GPUDevice::SwitchMatrixMode( MatrixType type )
{
	matrixMode = type;
	CHECK_GL_ERROR;
}


void GPUDevice::PushMatrix( MatrixType type )
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


void GPUDevice::MultMatrix( MatrixType type, const grinliz::Matrix4& m )
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


void GPUDevice::PopMatrix( MatrixType type )
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


void GPUDevice::SetMatrix( MatrixType type, const Matrix4& m )
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


const grinliz::Matrix4& GPUDevice::TopMatrix( MatrixType type )
{
	if ( type == MODELVIEW_MATRIX ) {
		return mvStack.Top();
	}
	return projStack.Top();
}


const grinliz::Matrix4& GPUDevice::ViewMatrix()
{
	return mvStack.Bottom();
}


int GPUState::HasLighting() const
{
	return shaderFlags & ShaderManager::LIGHTING;
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
//	nTexture0 = 2;
	texture0Offset = Vertex::TEXTURE_OFFSET;
	boneOffset = Vertex::BONE_ID_OFFSET;
}


GPUStream::GPUStream( GamuiType )
{
	Clear();
	stride = sizeof( gamui::Gamui::Vertex );
	nPos = 2;
	posOffset = gamui::Gamui::Vertex::POS_OFFSET;
//	nTexture0 = 2;
	texture0Offset = gamui::Gamui::Vertex::TEX_OFFSET;
}


GPUStream::GPUStream( const PTVertex& vertex )
{
	Clear();
	stride = sizeof( PTVertex );
	nPos = 3;
	posOffset = PTVertex::POS_OFFSET;
//	nTexture0 = 2;
	texture0Offset = PTVertex::TEXTURE_OFFSET;
}


GPUStream::GPUStream( const PTVertex2& vertex )
{
	Clear();
	stride = sizeof( PTVertex2 );
	nPos = 2;
	posOffset = PTVertex2::POS_OFFSET;
//	nTexture0 = 2;
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

