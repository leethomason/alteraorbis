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
#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "vertex.h"
#include "ufoutil.h"
#include "map.h"
#include "shadermanager.h"

class Texture;
struct ParticleDef;
class Camera;

// Here to fix rebuild hell.
void LoadParticles( grinliz::CDynArray< ParticleDef >* array, const char* path );

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
	int					alignment;
	grinliz::Vector2F	size;
	grinliz::Vector2F	sizeVel;
	grinliz::Vector4F	colorVel;		// units / second added to color. particle dead when a<=0
	grinliz::Vector4F	colorVel1;
	grinliz::Vector3F	velocity;		// units / second added to origin
	grinliz::Vector3F	pos;
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


// SHALLOW
struct ParticleDef
{
	grinliz::IString name;

	enum { ONCE, CONTINUOUS };
	int time;		// "once" "continuous"
	enum { SPREAD_POINT, SPREAD_SQUARE };
	int spread;
	enum { ALIGN_CAMERA, ALIGN_Y };
	int alignment;

	int count;
	enum { SPHERE, SMOKE_0, SMOKE_1, SQUARE, BOLT, NUM_TEX=8 };
	int config;		// "sphere" "hemi" "ray"
	int texMin, texMax;	// min and max texture index
	float posFuzz;
	float velocity;
	float velocityFuzz;

	grinliz::Vector2F size;		
	grinliz::Vector2F sizeVelocity;

	grinliz::Vector4F color;
	grinliz::Vector4F colorVelocity0;
	grinliz::Vector4F colorVelocity1;
	grinliz::Vector4F colorFuzz;

	ParticleDef() {
		time = ONCE;
		spread = SPREAD_POINT;
		alignment = ALIGN_CAMERA;
		count = 0;
		config = SPHERE;
		texMin = texMax = 0;
		posFuzz = velocity = velocityFuzz = 0;
		size.Zero();
		sizeVelocity.Zero();
		color.Zero();
		colorVelocity0.Zero();
		colorVelocity1.Zero();
		colorFuzz.Zero();
	}

	void Load( const tinyxml2::XMLElement* element );
};



/*	Class to render all sorts of particle effects.
*/
class ParticleSystem : public IDeviceLossHandler
{
public:
	ParticleSystem();
	virtual ~ParticleSystem();

	class IRegion {
	public:
		virtual grinliz::Vector3F CalcPoint( grinliz::Random* random ) = 0;
	};

	class RectangleRegion : public IRegion {
	public:
		RectangleRegion( const grinliz::Rectangle3F& r ) : rect( r ) {};
		virtual grinliz::Vector3F CalcPoint( grinliz::Random* random );
		const grinliz::Rectangle3F& rect;
	};

	class LineRegion : public IRegion {
	public:
		LineRegion( const grinliz::Vector3F& _p0, const grinliz::Vector3F& _p1 ) : p0(_p0), p1(_p1) {}
		virtual grinliz::Vector3F CalcPoint( grinliz::Random* random );
		const grinliz::Vector3F& p0;
		const grinliz::Vector3F& p1;
	};

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

	void EmitPD(	const ParticleDef& pd,
					IRegion* region,
					const grinliz::Vector3F& normal,
					U32 deltaTime );

	void EmitPD(	const ParticleDef& pd,
					const grinliz::Rectangle3F& pos,
					const grinliz::Vector3F& normal,
					U32 deltaTime )
	{
		RectangleRegion rr( pos );
		EmitPD( pd, &rr, normal, deltaTime );
	}

	void EmitPD(	const ParticleDef& pd,
					const grinliz::Vector3F& p0,
					const grinliz::Vector3F& p1,
					const grinliz::Vector3F& normal,
					U32 deltaTime )
	{
		LineRegion rr( p0, p1 );
		EmitPD( pd, &rr, normal, deltaTime );
	}


	void EmitPD(	const ParticleDef& pd,
					const grinliz::Vector3F& pos,
					const grinliz::Vector3F& normal,
					U32 deltaTime )
	{
		grinliz::Rectangle3F r3;
		r3.min = r3.max = pos;
		RectangleRegion rr( r3 );
		EmitPD( pd, &rr, normal, deltaTime );
	}

	void EmitPD(	const grinliz::IString& name,
					const grinliz::Vector3F& initPos,
					const grinliz::Vector3F& normal, 
					U32 deltaTime );

	const ParticleDef& GetPD( const char* name );
	const ParticleDef& GetPD( const grinliz::IString& name );

	void SetClip(const grinliz::Rectangle3F& clip) { this->clip = clip; }
	void Update( U32 deltaTime, Camera* camera );
	void Draw();
	void Clear();
	int NumParticles() const { return nParticles; }

	void LoadParticleDefs( const char* filename );
	virtual void DeviceLoss();

private:

	void Process( unsigned msec, Camera* camera );

	enum {
		MAX_PARTICLES = 2000	// don't want to re-allocate vertex buffers
	};

	grinliz::Random random;
	Texture* texture;
	U32 time;
	grinliz::Rectangle3F clip;

	GPUVertexBuffer* vbo;

	int nParticles;
	grinliz::CDynArray<ParticleDef> particleDefArr;
	ParticleData	particleData[MAX_PARTICLES];
	ParticleStream	vertexBuffer[MAX_PARTICLES*4];

};


#endif // UFOTACTICAL_PARTICLE_INCLUDED
