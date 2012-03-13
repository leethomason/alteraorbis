#include "gpustatemanager.h"
#include "platformgl.h"
#include "texture.h"
#include "enginelimits.h"
#include "../gamui/gamui.h"	// for auto setting up gamui stream
#include "../grinliz/glperformance.h"
#include "shadermanager.h"

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


/*static*/ int			GPUShader::trianglesDrawn = 0;
/*static*/ int			GPUShader::drawCalls = 0;
/*static*/ uint32_t		GPUShader::uid = 0;
/*static*/ GPUShader::MatrixType GPUShader::matrixMode = MODELVIEW_MATRIX;
/*static*/ MatrixStack	GPUShader::textureStack[2];
/*static*/ MatrixStack	GPUShader::mvStack;
/*static*/ MatrixStack	GPUShader::projStack;
/*static*/ bool			GPUShader::textureXFormInUse[2] = { false, false };
/*static*/ int			GPUShader::vboSupport = 0;
/*static*/ bool			GPUShader::currentBlend = false;
/*static*/ bool			GPUShader::currentDepthWrite = true;
/*static*/ bool			GPUShader::currentDepthTest = true;

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

	// Matrix
	glMatrixMode( GL_MODELVIEW );
	matrixMode = MODELVIEW_MATRIX;

	// Color
	glColor4f( 1, 1, 1, 1 );

	// General config:
	glCullFace( GL_BACK );
	glEnable( GL_CULL_FACE );

	currentBlend = false;
	currentDepthTest = true;
	currentDepthWrite = true;

	CHECK_GL_ERROR;
}


void GPUShader::SetTextureXForm( int unit )
{
	if ( textureStack[unit].NumMatrix()>1 || textureXFormInUse[unit] ) 
	{
		textureXFormInUse[unit] = textureStack[unit].NumMatrix()>1;
	}
	CHECK_GL_ERROR;
}


