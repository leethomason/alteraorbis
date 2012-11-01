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
	//void Upload( const Vertex* data, int size, int start );

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

	GPUStream( const Vertex* vertex );
	GPUStream( const InstVertex* vertex );
	GPUStream( GamuiType );
	GPUStream( const PTVertex* vertex );
	GPUStream( const PTVertex2* vertex );

	void Clear();

	bool HasPos() const			{ return nPos > 0; }
	bool HasNormal() const		{ return nNormal > 0; }
	bool HasColor() const		{ return nColor > 0; }
	bool HasTexture0() const	{ return nTexture0 > 0; }
	bool HasTexture1() const	{ return nTexture1 > 0; }
};


struct GPUStreamData
{
	GPUStreamData() : streamPtr(0), indexPtr(0), vertexBuffer(0), indexBuffer(0), texture0(0), matrix(0), param(0), param4(0), bones(0) {}

	const void*			streamPtr;
	const uint16_t*		indexPtr;
	U32					vertexBuffer;
	U32					indexBuffer;

	Texture*			texture0;
	grinliz::Matrix4*	matrix;
	grinliz::Vector4F*	param;
	grinliz::Matrix4*	param4;
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

	enum StencilMode {
		STENCIL_OFF,		// ignore stencil
		STENCIL_WRITE,		// draw commands write to stencil
		STENCIL_SET,		// draw if stencil is set
		STENCIL_CLEAR		// draw if stencil is clear
	};

	enum BlendMode {
		BLEND_NONE,			// opaque
		BLEND_NORMAL,		// a, 1-a
		BLEND_ADD			// additive blending
	};

	static void ResetState();
	static void Clear();

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

	void SetStencilMode( StencilMode value ) { stencilMode = value; }
	void SetDepthWrite( bool value ) { depthWrite = value; }
	void SetDepthTest( bool value ) { depthTest = value; }
	void SetColorWrite( bool value ) { colorWrite = value; }

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

	void Draw( const GPUStream& stream, Texture* texture, const void* vertex,				int nIndex, const uint16_t* indices );
	void Draw( const GPUStream& stream, Texture* texture, const GPUVertexBuffer& vertex,	int nIndex, const uint16_t* index );
	void Draw( const GPUStream& stream, Texture* texture, const GPUVertexBuffer& vertex,	int nIndex, const GPUIndexBuffer& index );

	void DrawLine( const grinliz::Vector3F p0, const grinliz::Vector3F p1 );
	void DrawQuad( Texture* texture, const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positiveWinding=true );
	void DrawArrow( const grinliz::Vector3F p0, const grinliz::Vector3F p1, bool positiveWinding=true, float width=0.4f );

	int SortOrder()	const { 
		if ( blend == BLEND_NORMAL ) return 2;
		if ( blend == BLEND_ADD ) return 1;
		return 0;
	}

	static void ResetTriCount()	{ trianglesDrawn = 0; drawCalls = 0; }
	static int TrianglesDrawn() { return trianglesDrawn; }
	static int DrawCalls()		{ return drawCalls; }

	static bool SupportsVBOs();


	GPUState() : shaderFlags( 0 ),
				 blend( BLEND_NONE ),
				 depthWrite( true ), depthTest( true ),
				 colorWrite( true ),
				 stencilMode( STENCIL_OFF ),
				 hemisphericalLighting( false )
	{
		color.Set( 1,1,1,1 );
	}

protected:

	static void Weld( const GPUState&, const GPUStream&, const GPUStreamData& );
	//void DebugDraw( const PTVertex* v, int nIndex, const U16* index );

private:

	static void SwitchMatrixMode( MatrixType type );	
	static MatrixType		matrixMode;		// Note this is static and global!
	static int vboSupport;

	static MatrixStack mvStack;
	static MatrixStack projStack;

	static BlendMode currentBlend;
	static bool currentDepthTest;
	static bool currentDepthWrite;
	static bool currentColorWrite;
	static StencilMode currentStencilMode;

protected:
	static int		primitive;
	static int		trianglesDrawn;
	static int		drawCalls;
	static uint32_t uid;
	static int		matrixDepth[3];

	static const void* PTR( const void* base, int offset ) {
		return (const void*)((const U8*)base + offset);
	}

	BlendMode		blend;
	bool			depthWrite;
	bool			depthTest;
	bool			colorWrite;
	StencilMode		stencilMode;
	bool			hemisphericalLighting;
	grinliz::Color4F color;	// actual state color; render a bunch of stuff in black, for example.
							// not to be confused with per-vertex or per-instance color, also supported.
	int				shaderFlags;

public:
	bool operator==( const GPUState& s ) const {
		return (   blend == s.blend 
				&& depthWrite == s.depthWrite
				&& depthTest == s.depthTest
				&& colorWrite == s.colorWrite
				&& stencilMode == s.stencilMode
				&& hemisphericalLighting == s.hemisphericalLighting
				&& color == s.color
				&& shaderFlags == s.shaderFlags );
	}
	U32 Hash() const {
		grinliz::Color4U8 c = grinliz::Convert_4F_4U8( color );
		U32 h[8] = {	(U32)blend, 
						depthWrite ? 1 : 0, 
						depthTest ? 1 : 0, 
						colorWrite ? 1 : 0, 
						(U32)stencilMode, 
						hemisphericalLighting ? 1 : 0,
						shaderFlags,
						(c.r) | (c.g<<8) | (c.b<<16) | (c.a<<24)
					};
		return grinliz::Random::Hash( h,8*sizeof(U32) );
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
	void SetBlend( BlendMode _blend )				{ this->blend = _blend; }
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
