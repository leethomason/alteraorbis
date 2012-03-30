#ifndef UFOATTACK_STATE_MANAGER_INCLUDED
#define UFOATTACK_STATE_MANAGER_INCLUDED

// Be sure to NOT include the gl platform header, so this can be 
// used as a platform-independent header file, and still exclude
// the gl headers.
#include <stdint.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glcolor.h"

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

	GPUStream() :  stride( 0 ),
				nPos( 0 ), posOffset( 0 ), 
				nTexture0( 0 ), texture0Offset( 0 ),
				nTexture1( 0 ), texture1Offset( 0 ), 
				nNormal( 0 ), normalOffset( 0 ),
				nColor( 0 ), colorOffset( 0 ), instanceIDOffset( 0 ) {}

	GPUStream( const Vertex* vertex );
	GPUStream( const InstVertex* vertex );

	enum GamuiType { kGamuiType };
	GPUStream( GamuiType );
	GPUStream( const PTVertex2* vertex );
	void Clear();

	bool HasPos() const			{ return nPos > 0; }
	bool HasNormal() const		{ return nNormal > 0; }
	bool HasColor() const		{ return nColor > 0; }
	bool HasTexture0() const	{ return nTexture0 > 0; }
	bool HasTexture1() const	{ return nTexture1 > 0; }
};


/* WARNING: this gets copied around, and slices.
   Sub-classes are for initialization. They can't
   store data.
*/
class GPUShader 
{
public:
	virtual ~GPUShader();

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
		BLEND_NORMAL,		// normal but not good...
		BLEND_ADD			// additive blending
	};

	static void ResetState();
	static void Clear();

	// Set the top level state. The engine has top level (root transforms)
	// for the screen size, scissor, and transform:
	static void SetViewport( int w, int h );
	static void SetOrthoTransform( int width, int height, int rotation );
	static void SetPerspectiveTransform( float left, float right, 
										 float bottom, float top, 
										 float near, float far,
										 int rotation );
	// The top level V matrix in perspective mode.
	static void SetCameraTransform( const grinliz::Matrix4& camera );
	static void SetScissor( int x, int y, int w, int h );
	
	void SetStream( const GPUStream& stream, const void* vertex, int nIndex, const uint16_t* indices ) 
	{
		GLASSERT( stream.stride > 0 );
		GLASSERT( nIndex % 3 == 0 );

		this->stream = stream;
		this->streamPtr = vertex;
		this->indexPtr = indices;
		this->nIndex = nIndex;
		this->vertexBuffer = 0;
		this->indexBuffer = 0;
	}


	void SetStream( const GPUStream& stream, const GPUVertexBuffer& vertex, int nIndex, const GPUIndexBuffer& index ) 
	{
		GLASSERT( stream.stride > 0 );
		GLASSERT( nIndex % 3 == 0 );
		GLASSERT( vertex.IsValid() );
		GLASSERT( index.IsValid() );

		this->stream = stream;
		this->streamPtr = 0;
		this->indexPtr = 0;
		this->nIndex = nIndex;
		this->vertexBuffer = vertex.ID();
		this->indexBuffer = index.ID();
	}


	void SetStream( const GPUStream& stream, const GPUVertexBuffer& vertex, int nIndex, const uint16_t* index ) 
	{
		GLASSERT( stream.stride > 0 );
		GLASSERT( nIndex % 3 == 0 );
		GLASSERT( vertex.IsValid() );
		GLASSERT( index );

		this->stream = stream;
		this->streamPtr = 0;
		this->indexPtr = index;
		this->nIndex = nIndex;
		this->vertexBuffer = vertex.ID();
		this->indexBuffer = 0;
	}

	void SetTexture0( Texture* tex ) { texture0 = tex; }
	bool HasTexture0() const { return texture0 != 0; }
	bool HasLighting( grinliz::Vector4F* dir, grinliz::Vector4F* ambient, grinliz::Vector4F* diffuse ) const { 
		if ( dir ) *dir = direction;
		if ( ambient ) ambient->Set( this->ambient.r, this->ambient.g, this->ambient.b, this->ambient.a );
		if ( diffuse ) diffuse->Set( this->diffuse.r, this->diffuse.g, this->diffuse.b, this->diffuse.a );
		return direction.Length() > 0;
	}

	void SetTexture1( Texture* tex ) { texture1 = tex; }
	bool HasTexture1() const { return texture1 != 0; }

	void SetColor( float r, float g, float b )				{ color.r = r; color.g = g; color.b = b; color.a = 1; }
	void SetColor( float r, float g, float b, float a )		{ color.r = r; color.g = g; color.b = b; color.a = a; }
	void SetColor( const grinliz::Color4F& c )				{ color = c; }
	void SetColor( const grinliz::Color4U8& color ) {
		static const float INV = 1.0f/255.0f;
		grinliz::Color4F c = { (float)color.r*INV, (float)color.g*INV, (float)color.b*INV, (float)color.a*INV };
		SetColor( c );
	}
	void SetEmissive( bool em )					{ emissive = em; }
	void SetEmissiveExclusive( bool ee )		{ emissiveExclusive = ee; }
	void SetHemisphericalLighting( bool hemi )	{ hemisphericalLighting = hemi; }

	void SetInstancing( bool value ) { instancing = value; }
	void InstanceMatrix( int i, const grinliz::Matrix4& mat ) { 
		GLASSERT( i >= 0 && i < EL_MAX_INSTANCE );
		instanceMatrix[i] = mat; 
	}

	void SetStencilMode( StencilMode value ) { stencilMode = value; }
	void SetDepthWrite( bool value ) { depthWrite = value; }
	void SetDepthTest( bool value ) { depthTest = value; }
	void SetColorWrite( bool value ) { colorWrite = value; }

	static void PushMatrix( MatrixType type );
	static void SetMatrix( MatrixType type, const grinliz::Matrix4& m );
	static void MultMatrix( MatrixType type, const grinliz::Matrix4& m );
	static void PopMatrix( MatrixType type );

	// Special, because there is a texture matrix per texture unit. Painful painful system.
	// Mask is a bit mask:
	// 1: unit0, 2: unit1, 3:both units
	static void PushTextureMatrix( int mask );
	static void MultTextureMatrix( int mask, const grinliz::Matrix4& m );
	static void PopTextureMatrix( int mask );

	static const grinliz::Matrix4& TopMatrix( MatrixType type );
	static const grinliz::Matrix4& TopTextureMatrix( int unit );
	static const grinliz::Matrix4& ViewMatrix();

	void Draw( int instances=0 );

	void DrawQuad( const grinliz::Vector3F p0, const grinliz::Vector3F p1 );

	int SortOrder()	const { 
		if ( blend == BLEND_NORMAL ) return 2;
		if ( blend == BLEND_ADD ) return 1;
		return 0;
	}

	static void ResetTriCount()	{ trianglesDrawn = 0; drawCalls = 0; }
	static int TrianglesDrawn() { return trianglesDrawn; }
	static int DrawCalls()		{ return drawCalls; }

	static bool SupportsVBOs();

