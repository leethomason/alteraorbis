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

#include "shadermanager.h"
#include "platformgl.h"
#include "texture.h"
#include "../grinliz/glrandom.h"
#include "../xegame/cgame.h"
#include "../grinliz/glperformance.h"

#define DEBUG_OUTPUT

using namespace grinliz;

ShaderManager* ShaderManager::instance = 0;

static const char* gAttributeName[ShaderManager::MAX_ATTRIBUTE] =
{
	"a_uv0",
	"a_pos",
	"a_normal",
	"a_color",
	"a_boneID",
	"a_instanceID"
};


static const char* gUniformName[ShaderManager::MAX_UNIFORM] = 
{
	"u_mvpMatrix",
	"u_mMatrix",
	"u_colorParamArr",
	"u_filterParamArr",
	"u_controlParamArr",
	"u_texture0XFormArr",
	"u_texture0ClipArr",
	"u_colorMapArr",

	"u_normalMatrix",
	"u_colorMult",
	"u_lightDir",
	"u_ambient",
	"u_diffuse",

	"u_boneXForm",

	// Samplers:
	"texture0",
};


int ShaderManager::Shader::GetAttributeLocation( int attribute )
{
	GLASSERT( attribute >= 0 && attribute < ShaderManager::MAX_ATTRIBUTE );

	if ( attributeLoc[attribute] < 0 ) {
		attributeLoc[attribute] = glGetAttribLocation( prog, gAttributeName[attribute] );
		GLASSERT( attributeLoc[attribute] >= 0 );
	}
	return attributeLoc[attribute];
}


int ShaderManager::Shader::GetUniformLocation( int uniform )
{
	GLASSERT( uniform >= 0 && uniform < ShaderManager::MAX_UNIFORM );

	if ( uniformLoc[uniform] < 0 ) {
		const char* name = gUniformName[uniform];
		uniformLoc[uniform] = glGetUniformLocation( prog, name );
		GLASSERT( uniformLoc[uniform] >= 0 );
		if ( uniformLoc[uniform] < 0 ) {
			GLOUTPUT_REL(( "Uniform id=%d name=%s\n", uniform, name )); 
		}
	}
	return uniformLoc[uniform];
}


ShaderManager::ShaderManager() : active(0), totalCompileTime(0)
{
	LoadProgram( "fixedpipe.vert", &fixedpipeVert );
	LoadProgram( "fixedpipe.frag", &fixedpipeFrag );

	U32 hash0 = Random::Hash( fixedpipeVert.c_str(), fixedpipeVert.size() );
	U32 hash1 = Random::Hash( fixedpipeFrag.c_str(), fixedpipeFrag.size() );
	U32 hash2 = Random::Hash( glGetString( GL_VENDOR ), -1 );
	U32 hash3 = Random::Hash( glGetString( GL_RENDERER ), -1 );
	U32 hash4 = Random::Hash( glGetString( GL_VERSION ), -1 );
	U32 hash = hash0 ^ hash1 ^ hash2 ^ hash3 ^ hash4;

	hashStr.Format( "%x", hash );
	vertexArrayID = 0;
}


ShaderManager::~ShaderManager()
{
	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].prog ) {
			DeleteProgram( &shaderArr[i] );
		}
	}
	if (vertexArrayID) {
		//glDeleteVertexArrays(1, &vertexArrayID);
	}
}


ShaderManager* ShaderManager::Instance() 
{ 
	if (!instance) instance = new ShaderManager(); 
	if (instance->vertexArrayID == 0) {
		//glGenVertexArrays(1, &instance->vertexArrayID);
		//glBindVertexArray(instance->vertexArrayID);
	}
	return instance; 
}


void ShaderManager::LoadProgram(const char* name, GLString* str)
{
	CStr<256> path;
	path.Format("res/%s", name);
	GLString fullPath;
	GetSystemPath(GAME_APP_DIR, path.c_str(), &fullPath);

	FILE* fp = fopen(fullPath.c_str(), "r");
	GLASSERT(fp);

	static const int SIZE = 100;
	char buf[SIZE];
	int count = 0;
	while ((count = fread(buf, 1, SIZE, fp)) > 0) {
		str->append(buf, count);
	}
	fclose(fp);
}


void ShaderManager::DeviceLoss() 
{
	// The programs are already deleted.
	active = 0;
	activeStreams.Clear();
	shaderArr.Clear();

	for( int i=0; i<deviceLossHandlers.Size(); ++i ) {
		deviceLossHandlers[i]->DeviceLoss();
	}
	if (vertexArrayID) {
		//glDeleteVertexArrays(1, &vertexArrayID);
		vertexArrayID = 0;
	}
}


void ShaderManager::AddDeviceLossHandler( IDeviceLossHandler* handler )
{
	deviceLossHandlers.Push( handler );
}


