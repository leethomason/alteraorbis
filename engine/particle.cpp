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


#include "../grinliz/glgeometry.h"
#include "../grinliz/glperformance.h"

#include "particle.h"
#include "surface.h"
#include "camera.h"
#include "texture.h"
#include "shadermanager.h"
#include "serialize.h"

#include <cfloat>

using namespace grinliz;

#if 0
/*static*/ void ParticleSystem::Create()
{
	GLASSERT( instance == 0 );
	instance = new ParticleSystem();
}


/*static*/ void ParticleSystem::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


ParticleSystem* ParticleSystem::instance = 0;
#endif

ParticleSystem::ParticleSystem() : texture( 0 ), time( 0 ), nParticles( 0 )
{
	for( int i=0; i<MAX_PARTICLES; ++i ) {
		U16* p = &indexBuffer[i*6];
		*(p+0) = i*4+0;
		*(p+1) = i*4+1;
		*(p+2) = i*4+2;
		*(p+3) = i*4+0;
		*(p+4) = i*4+2;
		*(p+5) = i*4+3;
	}
}


ParticleSystem::~ParticleSystem()
{
	Clear();
}


void ParticleSystem::Clear()
{
	nParticles = 0;
}


void ParticleSystem::Process( U32 delta, const grinliz::Vector3F eyeDir[] )
{
	// 8.4 ms (debug) in process. 0.8 in release (wow.)
	GRINLIZ_PERFTRACK

	time += delta;
	float deltaF = (float)delta * 0.001f;	// convert to seconds.
	const Vector3F& up = eyeDir[1];
	const Vector3F& right = eyeDir[2];

	ParticleData*   pd = particleData;
	ParticleStream* ps = vertexBuffer;
	ParticleData*	end = particleData + nParticles;
	ParticleData*   srcPD = particleData;
	ParticleStream* srcPS = vertexBuffer;

	while( srcPD < end ) {

		*pd = *srcPD;
		Vector4F color = srcPS->color + pd->colorVel * deltaF;

		if ( color.w <= 0 ) {
			nParticles--;
		}
		else { 
			if ( color.w > 1 ) {
				GLASSERT( pd->colorVel1.w < 0.f );
				color.w = 1.f;
				pd->colorVel = pd->colorVel1;
			}

			pd->pos += pd->velocity * deltaF;
			const Vector3F pos = pd->pos;
			const float size = pd->size;

			if ( size != FLT_MAX ) {
				ps[0].pos = pos - up*size - right*size;
				ps[1].pos = pos - up*size + right*size;
				ps[2].pos = pos + up*size + right*size;
				ps[3].pos = pos + up*size - right*size;
			}

			ps[0].color = color;
			ps[0].uv = srcPS[0].uv;

			ps[1].color = color;
			ps[1].uv = srcPS[1].uv;

			ps[2].color = color;
			ps[2].uv = srcPS[2].uv;

			ps[3].color = color;
			ps[3].uv = srcPS[3].uv;

			pd++;
			ps += 4;
		}
		srcPD++;
		srcPS += 4;
	}
}


void ParticleSystem::Update( U32 deltaTime, const grinliz::Vector3F eyeDir[] )
{
	//GRINLIZ_PERFTRACK
	Process( deltaTime, eyeDir );
}


const ParticleDef* ParticleSystem::GetPD( const char* name )
{
	for( int i=0; i<particleDefArr.Size(); ++i ) {
		if ( particleDefArr[i].name == name ) {
			return &particleDefArr[i];
		}
	}
	return 0;
}


void ParticleSystem::EmitPD(	const char* name,
								const grinliz::Vector3F& initPos,
								const grinliz::Vector3F& normal, 
								const grinliz::Vector3F eyeDir[],
								U32 deltaTime )
{
	const ParticleDef* pd = GetPD( name );
	if ( pd ) {
		EmitPD( *pd, initPos, normal, eyeDir, deltaTime );
		return;
	}
	GLASSERT( 0 );	// probably meant to actually find that particle.
}


