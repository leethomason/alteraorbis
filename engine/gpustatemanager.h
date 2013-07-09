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

#ifndef UFOATTACK_STATE_MANAGER_INCLUDED
#define UFOATTACK_STATE_MANAGER_INCLUDED

// Be sure to NOT include the gl platform header, so this can be 
// used as a platform-independent header file, and still exclude
// the gl headers.
#include <stdint.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glrandom.h"

#include "vertex.h"
#include "enginelimits.h"


class Texture;
class Particle;

class MatrixStack
{
public:
	MatrixStack();
	~MatrixStack();

	void Push();
	void Pop();
	void Set( const grinliz::Matrix4& m )			{ stack[index] = m; }
	void Multiply( const grinliz::Matrix4& m );

	const grinliz::Matrix4& Top() const				{ GLASSERT( index < MAX_DEPTH ); return stack[index]; }
	const grinliz::Matrix4& Bottom() const			{ return stack[0]; }
	int NumMatrix() const							{ return index+1; }

private:
	enum { MAX_DEPTH = 4 };
	int index;
	grinliz::Matrix4 stack[MAX_DEPTH];
};


class GPUBuffer
{
public:
	GPUBuffer() : id( 0 )			{}
	bool IsValid() const			{ return id != 0; }
	U32 ID() const					{ return id; }
	void Clear()					{ id = 0; }

protected:	
	U32 id;
};


class GPUVertexBuffer : public GPUBuffer
{
public:
	// a null value for vertex will create an empty buffer
	static GPUVertexBuffer Create( const void* vertex, int size, int nVertex );
	void Upload( const void* data, int nBytes, int start );

	GPUVertexBuffer() : GPUBuffer() {}
	void Destroy();
private:
};


class GPUIndexBuffer : public GPUBuffer
{
public:
	static GPUIndexBuffer Create( const uint16_t* index, int nIndex );
	//void Upload( const uint16_t* data, int size, int start );

	GPUIndexBuffer() : GPUBuffer() {}
	void Destroy();
};


class GPUInstanceBuffer : public GPUBuffer
{
public:
	static GPUInstanceBuffer Create( const uint8_t* index, int nIndex );
	//void Upload( const uint8_t* data, int size, int start );

	GPUInstanceBuffer() : GPUBuffer() {}
	void Destroy();
};


/* Class that defines the data layout of a stream.
*/
struct GPUStream {
	// Defines float sized components.
	int stride;
	int nPos;		
	int posOffset;
	int nTexture0;
	int texture0Offset;
	int nTexture1;
	int texture1Offset;
	int nNormal;
	int normalOffset;
	int nColor;
	int colorOffset;
	int instanceIDOffset;
	int boneOffset;

	enum GamuiType { kGamuiType };

	GPUStream() :  stride( 0 ),
				nPos( 0 ), posOffset( 0 ), 
				nTexture0( 0 ), texture0Offset( 0 ),
				nTexture1( 0 ), texture1Offset( 0 ), 
				nNormal( 0 ), normalOffset( 0 ),
				nColor( 0 ), colorOffset( 0 ), instanceIDOffset( 0 ), boneOffset( 0 ) {}

	// Uses type: GPUStream stream( Vertex );
	GPUStream( const Vertex& vertex );
	GPUStream( const InstVertex& vertex );
	GPUStream( GamuiType );
	GPUStream( const PTVertex& vertex );
	GPUStream( const PTVertex2& vertex );

	void Clear();

	bool HasPos() const			{ return nPos > 0; }
	bool HasNormal() const		{ return nNormal > 0; }
	bool HasColor() const		{ return nColor > 0; }
	bool HasTexture0() const	{ return nTexture0 > 0; }
	bool HasTexture1() const	{ return nTexture1 > 0; }
};


/* Class that actually contains the pointers, buffers, and uniforms for rendering.
*/
struct GPUStreamData
{
	GPUStreamData() : streamPtr(0), indexPtr(0), vertexBuffer(0), indexBuffer(0), 
					  texture0(0), 
					  matrix(0), 
					  colorParam(0), 
					  boneFilter(0), 
					  controlParam(0),
					  texture0XForm(0),
					  texture0Clip(0),
					  texture0ColorMap(0),
					  bones(0)
	{}

	const void*			streamPtr;
	const uint16_t*		indexPtr;
	U32					vertexBuffer;
	U32					indexBuffer;

