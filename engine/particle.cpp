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
#include "../Shiny/include/Shiny.h"

#include "particle.h"
#include "surface.h"
#include "camera.h"
#include "texture.h"
#include "shadermanager.h"
#include "serialize.h"

#include <cfloat>

using namespace grinliz;

ParticleSystem::ParticleSystem() : texture( 0 ), time( 0 ), nParticles( 0 )
{
	ShaderManager::Instance()->AddDeviceLossHandler( this );
	vbo = 0;
	clip.Zero();
}


ParticleSystem::~ParticleSystem()
{
	Clear();
	ShaderManager::Instance()->RemoveDeviceLossHandler( this );
}


void ParticleSystem::Clear()
{
	nParticles = 0;
	delete vbo;
	vbo = 0;
}


void ParticleSystem::DeviceLoss()
{
	Clear();
}


void ParticleSystem::Process( U32 delta, Camera* camera )
{
	// 8.4 ms (debug) in process. 0.8 in release (wow.)
	PROFILE_FUNC();

	const Vector3F* eyeDir = camera->EyeDir3();
	const Vector3F origin = camera->PosWC();
	float RAD2 = EL_FAR*EL_FAR;
	float alphaCutoff = 0.0f;

	if (nParticles > MAX_PARTICLES * 7 / 8) {
		alphaCutoff = 0.10f;
		RAD2 /= 2;
	}
	else if ( nParticles > MAX_PARTICLES*3/4 ) {
		alphaCutoff = 0.08f;
		RAD2 /= 2;
	}
	else if ( nParticles > MAX_PARTICLES/2 ) {
		alphaCutoff = 0.05f;
	}

	time += delta;
	float deltaF = (float)delta * 0.001f;	// convert to seconds.
	const Vector3F& up = eyeDir[1];
	const Vector3F& right = eyeDir[2];
	const Vector3F xaxis = { 1, 0, 0 };
	const Vector3F zaxis = { 0, 0, 1 };

	ParticleData*   pd = particleData;
	ParticleStream* ps = vertexBuffer;
	ParticleData*	end = particleData + nParticles;
	ParticleData*   srcPD = particleData;
	ParticleStream* srcPS = vertexBuffer;

	while( srcPD < end ) {

		*pd = *srcPD;
		Vector4F color = srcPS->color + pd->colorVel * deltaF;

		if (    (pd->colorVel.w < 0 && color.w <= alphaCutoff)
			 || ((pd->pos - origin).LengthSquared() > RAD2)
			 || (pd->velocity.y < 0 && pd->pos.y < 0 )
			 || (pd->velocity.y > 0 && pd->pos.y > EL_CAMERA_MAX) )
		{
			// don't advance pd & ps. This will
			// be overwritten
		}
		else { 
			if ( color.w > 1 ) {
				GLASSERT( pd->colorVel1.w < 0.f );
				color.w = 1.f;
				pd->colorVel = pd->colorVel1;
				GLASSERT( pd->colorVel.w < 0 );
			}

			pd->pos  += pd->velocity * deltaF;
			pd->size += pd->sizeVel * deltaF;
			const Vector3F pos  = pd->pos;
			const Vector2F size = pd->size;

			if ( pd->alignment == ParticleDef::ALIGN_CAMERA ) {
				ps[0].pos = pos - up*size.y - right*size.x;
				ps[1].pos = pos - up*size.y + right*size.x;
				ps[2].pos = pos + up*size.y + right*size.x;
				ps[3].pos = pos + up*size .y- right*size.x;
			}
			else {
				ps[0].pos = pos - xaxis*size.y - zaxis*size.x;
				ps[1].pos = pos - xaxis*size.y + zaxis*size.x;
				ps[2].pos = pos + xaxis*size.y + zaxis*size.x;
				ps[3].pos = pos + xaxis*size .y- zaxis*size.x;
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
	nParticles = pd - particleData;
}


void ParticleSystem::Update( U32 deltaTime, Camera* camera )
{
	//GRINLIZ_PERFTRACK
	Process( deltaTime, camera );
}


const ParticleDef& ParticleSystem::GetPD( const char* name )
{
	for( int i=0; i<particleDefArr.Size(); ++i ) {
		if ( particleDefArr[i].name == name ) {
			return particleDefArr[i];
		}
	}
	GLASSERT( 0 );	// not intended
	return particleDefArr[0];
}


const ParticleDef& ParticleSystem::GetPD( const IString& name )
{
	for( int i=0; i<particleDefArr.Size(); ++i ) {
		if ( particleDefArr[i].name == name ) {
			return particleDefArr[i];
		}
	}
	GLASSERT( 0 );	// not intended
	return particleDefArr[0];
}


void ParticleSystem::EmitPD(	const IString& name,
								const grinliz::Vector3F& initPos,
								const grinliz::Vector3F& normal, 
								U32 deltaTime )
{
	const ParticleDef& pd = GetPD( name );
	Rectangle3F r;
	r.min = initPos;
	r.max = initPos;
	EmitPD( pd, r, normal, deltaTime );
}


grinliz::Vector3F ParticleSystem::RectangleRegion::CalcPoint( grinliz::Random* random )
{
	Vector3F v = {
		rect.min.x + random->Uniform() * rect.SizeX(),
		rect.min.y + random->Uniform() * rect.SizeY(),
		rect.min.z + random->Uniform() * rect.SizeZ()
	};
	return v;
}


grinliz::Vector3F ParticleSystem::LineRegion::CalcPoint( grinliz::Random* random )
{
	Vector3F v = p0 + random->Uniform() * (p1-p0);
	return v;
}


void ParticleSystem::EmitPD(	const ParticleDef& def,
								ParticleSystem::IRegion* region,
								const grinliz::Vector3F& normal, 
								U32 deltaTime )
{
	Vector3F velocity = normal * def.velocity;

	static const float TEXTURE_SIZE = ParticleDef::NUM_TEX;
	static const Vector2F uv[4] = {{0,0},{1.f/TEXTURE_SIZE,0},{1.f/TEXTURE_SIZE,1},{0,1}};
	int count = def.count;

	if ( def.time == ParticleDef::CONTINUOUS ) {
		float nPart = (float)def.count * (float)deltaTime * 0.001f;
		count = (int) floorf( nPart );
		nPart -= count;
		if ( random.Uniform() <= nPart )
			++count;
	}

	for( int i=0; i<count; ++i ) {
		if ( nParticles < MAX_PARTICLES ) {
			ParticleData* pd = &particleData[nParticles];

			Vector3F pos = region->CalcPoint( &random );

			// Don't even emit if we'll never see it.
			if (clip.Volume() && !clip.Contains(pos)) {
				continue;
			}

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
			GLASSERT( def.colorVelocity0.w < -0.001f || def.colorVelocity1.w < -0.001f );
			pd->colorVel  = def.colorVelocity0;
			pd->colorVel1 = def.colorVelocity1;

			Vector3F vFuzz;
			random.NormalVector3D( &vFuzz.x );
			pd->velocity = velocity + vFuzz*def.velocityFuzz;

			pd->alignment	= def.alignment;
			pd->size		= def.size;
			pd->sizeVel		= def.sizeVelocity;
			
			Vector4F cFuzz = { 0, 0, 0, 0 };
			random.NormalVector3D( &cFuzz.x );
			Vector4F color = def.color + cFuzz*def.colorFuzz;

			Vector3F pFuzz;
			random.NormalVector3D( &pFuzz.x );
			pos = pos + pFuzz*def.posFuzz;
			pd->pos = pos;

			int texOffset = (def.texMax > def.texMin) ? random.Rand( def.texMax - def.texMin + 1 ) : 0;
			float uOffset = (float)(def.texMin + texOffset) / TEXTURE_SIZE;

			ParticleStream* ps = &vertexBuffer[nParticles*4];
			for( int k=0; k<4; ++k ) {
				ps[k].color = color;
				ps[k].uv = uv[k];
				ps[k].uv.x += uOffset;
			};

			ps[0].pos = pos;
			ps[1].pos = pos;
			ps[2].pos = pos;
			ps[3].pos = pos;

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
	if ( !vbo ) {
		vbo = new GPUVertexBuffer( 0, sizeof( ParticleStream )*MAX_PARTICLES*4 );
	}
	vbo->Upload( vertexBuffer, sizeof(ParticleStream)*nParticles*4, 0 );
	
	GPUStream stream;
	stream.stride = sizeof( ParticleStream );
	stream.nPos = 3;
	stream.posOffset = ParticleStream::POS_OFFSET;
	stream.texture0Offset = ParticleStream::TEXTURE_OFFSET;
	stream.nColor = 4;
	stream.colorOffset = ParticleStream::COLOR_OFFSET;

	GPUStreamData data;
	data.vertexBuffer = vbo->ID();
	data.texture0 = texture;

	ParticleShader shader;
	GPUDevice::Instance()->DrawQuads( shader, stream, data, nParticles );
}


void ParticleDef::Load( const tinyxml2::XMLElement* ele )
{
	name = StringPool::Intern(ele->Attribute( "name" ));
	
	time = ONCE;
	if ( ele->Attribute( "time", "continuous" )) {
		time = CONTINUOUS;
	}
	spread = SPREAD_POINT;
	if ( ele->Attribute( "spread", "square" )) {
		spread = SPREAD_SQUARE;
	}
	alignment = ALIGN_CAMERA;
	if ( ele->Attribute( "alignment", "y" )) {
		alignment = ALIGN_Y;
	}

	size.Set( 1, 1 );
	if ( ele->Attribute( "size" )) {
		ele->QueryFloatAttribute( "size", &size.x );
		size.y = size.x;
	}
	if ( ele->Attribute( "sizeX" )) {
		ele->QueryAttribute( "sizeX", &size.x );
	}
	if ( ele->Attribute( "sizeY" )) {
		ele->QueryAttribute( "sizeY", &size.y );
	}

	sizeVelocity.Set( 0, 0 );
	if ( ele->Attribute( "sizeVelocity" )) {
		ele->QueryFloatAttribute( "sizeVelocity", &sizeVelocity.x );
		sizeVelocity.y = sizeVelocity.x;
	}
	if ( ele->Attribute( "sizeVelocityX" )) {
		ele->QueryAttribute( "sizeVelocityX", &sizeVelocity.x );
	}
	if ( ele->Attribute( "sizeVelocityY" )) {
		ele->QueryAttribute( "sizeVelocityY", &sizeVelocity.y );
	}

	count = 1;
	ele->QueryIntAttribute( "count", &count );
	
	texMin = texMax = 0;
	ele->QueryIntAttribute( "texMin", &texMin );
	ele->QueryIntAttribute( "texMax", &texMax );

	config = ParticleSystem::PARTICLE_RAY;
	if ( ele->Attribute( "config", "sphere" ) ) config = ParticleSystem::PARTICLE_SPHERE;
	if ( ele->Attribute( "config", "hemi" ) ) config = ParticleSystem::PARTICLE_HEMISPHERE;

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