void ShaderManager::RemoveDeviceLossHandler( IDeviceLossHandler* handler )
{
	for( int i=0; i<deviceLossHandlers.Size(); ++i ) {
		if ( deviceLossHandlers[i] == handler ) {
			deviceLossHandlers.SwapRemove( i );
			return;
		}
	}
	GLASSERT( 0 );
}


void ShaderManager::ClearStream()
{
	while( !activeStreams.Empty() ) {
		glDisableVertexAttribArray( activeStreams.Pop() );
	}
	CHECK_GL_ERROR;
}


void ShaderManager::SetStreamData( int id, int size, int type, int stride, const void* data )
{
	CHECK_GL_ERROR;
	int loc = active->GetAttributeLocation( id );
	GLASSERT( loc >= 0 );
	if ( gDebugging ) {
		//const float* ft = (const float*)data;
		for( int i=0; i<activeStreams.Size(); ++i ) {
			GLASSERT( activeStreams[i] != loc );
		}
	}
	CHECK_GL_ERROR;
	glVertexAttribPointer( loc, size, type, 0, stride, data );
	CHECK_GL_ERROR;
	glEnableVertexAttribArray(loc);
	CHECK_GL_ERROR;

	activeStreams.Push( loc );
}


void ShaderManager::SetUniform( int id, float value )
{
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform1f( loc, value );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniform( int id, const grinliz::Matrix4& value )
{
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	if ( loc < 0 ) {
		GLOUTPUT_REL(( "SetUniform failed. id=%d.\n", id ));
	}
	glUniformMatrix4fv( loc, 1, false, value.Mem() );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniform( int id, const grinliz::Vector4F& value )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform4fv( loc, 1, &value.x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniform( int id, const grinliz::Vector3F& value )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform3fv( loc, 1, &value.x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniformArray(int id, int count, const grinliz::Matrix4* mat)
{
	matrixCache.Clear();
	for (int i = 0; i < count; ++i) {
		float* mem = matrixCache.PushArr(16);
		memcpy(mem, mat[i].Mem(), sizeof(float) * 16);
	}

	CHECK_GL_ERROR;
	GLASSERT(mat);
	int loc = active->GetUniformLocation(id);
	GLASSERT(loc >= 0);
	glUniformMatrix4fv(loc, count, false, matrixCache.Mem());
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniformArray( int id, int count, const grinliz::Vector4F* v )
{
	CHECK_GL_ERROR;
	GLASSERT( v );
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform4fv( loc, count, &v->x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniformArray( int id, int count, const grinliz::Vector3F* v )
{
	CHECK_GL_ERROR;
	GLASSERT( v );
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform3fv( loc, count, &v->x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetTexture( int index, Texture* texture )
{
	GLASSERT( texture );
	//char name[9] = "texture0";
	//name[7] = '0' + index;

	int loc = active->GetUniformLocation( U_TEXTURE0 + index );
	GLASSERT( loc >= 0 );
	glUniform1i( loc, index );
	CHECK_GL_ERROR;
}


void ShaderManager::AppendFlag( GLString* str, const char* flag, int set, int value )
{
	str->append( "#define " );
	str->append( flag );
	str->append( " " );
	if ( set ) {
		char buf[] = "1\n";
		buf[0] = '0' + value;
		str->append( buf );
	}
	else {
		str->append( "0\n" );
	}
}


void ShaderManager::AppendConst( grinliz::GLString* str, const char* name, int value )
{
	char buf[100];
	SNPrintf( buf, 100, "#define %s %d\n", name, value );
	str->append( buf );
}


void ShaderManager::ActivateShader( int flags )
{
	Shader* shader = CreateProgram( flags );
	CHECK_GL_ERROR;
	glUseProgram( shader->prog );
	active = shader;
	CHECK_GL_ERROR;
}


int  ShaderManager::CalcMaxInstances( int flags, int* uniforms )
{
	int predictedUniform     = 0;
	int predictedUniformInst = 0;

	predictedUniform		=	4 +		// u_mvpMatrix
								1;		// u_colorMult
	predictedUniformInst	=	4 +		// u_mMatrix
								1;		// u_controlParamArr
	if ( flags & TEXTURE0 )				predictedUniform	 += 1;
	if ( flags & COLOR_PARAM )			predictedUniformInst += 1;
	if ( flags & BONE_FILTER )			predictedUniformInst += 1;
	if ( flags & TEXTURE0_XFORM )		predictedUniformInst += 1;
	if ( flags & TEXTURE0_CLIP )		predictedUniformInst += 1;
	if ( flags & TEXTURE0_COLORMAP )	predictedUniformInst += 4;
	if ( flags & BONE_FILTER )			predictedUniformInst += 1;
	if ( flags & BONE_XFORM )			predictedUniformInst += 4 * EL_MAX_BONES;
	if ( flags & LIGHTING )				predictedUniform += 7;	// normal matrix & lights

	// GL specifies at least 256 v4 uniforms.
	// Theoretically, some of those get used by the compiler for constants. (?) Although
	// I've never neen any indication of this in the query.
	// Say, 246 reliably remain?
	static const int NUM_UNIFORM = 246;
	int nInstance = (NUM_UNIFORM - predictedUniform) / predictedUniformInst;
	GLASSERT( nInstance > 0 );
	if ( nInstance < 1 ) nInstance = 1;

	if ( uniforms ) {
		*uniforms = predictedUniform + nInstance * predictedUniformInst;
	}
	return nInstance;
}


ShaderManager::Shader* ShaderManager::CreateProgram( int flags )
{
	static const int LEN = 1000;
	char buf[LEN];
	int outLen = 0;

	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].prog && shaderArr[i].flags == flags ) {
			return &shaderArr[i];
		}
		if ( shaderArr[i].prog == 0 ) {
			break;
		}
	}

	CStr<50> profileStr;
	profileStr.Format( "CreateProgram flags=%d", flags );
	QuickClockProfile profile( profileStr.c_str(), &this->totalCompileTime );

	Shader* shader = shaderArr.PushArr(1);
	shader->Init();
	shader->flags = flags;
	shader->prog = glCreateProgram();

	header = "#version 130\n";
	AppendFlag( &header, "TEXTURE0",			flags & TEXTURE0 );
	AppendFlag( &header, "TEXTURE0_XFORM",		flags & TEXTURE0_XFORM );
	AppendFlag( &header, "TEXTURE0_CLIP",		flags & TEXTURE0_CLIP );
	AppendFlag( &header, "TEXTURE0_COLORMAP",	flags & TEXTURE0_COLORMAP );
	AppendFlag( &header, "COLORS",				flags & COLORS );
	AppendFlag( &header, "PREMULT",				flags & PREMULT );
	AppendFlag( &header, "EMISSIVE",			flags & EMISSIVE );
	AppendFlag( &header, "EMISSIVE_EXCLUSIVE",	flags & EMISSIVE_EXCLUSIVE );
	AppendFlag( &header, "COLOR_PARAM",			flags & COLOR_PARAM );
	AppendFlag( &header, "BONE_FILTER",			flags & BONE_FILTER );
	AppendFlag( &header, "SATURATION",			flags & SATURATION );
	AppendFlag( &header, "BONE_XFORM",			flags & BONE_XFORM );
	AppendFlag( &header, "LIGHTING",			flags & LIGHTING);
	AppendFlag( &header, "INSTANCE",			flags & INSTANCE);

	AppendConst( &header, "EL_MAX_BONES",	 EL_MAX_BONES );

	int predicted = 0;
	shader->maxInstance = CalcMaxInstances( flags, &predicted );
	GLOUTPUT_REL(( "**** Shader %d *********\n", shader->flags ));
	GLOUTPUT_REL(( "Shader MAX instance = %d\n", shader->maxInstance ));
	GLOUTPUT_REL(( "Predicted uniform size(v4): %d\n\n", predicted ));

	AppendConst( &header, "MAX_INSTANCE", shader->maxInstance );

	// Is it in the cache?
#if 0
	// This is all working fine, but does nothing to speed up the Intel GPU,
	// which is the only GPU with a problem. #ifdeffing out until I have
	// the time to research what is going on.
	if ( GLEW_ARB_get_program_binary ) {
		CStr<200> path;
		path.Format( "./cache/shader_%s_%d.shader", hashStr.c_str(), flags );
		FILE* fp = FOpen( path.c_str(), "rb" );
		if ( fp ) {
			// In the cache!
			fseek( fp, 0, SEEK_END );
			long len = ftell( fp )-4;
			fseek( fp, 0, SEEK_SET );
			U8* data = new U8[len];
			U32 binaryFormat;
			fread( &binaryFormat, 4, 1, fp );
			fread( data, 1, len, fp );
			fclose( fp );

			glProgramBinary( shader->prog, binaryFormat, data, len );
			delete [] data;
			int success = 0;

			glGetProgramiv( shader->prog, GL_LINK_STATUS, &success);
			if ( success ) {
				GLOUTPUT_REL(( "Shader %d loaded from cache.\n", flags ));
				return shader;
			}
			GLOUTPUT_REL(( "Bad shader cache.\n" ));
		}
	}
#endif

	shader->vertexProg = glCreateShader( GL_VERTEX_SHADER );
	shader->fragmentProg = glCreateShader( GL_FRAGMENT_SHADER );


#if 0 
#ifdef DEBUG_OUTPUT
	GLOUTPUT(( "header\n%s\n", header.c_str() ));
#endif
#endif

	const char* vertexSrc[2]   = { header.c_str(), fixedpipeVert.c_str() };
	const char* fragmentSrc[2] = { header.c_str(), fixedpipeFrag.c_str() };

	glShaderSource( shader->vertexProg, 2, vertexSrc, 0 );

	glCompileShader( shader->vertexProg );
	glGetShaderInfoLog( shader->vertexProg, LEN, &outLen, buf );

	if ( outLen > 0 ) {
		GLOUTPUT_REL(( "Vertex %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

	glShaderSource( shader->fragmentProg, 2, fragmentSrc, 0 );
	glCompileShader( shader->fragmentProg );
	glGetShaderInfoLog( shader->fragmentProg, LEN, &outLen, buf );

	if ( outLen > 0 ) {
		GLOUTPUT_REL(( "Fragment %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

	glAttachShader( shader->prog, shader->vertexProg );
	glAttachShader( shader->prog, shader->fragmentProg );
	if ( GLEW_ARB_get_program_binary ) {
		glProgramParameteri( shader->prog, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	}
	glLinkProgram( shader->prog );
	glGetProgramInfoLog( shader->prog, LEN, &outLen, buf );

	if ( outLen > 0 ) {
		GLOUTPUT_REL(( "Link %d:\n%s\n", flags, buf ));
	}
	CHECK_GL_ERROR;

#if 0
	if ( GLEW_ARB_get_program_binary ) {
		_mkdir( "./cache" );
		CStr<200> path;
		path.Format( "./cache/shader_%s_%d.shader", hashStr.c_str(), flags );
		FILE* fp = FOpen( path.c_str(), "wb" );
		if ( fp ) {
			int len;
			U32 format=0;
			glGetProgramiv( shader->prog, GL_PROGRAM_BINARY_LENGTH, &len);
			U8* data = new U8[len];
			glGetProgramBinary(shader->prog, len, NULL, &format, data);

			fwrite( &format, 4, 1, fp );
			fwrite( data, len, 1, fp );
			fclose( fp );
			delete [] data;
		}
	}
#endif
	int nUniforms = 0;
	glGetProgramiv( shader->prog, GL_ACTIVE_UNIFORMS, &nUniforms );
	int uniformSize = 0;
	char name[100];
	GLint length = 0;
	GLsizei size = 0;
	GLenum type = 0;
	for( int i=0; i<nUniforms; ++i ) {
		glGetActiveUniform( shader->prog, i, 100, &length, &size, &type, name );
		const char* cType = "?";
		int v4Size = 1;
		switch ( type ) {
		case GL_FLOAT_MAT2:	cType = "mat2";	v4Size = 2;	break;
		case GL_FLOAT_MAT3:	cType = "mat3";	v4Size = 3;	break;
		case GL_FLOAT_MAT4:	cType = "mat4";	v4Size = 4;	break;
		case GL_SAMPLER_2D: cType = "sampler2D";		break;
		case GL_FLOAT:		cType = "float";			break;
		case GL_FLOAT_VEC2:	cType = "vec2";				break;
		case GL_FLOAT_VEC3:	cType = "vec3";				break;
		case GL_FLOAT_VEC4:	cType = "vec4";				break;
		default: 
			GLASSERT( 0 );
			break;
		};

		GLOUTPUT_REL(( "  %20s %10s %d x [%d] = %d\n", name, cType, v4Size, size, v4Size*size ));
		uniformSize += v4Size*size;
	}
	GLOUTPUT_REL(( "Queried uniform size(v4): %d\n", uniformSize ));
	GLASSERT( uniformSize <= predicted );

	GLOUTPUT_REL(( "Shader %d created. nUniforms=%d\n", flags, nUniforms ));
	GLOUTPUT_REL(( "%s", header.c_str() ));

	CHECK_GL_ERROR;

	return shader;
}


void ShaderManager::DeleteProgram( Shader* shader )
{
	CHECK_GL_ERROR;
	if ( shader->vertexProg )	// 0 in the case of loaded from disk cache
		glDetachShader( shader->prog, shader->vertexProg );
	CHECK_GL_ERROR;
	if ( shader->fragmentProg ) 
		glDetachShader( shader->prog, shader->fragmentProg );
	CHECK_GL_ERROR;
	if ( shader->vertexProg )	// 0 in the case of loaded from disk cache
		glDeleteShader( shader->vertexProg );
	if ( shader->fragmentProg )	// 0 in the case of loaded from disk cache
		glDeleteShader( shader->fragmentProg );
	glDeleteProgram( shader->prog );
	CHECK_GL_ERROR;
}