	Texture*			texture0;
	grinliz::Matrix4*	matrix;
	grinliz::Vector4F*	colorParam;
	grinliz::Vector4F*	boneFilter;
	grinliz::Vector4F*  controlParam;
	grinliz::Vector4F*	texture0XForm;
	grinliz::Vector4F*	texture0Clip;
	grinliz::Matrix4*	texture0ColorMap;	// 3 vec4 per instance
	BoneData*			bones;
};


/* WARNING: this gets copied around, and slices.
   Sub-classes are for initialization. They can't
   store data.
*/
class GPUState 
{
public:
	enum MatrixType {
		MODELVIEW_MATRIX,
		PROJECTION_MATRIX,
	};

	// FLag values used for state ordering - be sure Blend is in the correct high bits!

	enum StencilMode {
		STENCIL_OFF		= 0,			// ignore stencil
		STENCIL_WRITE	= (1<<0),		// draw commands write to stencil
		STENCIL_SET		= (1<<1),		// draw if stencil is set
		STENCIL_CLEAR	= (1<<2),		// draw if stencil is clear
		STENCIL_MASK    = STENCIL_WRITE | STENCIL_SET | STENCIL_CLEAR
	};

	enum DepthWrite {
		DEPTH_WRITE_TRUE	= 0,
		DEPTH_WRITE_FALSE	= (1<<3),
		DEPTH_WRITE_MASK	= DEPTH_WRITE_FALSE
	};

	enum DepthTest {
		DEPTH_TEST_TRUE		= 0,
		DEPTH_TEST_FALSE	= (1<<4),
		DEPTH_TEST_MASK		= DEPTH_TEST_FALSE
	};

	enum ColorWrite {
		COLOR_WRITE_TRUE	= 0,
		COLOR_WRITE_FALSE	= (1<<5),
		COLOR_WRITE_MASK	= COLOR_WRITE_FALSE
	};

	enum Lighting {
		LIGHTING_LAMBERT	= 0,
		LIGHTING_HEMI		= (1<<6),
		LIGHTING_MASK		= LIGHTING_HEMI
	};

	enum BlendMode {
		BLEND_NONE		= 0,			// opaque
		BLEND_NORMAL	= (1<<7),		// a, 1-a
		BLEND_ADD		= (1<<8),		// additive blending
		BLEND_MASK		= BLEND_NORMAL | BLEND_ADD
	};
	enum { STATE_BITS = 9 };


	static void ResetState();
	static void Clear( float r, float g, float b, float a );

	// Set the top level state. The engine has top level (root transforms)
	// for the screen size, scissor, and transform:
	static void SetViewport( int w, int h );
	static void SetOrthoTransform( int width, int height );
	static void SetPerspectiveTransform( const grinliz::Matrix4& perspective );
	// The top level V matrix in perspective mode.
	static void SetCameraTransform( const grinliz::Matrix4& camera );
	static void SetScissor( int x, int y, int w, int h );

	int HasLighting() const;

	void SetColor( float r, float g, float b )				{ color.r = r; color.g = g; color.b = b; color.a = 1; }
	void SetColor( float r, float g, float b, float a )		{ color.r = r; color.g = g; color.b = b; color.a = a; }
	void SetColor( const grinliz::Color4F& c )				{ color = c; }
	void SetColor( const grinliz::Color4U8& color ) {
		static const float INV = 1.0f/255.0f;
		grinliz::Color4F c = { (float)color.r*INV, (float)color.g*INV, (float)color.b*INV, (float)color.a*INV };
		SetColor( c );
	}

	// Set any of the flags (that are boolean) from ShaderManager
	void SetShaderFlag( int flag )				{ shaderFlags |= flag; }
	void ClearShaderFlag( int flag )			{ shaderFlags &= (~flag); }
	int  ShaderFlags() const					{ return shaderFlags; }
	int  StateFlags() const						{ return stateFlags; }

