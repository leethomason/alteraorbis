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

//#include "shaders.inc"

#include "../grinliz/glperformance.h"
#include "../grinliz/glrandom.h"

#define DEBUG_OUTPUT

using namespace grinliz;

ShaderManager* ShaderManager::instance = 0;

static const char* gAttributeName[ShaderManager::MAX_ATTRIBUTE] =
{
	"a_uv0",
	"a_uv1",
	"a_pos",
	"a_normal",
	"a_color",
	"a_instanceID",
	"a_boneID",
};


static const char* gUniformName[ShaderManager::MAX_UNIFORM] = 
{
	"u_mvpMatrix",
	"u_mMatrix",
	"u_paramArr",

	"u_normalMatrix",
	"u_colorMult",
	"u_lightDir",
	"u_ambient",
	"u_diffuse",
	"u_radius",
	"u_param",
	"u_boneXForm",
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
	}
	return uniformLoc[uniform];
}


ShaderManager::ShaderManager() : active(0), totalCompileTime(0)
{
	LoadProgram( "fixedpipe.vert", &fixedpipeVert );
	LoadProgram( "fixedpipe.frag", &fixedpipeFrag );

	U32 hash0 = Random::Hash( fixedpipeVert.c_str(), fixedpipeVert.size() );
	U32 hash1 = Random::Hash( fixedpipeFrag.c_str(), fixedpipeFrag.size() );
	U32 hash = hash0 ^ hash1;

	hashStr.Format( "%x", hash );

	// FIXME make cache directory
}


ShaderManager::~ShaderManager()
{
	for( int i=0; i<shaderArr.Size(); ++i ) {
		if ( shaderArr[i].prog ) {
			DeleteProgram( &shaderArr[i] );
		}
	}
}


