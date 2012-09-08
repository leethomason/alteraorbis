#include "bolt.h"
#include "engine.h"
#include "enginelimits.h"
#include "texture.h"
#include "gpustatemanager.h"

using namespace grinliz;

void Bolt::TickAll( grinliz::CDynArray<Bolt>* bolts, U32 delta, Engine* engine )
{
	//	Move
	//	Check for done
	//	Check for impact.
	//		Send a message to impacted object, telling it to absorb damage
	//		Terminate bolt motion at that location
	//		Particle effect
	//	Move bolt forward

	static const float SPEED = 5.0f;

	GLASSERT( engine->GetMap() );
	Map* map = engine->GetMap();

	Rectangle3F bounds;
	bounds.Set( 0, 0, 0, (float)(map->Width()), (float)(map->Width())*0.25f, (float)(map->Height()) ); 
	float timeSlice = (float)delta / 1000.0f;

	ParticleSystem* ps = engine->particleSystem;

	int i=0;
	while ( i < bolts->Size() ) {
		Bolt& b = (*bolts)[i];

		Vector3F travel = b.dir * SPEED * timeSlice;
		if ( !b.impact ) {
			// Check if we hit something in the world.
			// FIXME: add ignore list of the shooter, or move head away from model?
			Vector3F at;

			if ( (b.head + travel).y <= 0 ) {
				b.impact = true;
					
				// we hit the ground.
				if ( b.head.y > 0 ) {
					at = Lerp( b.head, b.head+travel, (b.head.y) / (travel.y));
				}
				else {
					at = b.head;	// not really sure how this happened.
				}
			}

			if ( !b.impact ) {
				Model* m = engine->IntersectModel( b.head, b.dir, SPEED*timeSlice, TEST_TRI, 0, 0, 0, &at );
				if ( m ) {
					b.impact = true;
					b.head = at;
				}
			}

			if ( b.impact ) {
				ParticleDef def = *ps->GetPD( "boltImpact" );
				def.color = b.color;
				ps->EmitPD( def, at, b.dir, engine->camera.EyeDir3(), delta );
			}
		}

		if ( !b.impact ) {
			b.head += travel;
			b.len += SPEED*timeSlice;
			if ( b.len > 2.0f )
				b.len = 2.0f;
		}
		else {
			b.len -= SPEED*timeSlice;
		}
		
		Vector3F tail = b.head - b.len*b.dir;
		
		if ( !bounds.Contains( b.head ) && !bounds.Contains( tail ) ) {
			bolts->SwapRemove( i );
		}
		else if ( b.len <= 0 ) {
			bolts->SwapRemove( i );
		}
		else {
			++i;
		}
	}
}


BoltRenderer::BoltRenderer()
{
	nBolts = 0;
	float u0 = (float)ParticleDef::BOLT / (float)ParticleDef::NUM_TEX;
	float u1 = (float)(ParticleDef::BOLT+1) / (float)ParticleDef::NUM_TEX;

	for( int i=0; i<MAX_BOLTS; ++i ) {
		index[i*6+0] = i*4+0;
		index[i*6+1] = i*4+1;
		index[i*6+2] = i*4+2;
		index[i*6+3] = i*4+0;
		index[i*6+4] = i*4+2;
		index[i*6+5] = i*4+3;
	}
	for( int i=0; i<MAX_BOLTS; ++i ) {
		vertex[i*4+0].tex.Set( u0, 0 );
		vertex[i*4+1].tex.Set( u1, 0 );
		vertex[i*4+2].tex.Set( u1, 1 );
		vertex[i*4+3].tex.Set( u0, 1 );

		for( int k=0; k<4; ++k ) {
			vertex[i*4+k].color.Set( 1,1,1,1 );
		}
	}
}


void BoltRenderer::DrawAll( const Bolt* bolts, int nBolts, Engine* engine )
{
	// FIXME: crude culling to the view frustum
	// FIXME: move more to the shader?

	Vector3F eyeNormal = engine->camera.EyeDir3()[Camera::NORMAL];
	static const float hw = 0.1f;

	nBolts = Min( nBolts, (int)MAX_BOLTS );
	for( int i=0; i<nBolts; ++i ) {

		Vector3F n;
		Vector3F tail = bolts[i].head - bolts[i].len*bolts[i].dir;
		CrossProduct( eyeNormal, bolts[i].head - tail, &n );
		n.SafeNormalize( 1, 0, 0 );

		vertex[i*4+0].pos = tail - hw*n;
		vertex[i*4+1].pos = tail + hw*n;
		vertex[i*4+2].pos = bolts[i].head + hw*n;
		vertex[i*4+3].pos = bolts[i].head - hw*n;

		Vector4F color = bolts[i].color;
		vertex[i*4+0].color = color;
		vertex[i*4+1].color = color;
		vertex[i*4+2].color = color;
		vertex[i*4+3].color = color;
	}

	Texture* texture = TextureManager::Instance()->GetTexture( "particle" );

	if ( nBolts ) {
		ParticleShader shader;
		shader.SetTexture0( texture );

		GPUStream stream;
		stream.stride			= sizeof( vertex[0] );
		stream.nPos				= 3;
		stream.posOffset		= PTCVertex::POS_OFFSET;
		stream.nTexture0		= 2;
		stream.texture0Offset	= PTCVertex::TEXTURE_OFFSET;
		stream.nColor			= 4;
		stream.colorOffset		= PTCVertex::COLOR_OFFSET;

		shader.SetStream( stream, vertex, nBolts*6, index );
		shader.Draw();
	}
}

