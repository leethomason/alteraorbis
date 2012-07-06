#ifndef XENOENGINE_SHADERMANAGER_INCLUDED
#define XENOENGINE_SHADERMANAGER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glcontainer.h"

#include "enginelimits.h"

class Texture;
class Stream;

class IDeviceLossHandler
{
public:
	virtual void DeviceLoss() = 0;
};

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	static ShaderManager* Instance() { if ( !instance ) instance = new ShaderManager(); return instance; }

	enum {				
		TEXTURE0			= (1<<0),		// Texture is in use. Note that the sampling state (linear, nearest) is saved with the texture.
		TEXTURE0_ALPHA_ONLY = (1<<1),		// Texture is only alpha, which composites differently.
		TEXTURE0_TRANSFORM	= (1<<2),		// Texture has a texture transform: PARAM

		TEXTURE1			= (1<<4),
		TEXTURE1_ALPHA_ONLY	= (1<<5),
		TEXTURE1_TRANSFORM	= (1<<6),
		
		COLOR_PARAM			= (1<<7),		// Apply per model color. PARAM
		COLORS				= (1<<8),		// Per-vertex colors.
		COLOR_MULTIPLIER	= (1<<9),		// Global color multiplier.
		LIGHTING_DIFFUSE	= (1<<10),		// Diffuse lighting. Requires per vertex normals, 
											// light direction, ambient color, and diffuse color.
		LIGHTING_HEMI		= (1<<11),		// Hemisperical lighting. Like diffuse, but uses a 
											// different light model. FLAG
		INSTANCE			= (1<<12),		// Use instancing. Up to 16 uniform matrices contain the model
											// transform. The instance attribute must be in the vertex data.
		PREMULT				= (1<<13),		// convert to pre-multiplied in the fragment shader
		EMISSIVE			= (1<<14),		// interpret the alpha channel as emission. FLAG
		EMISSIVE_EXCLUSIVE  = (1<<15),		// everything not emissive is black
		
		BONE_FILTER			= (1<<17),		// Only show one bone. PARAM.x
		BONE_XFORM			= (1<<18),

		/*
		// Switch to different shader:
		BLUR				= (1<<18),		// requires u_radius
		BLUR_Y				= (1<<19),		//
		*/
	};

	void DeviceLoss();
	void AddDeviceLossHandler( IDeviceLossHandler* handler );
	void RemoveDeviceLossHandler( IDeviceLossHandler* handler );

	void ActivateShader( int flags );
	bool ParamNeeded() const { return active->ParamNeeded(); }
	bool BonesNeeded() const { return active->BonesNeeded(); }

	// Warning: must match gAttributeName
	enum {
		A_TEXTURE0,		// 2 or 3 comp
		A_TEXTURE1,		// 2 or 3 comp
		A_POS,			// 3 comp
		A_NORMAL,		// 2 comp
		A_COLOR,		// 3 comp
		A_INSTANCE_ID,	// int
		A_BONE_ID,		// int
		MAX_ATTRIBUTE
	};
	void ClearStream();
	void SetStreamData( int id, int count, int type, int stride, const void* data );	 
	void SetParticleStream();

	// Warning: must match gUniformName
	enum {
		U_MVP_MAT,		
		U_M_MAT_ARR,	// array for instancing
		U_PARAM_ARR,	// params for instancing

		U_NORMAL_MAT,
		U_COLOR_MULT,
		U_LIGHT_DIR,
		U_AMBIENT,
		U_DIFFUSE,
		U_RADIUS,
		U_PARAM,
		U_BONEXFORM,
		MAX_UNIFORM
	};

	void SetTexture( int index, Texture* texture );
	void SetUniform( int id, const grinliz::Matrix4& mat );
	void SetUniform( int id, const grinliz::Color4F& color ) {
		grinliz::Vector4F v = { color.r, color.g, color.b, color.a };
		SetUniform( id, v );
	}
	void SetUniform( int id, const grinliz::Vector4F& vector );
	void SetUniform( int id, const grinliz::Vector3F& vector );
	void SetUniform( int id, float value );

	void SetUniformArray( int id, int count, const grinliz::Matrix4* mat );
	void SetUniformArray( int id, int count, const grinliz::Vector4F* params );
	void SetUniformArray( int id, int count, const grinliz::Vector3F* params );

private:
	static ShaderManager* instance;
	
	struct Shader {
		void Init() {
			flags = 0;
			vertexProg = fragmentProg = prog = 0;
			for( int i=0; i<MAX_ATTRIBUTE; ++i ) attributeLoc[i] = -1;
			for( int i=0; i<MAX_UNIFORM; ++i ) uniformLoc[i] = -1;
		}
		bool ParamNeeded() const { 
			return (flags & (   ShaderManager::TEXTURE0_TRANSFORM 
				              | ShaderManager::TEXTURE1_TRANSFORM 
							  | ShaderManager::COLOR_PARAM
							  | ShaderManager::BONE_FILTER )) != 0; 
		}
		bool BonesNeeded() const {
			return (flags & (   ShaderManager::BONE_FILTER 
							  | ShaderManager::BONE_XFORM ))  != 0;
		}

		int flags;
		U32 vertexProg;
		U32 fragmentProg;
		U32 prog;

		int attributeLoc[MAX_ATTRIBUTE];
		int uniformLoc[MAX_UNIFORM];

		int GetAttributeLocation( int attribute );
		int GetUniformLocation( int uniform );
	};

	grinliz::CDynArray< IDeviceLossHandler* > deviceLossHandlers;
	grinliz::CDynArray< Shader > shaderArr;
	grinliz::GLString header;
	Shader* active;
	grinliz::CDynArray<int> activeStreams;

	Shader* CreateProgram( int flag );
	void DeleteProgram( Shader* );

	void AppendFlag( grinliz::GLString* str, const char* flag, int set, int value=1 );
	void AppendConst( grinliz::GLString* str, const char* name, int value );
};

#endif //  XENOENGINE_SHADERMANAGER_INCLUDED