void ShaderManager::LoadProgram( const char* name, GLString* str ) 
{
	CStr<256> path;
	path.Format( "./res/%s", name );
	FILE* fp = fopen( path.c_str(), "r" );
	GLASSERT( fp );

	static const int SIZE = 100;
	char buf[SIZE];
	int count = 0;
	while( (count = fread( buf, 1, SIZE, fp )) > 0 ) {
		str->append( buf, count );
	}
	fclose( fp );
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
	int loc = active->GetAttributeLocation( id );
	GLASSERT( loc >= 0 );
#ifdef DEBUG
	const float* ft = (const float*)data;
	for( int i=0; i<activeStreams.Size(); ++i ) {
		GLASSERT( activeStreams[i] != loc );
	}
#endif
	glVertexAttribPointer( loc, size, type, 0, stride, data );
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
	glUniformMatrix4fv( loc, 1, false, value.x );
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


void ShaderManager::SetUniformArray( int id, int count, const grinliz::Matrix4* mat )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniformMatrix4fv( loc, count, false, mat->x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniformArray( int id, int count, const grinliz::Vector4F* v )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform4fv( loc, count, &v->x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetUniformArray( int id, int count, const grinliz::Vector3F* v )
{
	CHECK_GL_ERROR;
	int loc = active->GetUniformLocation( id );
	GLASSERT( loc >= 0 );
	glUniform3fv( loc, count, &v->x );
	CHECK_GL_ERROR;
}


void ShaderManager::SetTexture( int index, Texture* texture )
{
	char name[9] = "texture0";
	name[7] = '0' + index;
	int loc = glGetUniformLocation( active->prog, name );
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

	// Is it in the cache?
	if ( GLEW_ARB_get_program_binary ) {
		CStr<200> path;
		path.Format( "./cache/shader_%s_%d.shader", hashStr.c_str(), flags );
		FILE* fp = fopen( path.c_str(), "rb" );
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
				GLOUTPUT(( "Shader %d loaded from cache.\n", flags ));
				return shader;
			}
			GLASSERT( false );	// bad cache
		}
	}

	shader->vertexProg = glCreateShader( GL_VERTEX_SHADER );
	shader->fragmentProg = glCreateShader( GL_FRAGMENT_SHADER );

	header = "";
	AppendFlag( &header, "TEXTURE0",			flags & TEXTURE0 );
	if ( flags & TEXTURE0 ) {
		AppendFlag( &header, "TEXTURE0_ALPHA_ONLY",	flags & TEXTURE0_ALPHA_ONLY );
		AppendFlag( &header, "TEXTURE0_TRANSFORM",	flags & TEXTURE0_TRANSFORM );
	}
	AppendFlag( &header, "TEXTURE1",			flags & TEXTURE1 );
	if ( flags & TEXTURE1 ) {
		AppendFlag( &header, "TEXTURE1_ALPHA_ONLY",	flags & TEXTURE1_ALPHA_ONLY );
		AppendFlag( &header, "TEXTURE1_TRANSFORM",	flags & TEXTURE1_TRANSFORM );
	}
	AppendFlag( &header, "COLOR_PARAM",			flags & COLOR_PARAM );
	AppendFlag( &header, "COLORS",				flags & COLORS );
	//AppendFlag( &header, "COLOR_MULTIPLIER",	flags & COLOR_MULTIPLIER );
	AppendFlag( &header, "INSTANCE",			flags & INSTANCE );
	AppendFlag( &header, "PREMULT",				flags & PREMULT );
	AppendFlag( &header, "EMISSIVE",			flags & EMISSIVE );
	AppendFlag( &header, "EMISSIVE_EXCLUSIVE",	flags & EMISSIVE_EXCLUSIVE );
	AppendFlag( &header, "BONE_FILTER",			flags & BONE_FILTER );
	AppendFlag( &header, "BONES",				shader->BonesNeeded() );
	AppendFlag( &header, "PARAM",				shader->ParamNeeded() );
	AppendFlag( &header, "PROCEDURAL",			flags & PROCEDURAL );

	if ( flags & LIGHTING_DIFFUSE )
		AppendFlag( &header, "LIGHTING_DIFFUSE", 1, 1 );
	else if ( flags & LIGHTING_HEMI )
		AppendFlag( &header, "LIGHTING_DIFFUSE", 1, 2 );
	else 
		AppendFlag( &header, "LIGHTING_DIFFUSE", 0, 0 );

	AppendConst( &header, "EL_MAX_INSTANCE", EL_MAX_INSTANCE );
	AppendConst( &header, "EL_MAX_BONES",	 EL_MAX_BONES );

#ifdef DEBUG_OUTPUT
	GLOUTPUT(( "header\n%s\n", header.c_str() ));
#endif

	const char* vertexSrc[2]   = { header.c_str(), fixedpipeVert.c_str() };
	const char* fragmentSrc[2] = { header.c_str(), fixedpipeFrag.c_str() };

	glShaderSource( shader->vertexProg, 2, vertexSrc, 0 );

	glCompileShader( shader->vertexProg );
	glGetShaderInfoLog( shader->vertexProg, LEN, &outLen, buf );
#ifdef DEBUG_OUTPUT
	if ( outLen > 0 ) {
		GLOUTPUT(( "Vertex %d:\n%s\n", flags, buf ));
	}
#endif
	CHECK_GL_ERROR;

	glShaderSource( shader->fragmentProg, 2, fragmentSrc, 0 );
	glCompileShader( shader->fragmentProg );
	glGetShaderInfoLog( shader->fragmentProg, LEN, &outLen, buf );
#ifdef DEBUG_OUTPUT
	if ( outLen > 0 ) {
		GLOUTPUT(( "Fragment %d:\n%s\n", flags, buf ));
	}
#endif
	CHECK_GL_ERROR;

	glAttachShader( shader->prog, shader->vertexProg );
	glAttachShader( shader->prog, shader->fragmentProg );
	if ( GLEW_ARB_get_program_binary ) {
		glProgramParameteri( shader->prog, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	}
	glLinkProgram( shader->prog );
	glGetProgramInfoLog( shader->prog, LEN, &outLen, buf );
#ifdef DEBUG_OUTPUT
	if ( outLen > 0 ) {
		GLOUTPUT(( "Link %d:\n%s\n", flags, buf ));
	}
#endif
	CHECK_GL_ERROR;

	if ( GLEW_ARB_get_program_binary ) {
		CStr<200> path;
		path.Format( "./cache/shader_%s_%d.shader", hashStr.c_str(), flags );
		FILE* fp = fopen( path.c_str(), "wb" );
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
	int nUniforms, maxUniforms;
	glGetProgramiv( shader->prog, GL_ACTIVE_UNIFORMS, &nUniforms );
	glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxUniforms );
	GLOUTPUT(( "Shader %d created. Uniforms=%d / %d\n", flags, nUniforms, maxUniforms ));

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

	