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

#include "particle.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glperformance.h"
#include "surface.h"
#include "camera.h"
#include "texture.h"

using namespace grinliz;


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
	time += delta;
	float deltaF = (float)delta * 0.001f;	// convert to seconds.
	const Vector3F& up = eyeDir[1];
	const Vector3F& right = eyeDir[2];

	int i=0;
	while( i<nParticles ) {
		ParticleData* pd = &particleData[i];
		ParticleStream* ps = &vertexBuffer[i*4];
		
		Vector4F color = ps->color + pd->colorVel * deltaF;

		if ( color.w > 1 ) {
			GLASSERT( pd->colorVel1.w < 0.f );
			color.w = 1.f;
			pd->colorVel = pd->colorVel1;
		}
		if ( color.w <= 0 ) {
			ParticleStream* end = vertexBuffer + nParticles*4;
			Swap( ps+0, end-4 );
			Swap( ps+1, end-3 );
			Swap( ps+2, end-2 );
			Swap( ps+3, end-1 );
			Swap( pd, &particleData[nParticles-1] );
			nParticles--;
		}
		else {
			pd->pos += pd->velocity * deltaF;
			const Vector3F pos = pd->pos;
			const float size = pd->size;

			ps[0].color = color;
			ps[1].color = color;
			ps[2].color = color;
			ps[3].color = color;

			ps[0].pos = pos - up*size - right*size;
			ps[1].pos = pos - up*size + right*size;
			ps[2].pos = pos + up*size + right*size;
			ps[3].pos = pos + up*size - right*size;

			i++;
		}
	}
}


void ParticleSystem::Update( U32 deltaTime, const grinliz::Vector3F eyeDir[] )
{
	GRINLIZ_PERFTRACK
	Process( deltaTime, eyeDir );
}


/*
void ParticleSystem::EmitBeam( const grinliz::Vector3F& p0, const grinliz::Vector3F& p1, const grinliz::Color4F& color )
{
	Beam* beam = beamBuffer.Push();
	beam->pos0 = p0;
	beam->pos1 = p1;
	beam->color = color;
}
*/


void ParticleSystem::EmitPD(	const ParticleDef& def,
								const grinliz::Vector3F& initPos,
								const grinliz::Vector3F& normal, 
								const grinliz::Vector3F eyeDir[],
								U32 deltaTime )
{
	Vector3F velocity = normal * def.velocity;

	static const Vector2F uv[4] = {{0,0},{0.25,0},{0.25,1},{0,1}};
	static const float TEXTURE_SIZE = 4;
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

			int texOffset = (def.texMax > def.texMin) ? random.Rand( def.texMax - def.texMin ) : 0;
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
	GRINLIZ_PERFTRACK

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


/*
void ParticleSystem::DrawBeamParticles( const Vector3F* eyeDir )
{
	if ( beamBuffer.Empty() ) {
		return;
	}

	//const static float cornerX[] = { -1, 1, 1, -1 };
	//const static float cornerY[] = { -1, -1, 1, 1 };

	// fixme: hardcoded texture coordinates
	static const Vector2F tex[4] = {
		{ 0.50f, 0.0f },
		{ 0.75f, 0.0f },
		{ 0.75f, 0.25f },
		{ 0.50f, 0.25f }
	};

	int nIndex = 0;
	int nVertex = 0;

	vertexBuffer.Clear();
	indexBuffer.Clear();

	U16* iBuf = indexBuffer.PushArr( 6*quadBuffer.Size() );
	QuadVertex* vBuf = vertexBuffer.PushArr( 4*quadBuffer.Size() );

	for( int i=0; i<beamBuffer.Size(); ++i ) 
	{
		const Beam& b = beamBuffer[i];

		// Set up the particle that everything else is derived from:
		iBuf[nIndex++] = nVertex+0;
		iBuf[nIndex++] = nVertex+1;
		iBuf[nIndex++] = nVertex+2;

		iBuf[nIndex++] = nVertex+0;
		iBuf[nIndex++] = nVertex+2;
		iBuf[nIndex++] = nVertex+3;

		QuadVertex* pV = &vBuf[nVertex];

		const float hw = 0.1f;	//quadBuffer[i].halfWidth;
		Vector3F n;
		CrossProduct( eyeDir[Camera::NORMAL], b.pos1-b.pos0, &n );
		if ( n.LengthSquared() > 0.0001f )
			n.Normalize();
		else	
			n = eyeDir[Camera::RIGHT];

		pV[0].pos = b.pos0 - hw*n;
		pV[1].pos = b.pos0 + hw*n;
		pV[2].pos = b.pos1 + hw*n;
		pV[3].pos = b.pos1 - hw*n;

		for( int j=0; j<4; ++j ) {
			pV->tex = tex[j];
			pV->color = b.color;
			++pV;
		}
		nVertex += 4;
	}

	if ( nIndex ) {
		QuadParticleShader shader;
		shader.SetTexture0( quadTexture );

		GPUStream stream;
		stream.stride = sizeof( vertexBuffer[0] );
		stream.nPos = 3;
		stream.posOffset = 0;
		stream.nTexture0 = 2;
		stream.texture0Offset = 12;
		stream.nColor = 4;
		stream.colorOffset = 20;

		shader.SetStream( stream, vBuf, nIndex, iBuf );
		shader.Draw();
	}
}
*/


void ParticleDef::Load( const tinyxml2::XMLElement* ele )
{
	name = ele->Attribute( "name" );
	
	time = ONCE;
	if ( ele->Attribute( "time", "continuous" ) ) {
		time = CONTINUOUS;
	}
	size = 1.0f;
	ele->QueryFloatAttribute( "size", &size );
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