//static 
void GPUShader::SetState( const GPUShader& ns )
{
	GRINLIZ_PERFTRACK
	CHECK_GL_ERROR;
	GLASSERT( ns.stream.stride > 0 );

	ShaderManager* shadman = ShaderManager::Instance();

	int flags = 0;
	flags |= ( ns.HasTexture0() ) ? ShaderManager::TEXTURE0 : 0;
	flags |= (textureStack[0].NumMatrix()>1 || textureXFormInUse[0]) ? ShaderManager::TEXTURE0_TRANSFORM : 0;
	flags |= (ns.HasTexture0() && (ns.texture0->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE0_ALPHA_ONLY : 0;
	flags |= (ns.stream.nTexture0 == 3 ) ? ShaderManager::TEXTURE0_3COMP : 0;

	flags |= ( ns.HasTexture1() ) ? ShaderManager::TEXTURE1 : 0;
	flags |= (textureStack[1].NumMatrix()>1 || textureXFormInUse[1]) ? ShaderManager::TEXTURE1_TRANSFORM : 0;
	flags |= (ns.HasTexture1() && (ns.texture1->Format() == Texture::ALPHA )) ? ShaderManager::TEXTURE1_ALPHA_ONLY : 0;
	flags |= (ns.stream.nTexture1 == 3 ) ? ShaderManager::TEXTURE1_3COMP : 0;

	flags |= ns.stream.HasColor() ? ShaderManager::COLORS : 0;
	flags |= ( ns.color.r != 1.f || ns.color.g != 1.f || ns.color.b != 1.f || ns.color.a != 1.f ) ? ShaderManager::COLOR_MULTIPLIER : 0;
	flags |= ns.HasLighting( 0, 0, 0 ) ? ShaderManager::LIGHTING_DIFFUSE : 0;

	flags |= ns.instancing ? ShaderManager::INSTANCE : 0;

	if ( flags & ShaderManager::INSTANCE) {
		int debug=1;
	}

	shadman->ActivateShader( flags );
	shadman->ClearStream();

	const Matrix4& mv = ns.TopMatrix( GPUShader::MODELVIEW_MATRIX );

	if ( flags & ShaderManager::INSTANCE ) {
		Matrix4 vp;
		MultMatrix4( ns.TopMatrix( GPUShader::PROJECTION_MATRIX ), ns.ViewMatrix(), &vp );
		shadman->SetUniform( ShaderManager::U_MVP_MAT, vp );
		shadman->SetUniformArray( ShaderManager::U_M_MAT_ARR, EL_MAX_INSTANCE, ns.instanceMatrix );
		GLASSERT( ns.stream.instanceIDOffset > 0 );
		// fixme: switch back to int
		shadman->SetStreamData( ShaderManager::A_INSTANCE_ID, 1, GL_INT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.instanceIDOffset ) );
	}
	else {
		Matrix4 mvp;
		MultMatrix4( ns.TopMatrix( GPUShader::PROJECTION_MATRIX ), mv, &mvp );
		shadman->SetUniform( ShaderManager::U_MVP_MAT, mvp );
	}

	// Texture1
	glActiveTexture( GL_TEXTURE1 );

	if (  ns.HasTexture1() ) {
		glBindTexture( GL_TEXTURE_2D, ns.texture1->GLID() );
		shadman->SetTexture( 1, ns.texture1 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE1, ns.stream.nTexture1, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.texture1Offset ) );
		if ( flags & ShaderManager::TEXTURE1_TRANSFORM ) {
			shadman->SetUniform( ShaderManager::U_TEXTURE1_MAT, textureStack[1].Top() );
		}	
	}
	CHECK_GL_ERROR;

	// Texture0
	glActiveTexture( GL_TEXTURE0 );

	if (  ns.HasTexture0() ) {
		glBindTexture( GL_TEXTURE_2D, ns.texture0->GLID() );
		shadman->SetTexture( 0, ns.texture0 );
		shadman->SetStreamData( ShaderManager::A_TEXTURE0, ns.stream.nTexture0, GL_FLOAT, ns.stream.stride, PTR( ns.streamPtr, ns.stream.texture0Offset ) );
		if ( flags & ShaderManager::TEXTURE0_TRANSFORM ) {
			shadman->SetUniform( ShaderManager::U_TEXTURE0_MAT, textureStack[0].Top() );
		}
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

	// lighting 
	if ( flags & ShaderManager::LIGHTING_DIFFUSE ) {
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

	// color multiplier
	if ( flags & ShaderManager::COLOR_MULTIPLIER ) {
		shadman->SetUniform( ShaderManager::U_COLOR_MULT, ns.color );
	}

	// Blend
	if ( ns.blend && !currentBlend ) {
		glEnable( GL_BLEND );
		currentBlend = true;
	}
	else if ( !ns.blend && currentBlend ) {
		glDisable( GL_BLEND );
		currentBlend = false;
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
	CHECK_GL_ERROR;
}


void GPUShader::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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


/*static*/ void GPUShader::SetOrthoTransform( int screenWidth, int screenHeight, int rotation )
{
	Matrix4 r, t;
	r.SetZRotation( (float)rotation );
	
	// the tricky bit. After rotating the ortho display, move it back on screen.
	switch (rotation) {
		case 0:
			break;
		case 90:
			t.SetTranslation( 0, (float)(-screenWidth), 0 );	
			break;
			
		default:
			GLASSERT( 0 );	// work out...
			break;
	}
	Matrix4 view2D = r*t;

	Matrix4 ortho;
	ortho.SetOrtho( 0, (float)screenWidth, (float)screenHeight, 0, -100.f, 100.f );
	SetMatrix( PROJECTION_MATRIX, ortho );

	SetMatrix( MODELVIEW_MATRIX, view2D );
	CHECK_GL_ERROR;
}


/*static*/ void GPUShader::SetPerspectiveTransform( float left, float right, 
													 float bottom, float top, 
													 float near, float far,
													 int rotation)
{
	// Give the driver hints:
	GLASSERT( rotation == 0 );
	Matrix4 perspective;
	perspective.SetFrustum( left, right, bottom, top, near, far );
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
	GLASSERT( matrixDepth[0] == 0 );
	GLASSERT( matrixDepth[1] == 0 );
	GLASSERT( matrixDepth[2] == 0 );
}


void GPUShader::Draw( int instances )
{
	GRINLIZ_PERFTRACK

	if ( nIndex == 0 )
		return;

	bool useInstancing = instances > 0;
	if ( instances == 0 ) instances = 1;
	this->SetInstancing( useInstancing );

	GLASSERT( nIndex % 3 == 0 );

	trianglesDrawn += instances * nIndex / 3;
	++drawCalls;

	if ( indexPtr ) {	
		if ( vertexBuffer ) {
			glBindBufferX( GL_ARRAY_BUFFER, vertexBuffer );
		}
		SetState( *this );

		GLASSERT( !indexBuffer );
		glDrawElements( GL_TRIANGLES, nIndex*instances, GL_UNSIGNED_SHORT, indexPtr );

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

		glDrawElements( GL_TRIANGLES, nIndex*instances, GL_UNSIGNED_SHORT, 0 );

		glBindBufferX( GL_ARRAY_BUFFER, 0 );
		glBindBufferX( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}
	CHECK_GL_ERROR;
}


void GPUShader::Debug_DrawQuad( const grinliz::Vector3F p0, const grinliz::Vector3F p1 )
{
#ifdef DEBUG
	grinliz::Vector3F pos[4] = { 
		{ p0.x, p0.y, p0.z },
		{ p1.x, p0.y, p0.z },
		{ p1.x, p1.y, p1.z },
		{ p0.x, p1.y, p1.z },
	};
	static const U16 index[6] = { 0, 2, 1, 0, 3, 2 };
	GPUStream stream;
	stream.stride = sizeof(grinliz::Vector3F);
	stream.nPos = 3;
	stream.posOffset = 0;

	SetStream( stream, pos, 6, index );
	Draw();
#endif
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


const grinliz::Matrix4& GPUShader::TopTextureMatrix( int unit )
{
	GLASSERT( unit >= 0 && unit < 2 );
	return textureStack[unit].Top();
}

const grinliz::Matrix4& GPUShader::ViewMatrix()
{
	return mvStack.Bottom();
}


void GPUShader::PushTextureMatrix( int mask )
{
	if ( mask & 1 ) textureStack[0].Push();
	if ( mask & 2 ) textureStack[1].Push();
}


void GPUShader::MultTextureMatrix( int mask, const grinliz::Matrix4& m )
{
	if ( mask & 1 ) textureStack[0].Multiply( m );
	if ( mask & 2 ) textureStack[1].Multiply( m );
}

void GPUShader::PopTextureMatrix( int mask )
{
	if ( mask & 1 ) textureStack[0].Pop();
	if ( mask & 2 ) textureStack[1].Pop();
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


CompositingShader::CompositingShader( bool _blend )
{
	blend = _blend;
	depthWrite = false;
	depthTest = false;
}


LightShader::LightShader( const Color4F& ambient, const grinliz::Vector4F& direction, const Color4F& diffuse, bool blend )
{
	GLASSERT( !(blend && alphaTest ) );	// technically fine, probably not intended behavior.

	//this->alphaTest = alphaTest;
	this->blend = blend;
	this->ambient = ambient;
	this->direction = direction;
	this->diffuse = diffuse;
}


LightShader::~LightShader()
{
}


/* static */ int PointParticleShader::particleSupport = 0;

/*static*/ bool PointParticleShader::IsSupported()
{
	if ( particleSupport == 0 ) {
		const char* extensions = (const char*)glGetString( GL_EXTENSIONS );
		const char* sprite = strstr( extensions, "point_sprite" );
		particleSupport = (sprite) ? 1 : -1;
	}
	return ( particleSupport > 0);
}


PointParticleShader::PointParticleShader()
{
	this->depthTest = true;
	this->depthWrite = false;
	this->blend = true;
}


void PointParticleShader::DrawPoints( Texture* texture, float pointSize, int start, int count )
{
	#ifdef USING_GL	
		glEnable(GL_POINT_SPRITE);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	#else
		glEnable(GL_POINT_SPRITE_OES);
		glTexEnvx(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
	#endif
	CHECK_GL_ERROR;

	GLASSERT( texture0 == 0 );
	GLASSERT( texture1 == 0 );
	
	// Will disable the texture units:
	SetState( *this );

	// Which is a big cheat because we need to bind a texture without texture coordinates.
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->GLID() );

	glPointSize( pointSize );
	CHECK_GL_ERROR;

	glDrawArrays( GL_POINTS, start, count );
	CHECK_GL_ERROR;

	#ifdef USING_GL	
		glDisable(GL_POINT_SPRITE);
	#else
		glDisable(GL_POINT_SPRITE_OES);
	#endif

	// Clear the texture thing back up.
	glDisable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, 0 );
		
	drawCalls++;
	trianglesDrawn += count;
}


QuadParticleShader::QuadParticleShader()
{
	this->depthTest = true;
	this->depthWrite = false;
	this->blend = true;
}