	void SetStencilMode( StencilMode value )	{ stateFlags &= (~STENCIL_MASK); stateFlags |= value; }
	void SetDepthWrite( bool value )			{ stateFlags &= (~DEPTH_WRITE_MASK); stateFlags |= (value ? DEPTH_WRITE_TRUE : DEPTH_WRITE_FALSE); }
	void SetDepthTest( bool value )				{ stateFlags &= (~DEPTH_TEST_MASK); stateFlags |= (value ? DEPTH_TEST_TRUE : DEPTH_TEST_FALSE); }
	void SetColorWrite( bool value )			{ stateFlags &= (~COLOR_WRITE_MASK); stateFlags |= (value ? COLOR_WRITE_TRUE : COLOR_WRITE_FALSE); }
	void SetBlendMode( BlendMode value )		{ stateFlags &= (~BLEND_MASK); stateFlags |= value; }

	static void PushMatrix( MatrixType type );
	static void SetMatrix( MatrixType type, const grinliz::Matrix4& m );
	static void MultMatrix( MatrixType type, const grinliz::Matrix4& m );
	static void PopMatrix( MatrixType type );

	static const grinliz::Matrix4& TopMatrix( MatrixType type );
	static const grinliz::Matrix4& ViewMatrix();

	// More input to the draw call. Must be set up by the engine.
	static grinliz::Color4F		ambient;
	static grinliz::Vector4F	directionWC;
	static grinliz::Color4F		diffuse;

	void Draw( const GPUStream& stream, const GPUStreamData& data, int nIndex, int nInstance=0 );
	void DrawQuads( const GPUStream& stream, const GPUStreamData& data, int nQuad );

	void Draw( const GPUStream& stream, Texture* texture, const void* vertex,				int nIndex, const uint16_t* indices );
	void Draw( const GPUStream& stream, Texture* texture, const GPUVertexBuffer& vertex,	int nIndex, const uint16_t* index );
	void Draw( const GPUStream& stream, Texture* texture, const GPUVertexBuffer& vertex,	int nIndex, const GPUIndexBuffer& index );

	void DrawLine( const grinliz::Vector3F p0, const grinliz::Vector3F p1 );
	void DrawQuad( Texture* texture, const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positiveWinding=true );
	void DrawArrow( const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positiveWinding=true, float width=0.4f );

	static void ResetTriCount()	{ quadsDrawn = 0; trianglesDrawn = 0; drawCalls = 0; }
	static int TrianglesDrawn() { return trianglesDrawn; }
	static int QuadsDrawn()		{ return quadsDrawn; }
	static int DrawCalls()		{ return drawCalls; }

	static bool SupportsVBOs();


	GPUState() : stateFlags( 0 ),
				 shaderFlags( 0 )
	{
		color.Set( 1,1,1,1 );
	}

protected:

	static void Weld( const GPUState&, const GPUStream&, const GPUStreamData& );

private:

	static void SwitchMatrixMode( MatrixType type );	
	static MatrixType		matrixMode;		// Note this is static and global!
	static int vboSupport;

	static MatrixStack mvStack;
	static MatrixStack projStack;

	static BlendMode	currentBlend;
	static DepthTest	currentDepthTest;
	static DepthWrite	currentDepthWrite;
	static ColorWrite	currentColorWrite;
	static StencilMode	currentStencilMode;

protected:
	static int		primitive;
	static int		trianglesDrawn;
	static int		quadsDrawn;
	static int		drawCalls;
	static uint32_t uid;
	static int		matrixDepth[3];

	static const void* PTR( const void* base, int offset ) {
		return (const void*)((const U8*)base + offset);
	}

	int				stateFlags;
	int				shaderFlags;
	grinliz::Color4F color;	// actual state color; render a bunch of stuff in black, for example.
							// not to be confused with per-vertex or per-instance color, also supported.

public:
	bool operator==( const GPUState& s ) const {
		return (   stateFlags == s.stateFlags
				&& shaderFlags == s.shaderFlags
				&& color == s.color );
	}
};


class CompositingShader : public GPUState
{
public:
	/** Writes texture or color and neither writes nor tests z. 
		Texture/Color support
			- no texture
			- texture0, modulated by color
			- texture0 and texture1 (light map compositing)
		Blend support
	*/
	CompositingShader( BlendMode blend=BLEND_NONE );
};


class LightShader : public GPUState
{
public:
	/** Texture or color. Writes & tests z. Enables lighting. */
	LightShader( int lightFlag, BlendMode blend = BLEND_NONE );
	
protected:
};


class FlatShader : public GPUState
{
public:
	FlatShader()	{}	// totally vanilla
};


class ParticleShader : public GPUState
{
public:
	ParticleShader();
};

#endif
