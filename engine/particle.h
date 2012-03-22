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

#ifndef UFOTACTICAL_PARTICLE_INCLUDED
#define UFOTACTICAL_PARTICLE_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glbitarray.h"
#include "vertex.h"
#include "ufoutil.h"
#include "map.h"

class Texture;
struct ParticleDef;


// Full GPU system. Cool. Probably fast. But shader hell.
/*
struct Particle
{
	float				time;			// start time, in seconds
	grinliz::Color4F	color;			// start color
	grinliz::Color4F	colorVel;		// units / second added to color. particle dead when a<=0
	grinliz::Vector3F	origin;			// where this particle started in WC		
	grinliz::Vector2F	uv;				// texture coordinates of this vertex
	grinliz::Vector2F	offset;			// offset from the origin (-0.5,-0.5) - (0.5,0.5)
	grinliz::Vector3F	velocity;		// units / second added to origin
	float				size;			// size of the particle
};
*/


// Mixed system. *much* simpler.
// CPU part.
struct ParticleData
{
	grinliz::Vector4F	colorVel;		// units / second added to color. particle dead when a<=0
	grinliz::Vector4F	colorVel1;
	grinliz::Vector3F	velocity;		// units / second added to origin
	float				size;			// size of the particle
	grinliz::Vector3F	pos;
	// do we need this on the shader? Easy to do, but bandwidth vs. cpu clock tradeoff
	//grinliz::Vector2F	offset;			// offset from pos; size*0.5,0.5 offset
};


// GPU part.
struct ParticleStream
{
	enum {
		COLOR_OFFSET	= 0,
		POS_OFFSET		= 16,
		TEXTURE_OFFSET	= 28
	};

	grinliz::Vector4F	color;			// current color
	grinliz::Vector3F	pos;			// position in WC	
	grinliz::Vector2F	uv;				// texture coordinates of this vertex
};


/*	Class to render all sorts of particle effects.
*/
class ParticleSystem
{
public:
	static ParticleSystem* Instance()	{ GLASSERT( instance ); return instance; }

	static void Create();
	static void Destroy();

	// Texture particles. Location in texture follows.
	enum {
		BASIC			= 0,
		SMOKE_0			= 1,
		SMOKE_1			= 2,
	};

	enum {
		PARTICLE_RAY,
		PARTICLE_HEMISPHERE,
		PARTICLE_SPHERE,
	};

	void EmitPD( const ParticleDef& pd,
				 const grinliz::Vector3F& pos,
				 const grinliz::Vector3F& normal,
				 const grinliz::Vector3F eyeDir[],
				 U32 deltaTime );					// needed for pd.config == continuous

	void Update( U32 deltaTime, const grinliz::Vector3F eyeDir[] );
	void Draw();
	void Clear();
	int NumParticles() const { return nParticles; }

private:
	ParticleSystem();
	~ParticleSystem();

	static ParticleSystem* instance;

	void Process( unsigned msec, const grinliz::Vector3F eyeDir[] );

	enum {
		MAX_PARTICLES = 2000	// don't want to re-allocate vertex buffers
	};

	grinliz::Random random;
	Texture* texture;
	U32 time;
	
	int nParticles;
	ParticleData	particleData[MAX_PARTICLES];
	ParticleStream	vertexBuffer[MAX_PARTICLES*4];
	U16				indexBuffer[MAX_PARTICLES*6];
};


// SHALLOW
struct ParticleDef
{
	grinliz::CStr<16> name;

	enum { ONCE, CONTINUOUS };
	int time;		// "once" "continuous"

	float size;
	int count;
	int config;		// "sphere" "hemi" "ray"
	int texMin, texMax;	// min and max texture index
	float posFuzz;
	float velocity;
	float velocityFuzz;

	// fixme: add vbos
	grinliz::Vector4F color;
	grinliz::Vector4F colorVelocity0;
	grinliz::Vector4F colorVelocity1;
	grinliz::Vector4F colorFuzz;

	void Load( const tinyxml2::XMLElement* element );
};

#endif // UFOTACTICAL_PARTICLE_INCLUDED