protected:

	GPUShader() : texture0( 0 ), texture1( 0 ), 
				 streamPtr( 0 ), nIndex( 0 ), indexPtr( 0 ),
				 vertexBuffer( 0 ), indexBuffer( 0 ),
				 instancing( false ),
				 premult( false ),
				 emissive( false ),
				 emissiveExclusive( false ),
				 blend( BLEND_NONE ),
				 depthWrite( true ), depthTest( true ),
				 colorWrite( true ),
				 stencilMode( STENCIL_OFF ),
				 hemisphericalLighting( false )
	{
		color.Set( 1, 1, 1, 1 );
		direction.Set( 0, 0, 0, 0 );
		ambient.Set( 0, 0, 0, 0 );
		diffuse.Set( 0, 0, 0, 0 );
	}

	static void SetState( const GPUShader& );

private:

	static void SwitchMatrixMode( MatrixType type );	
	static MatrixType		matrixMode;		// Note this is static and global!
	static int vboSupport;

	// Seem to do okay on MODELVIEW and PERSPECTIVE stacks, but
	// not so much on TEXTURE. Use our own texture stack, one for each texture unit.
	static MatrixStack textureStack[2];
	static bool textureXFormInUse[2];
	static MatrixStack mvStack;
	static MatrixStack projStack;

	static BlendMode currentBlend;
	static bool currentDepthTest;
	static bool currentDepthWrite;
	static bool currentColorWrite;
	static StencilMode currentStencilMode;

	static void SetTextureXForm( int unit );

protected:
	static int trianglesDrawn;
	static int drawCalls;
	static uint32_t uid;
	static int			matrixDepth[3];

	static const void* PTR( const void* base, int offset ) {
		return (const void*)((const U8*)base + offset);
	}

	Texture* texture0;
	Texture* texture1;

	GPUStream		stream;
	const void*		streamPtr;
	int				nIndex;
	const uint16_t* indexPtr;
	U32				vertexBuffer;
	U32				indexBuffer;
	bool			instancing;
	bool			premult;
	bool			emissive;		// interpret alpha as emissive
	bool			emissiveExclusive;

	BlendMode	blend;
	bool		depthWrite;
	bool		depthTest;
	bool		colorWrite;
	StencilMode	stencilMode;

	bool				hemisphericalLighting;
	grinliz::Color4F	color;
	grinliz::Color4F	ambient;
	grinliz::Vector4F	direction;
	grinliz::Color4F	diffuse;

	grinliz::Matrix4	instanceMatrix[EL_MAX_INSTANCE];
};


class CompositingShader : public GPUShader
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


class LightShader : public GPUShader
{
public:
	/** Texture or color. Writes & tests z. Enables lighting. */
	LightShader( const grinliz::Color4F& ambient, 
		         const grinliz::Vector4F& direction, 
				 const grinliz::Color4F& diffuse, 
				 BlendMode blend = BLEND_NONE );
	~LightShader();
	
protected:
};


class FlatShader : public GPUShader
{
public:
	FlatShader()	{}	// totally vanilla
};


class ParticleShader : public GPUShader
{
public:
	ParticleShader() : GPUShader() 
	{
		depthWrite = false;
		depthTest = true;
		premult = true;
		blend = BLEND_ADD;
	}

};
#endif