void ParticleSystem::EmitPD(	const ParticleDef& def,
								const grinliz::Vector3F& initPos,
								const grinliz::Vector3F& normal, 
								const grinliz::Vector3F eyeDir[],
								U32 deltaTime )
{
	Vector3F velocity = normal * def.velocity;

	static const float TEXTURE_SIZE = ParticleDef::NUM_TEX;
	static const Vector2F uv[4] = {{0,0},{1.f/TEXTURE_SIZE,0},{1.f/TEXTURE_SIZE,1},{0,1}};
	const Vector3F& up = eyeDir[1];
	const Vector3F& right = eyeDir[2];
	int count = def.count;

	if ( def.time == ParticleDef::CONTINUOUS ) {
		float nParticles = (float)def.count * (float)deltaTime * 0.001f;
		count = (int) floorf( nParticles );
		nParticles -= floorf( nParticles );
		if ( random.Uniform() <= nParticles )
			++count;
	}

	for( int i=0; i<count; ++i ) {
		if ( nParticles < MAX_PARTICLES ) {
			ParticleData* pd = &particleData[nParticles];

			// For (hemi) sperical distributions, recompute a random direction.
			if ( def.config == ParticleSystem::PARTICLE_HEMISPHERE || def.config == ParticleSystem::PARTICLE_SPHERE ) {
				Vector3F n = { 0, 0, 0 };
				random.NormalVector3D( &n.x );

				if ( def.config == ParticleSystem::PARTICLE_HEMISPHERE && DotProduct( normal, n ) < 0.f ) {
					n = -n;
				}
				velocity = n * def.velocity;
			}

			// There is potentially both an increasing alpha
			// and decreasing alpha velocity.
			GLASSERT( def.colorVelocity0.w < 0 || def.colorVelocity1.w < 0 );
			pd->colorVel = def.colorVelocity0;
			pd->colorVel1 = def.colorVelocity1;

			Vector3F vFuzz;
			random.NormalVector3D( &vFuzz.x );
			pd->velocity = velocity + vFuzz*def.velocityFuzz;

			pd->size = def.size;
			
			Vector4F cFuzz = { 0, 0, 0, 0 };
			random.NormalVector3D( &cFuzz.x );
			Vector4F color = def.color + cFuzz*def.colorFuzz;

			Vector3F pFuzz;
			random.NormalVector3D( &pFuzz.x );
			Vector3F pos = initPos + pFuzz*def.posFuzz;
			pd->pos = pos;

			int texOffset = (def.texMax > def.texMin) ? random.Rand( def.texMax - def.texMin + 1 ) : 0;
			float uOffset = (float)(def.texMin + texOffset) / TEXTURE_SIZE;

			ParticleStream* ps = &vertexBuffer[nParticles*4];
			for( int k=0; k<4; ++k ) {
				ps[k].color = color;
				ps[k].uv = uv[k];
				ps[k].uv.x += uOffset;
			};
			const float size = def.size;

			ps[0].pos = pos - up*size - right*size;
			ps[1].pos = pos - up*size + right*size;
			ps[2].pos = pos + up*size + right*size;
			ps[3].pos = pos + up*size - right*size;

			++nParticles;
		}
	}
}



void ParticleSystem::Draw()
{
	//GRINLIZ_PERFTRACK

	if ( nParticles == 0 ) {
		return;
	}
	if ( !texture ) {
		texture = TextureManager::Instance()->GetTexture( "particle" );
	}
	
	GPUStream stream;
	stream.stride = sizeof( ParticleStream );
	stream.nPos = 3;
	stream.posOffset = ParticleStream::POS_OFFSET;
	stream.nTexture0 = 2;
	stream.texture0Offset = ParticleStream::TEXTURE_OFFSET;
	stream.nColor = 4;
	stream.colorOffset = ParticleStream::COLOR_OFFSET;

	ParticleShader shader;
	shader.SetTexture0( texture );

	shader.SetStream( stream, vertexBuffer, 6*nParticles, indexBuffer );
	shader.Draw(); 
}


void ParticleDef::Load( const tinyxml2::XMLElement* ele )
{
	name = ele->Attribute( "name" );
	
	time = ONCE;
	if ( ele->Attribute( "time", "continuous" ) ) {
		time = CONTINUOUS;
	}
	size = 1.0f;
	if ( ele->Attribute( "size" )) {
		ele->QueryFloatAttribute( "size", &size );
	}

	count = 1;
	ele->QueryIntAttribute( "count", &count );
	
	texMin = texMax = 0;
	ele->QueryIntAttribute( "texMin", &texMin );
	ele->QueryIntAttribute( "texMax", &texMax );

	config = ParticleSystem::PARTICLE_RAY;
	if ( ele->Attribute( "config", "sphere" ) ) config = ParticleSystem::PARTICLE_SPHERE;
	if ( ele->Attribute( "config", "hemi" ) ) config = ParticleSystem::PARTICLE_HEMISPHERE;
//	if ( ele->Attribute( "config", "world" ) ) config = ParticleSystem::PARTICLE_WORLD;

	posFuzz = 0;
	ele->QueryFloatAttribute( "posFuzz", &posFuzz );
	velocity = 1.0f;
	ele->QueryFloatAttribute( "velocity", &velocity );
	velocityFuzz = 0;
	ele->QueryFloatAttribute( "velocityFuzz", &velocityFuzz );

	color.Set( 1,1,1,1 );
	colorVelocity0.Set( 0, 0, 0, -0.5f );
	colorVelocity1.Set( 0, 0, 0, -0.5f );
	colorFuzz.Set( 0, 0, 0, 0 );

	const tinyxml2::XMLElement* child = 0;
	child = ele->FirstChildElement( "color" );
	if ( child ) {
		LoadColor( child, &color );
	}
	child = ele->FirstChildElement( "colorVelocity" );
	if ( child ) {
		LoadColor( child, &colorVelocity0 );
		child = child->NextSiblingElement( "colorVelocity" );
		if ( child ) {
			LoadColor( child, &colorVelocity1 );
		}
	}
	child = ele->FirstChildElement( "colorFuzz" );
	if ( child ) {
		LoadColor( child, &colorFuzz );
	}
}


void ParticleSystem::LoadParticleDefs( const char* filename )
{
	LoadParticles( &particleDefArr, filename );
}
