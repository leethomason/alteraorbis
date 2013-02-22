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



/*	A class to manage shaders. No actual "shader" object is returned; a
	Shader is made current with ActivateShader() and then Set methods
	are used.
*/
class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	static ShaderManager* Instance() { if ( !instance ) instance = new ShaderManager(); return instance; }

	enum {				
		INSTANCE			= (1<<0),		// Use instancing. Up to 16 uniform matrices contain the model
											// transform. The instance attribute must be in the vertex data.

		// Texture state.
		TEXTURE0			= (1<<1),		// Texture is in use. Note that the sampling state (linear, nearest) is saved with the texture.
		TEXTURE0_ALPHA_ONLY = (1<<2),		// Texture is only alpha, which composites differently.

		TEXTURE1			= (1<<3),
		TEXTURE1_ALPHA_ONLY	= (1<<4),
		
		// Colors and Lighting
		COLORS				= (1<<5),		// Per-vertex colors.
		LIGHTING_DIFFUSE	= (1<<6),		// Diffuse lighting. Requires per vertex normals, 
											// light direction, ambient color, and diffuse color.
		LIGHTING_HEMI		= (1<<7),		// Hemisperical lighting. Like diffuse, but uses a 
											// different light model.

		// Color features.
		PREMULT				= (1<<8),		// convert to pre-multiplied in the fragment shader
		EMISSIVE			= (1<<9),		// interpret the alpha channel as emission.
		EMISSIVE_EXCLUSIVE  = (1<<10),		// everything not emissive is black

		COLOR_PARAM			= (1<<11),		// Apply per model color.
		BONE_FILTER			= (1<<12),		// Apply per model bone filtering

		// Features:
		PROCEDURAL			= (1<<13),		// Engage the procedural renderer, and use a Matrix param.
		BONE_XFORM			= (1<<14),
	};

	void DeviceLoss();
	void AddDeviceLossHandler( IDeviceLossHandler* handler );
	void RemoveDeviceLossHandler( IDeviceLossHandler* handler );

	// ActivateShader should be called *before* the sets, so  the sets are correctly validated. 
	void ActivateShader( int flags );

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
		U_COLOR_PARAM_ARR,	// params for instancing
		U_FILTER_PARAM_ARR,
		U_CONTROL_PARAM_ARR,
		U_PARAM4_ARR,	// params for instancing

		U_NORMAL_MAT,
		U_COLOR_MULT,
		U_LIGHT_DIR,
		U_AMBIENT,
		U_DIFFUSE,
		U_RADIUS,
		U_COLOR_PARAM,
		U_FILTER_PARAM,
		U_CONTROL_PARAM,
		U_PARAM4,
		U_BONEXFORM,

		U_TEXTURE0,
		U_TEXTURE1,

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
	grinliz::GLString fixedpipeVert, fixedpipeFrag;
	grinliz::CStr<9> hashStr;

	//grinliz::GLString fixedpipe7Vert, fixedpipe7Frag;
	Shader* active;
	grinliz::CDynArray<int> activeStreams;
	U32 totalCompileTime;

	void LoadProgram( const char* name, grinliz::GLString *store );
	Shader* CreateProgram( int flag );
	void DeleteProgram( Shader* );

	void AppendFlag( grinliz::GLString* str, const char* flag, int set, int value=1 );
	void AppendConst( grinliz::GLString* str, const char* name, int value );
};

#endif //  XENOENGINE_SHADERMANAGER_INCLUDED