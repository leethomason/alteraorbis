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

#include "bolt.h"
#include "engine.h"
#include "enginelimits.h"
#include "texture.h"
#include "gpustatemanager.h"

using namespace grinliz;

void Bolt::TickAll( grinliz::CDynArray<Bolt>* bolts, U32 delta, Engine* engine, IBoltImpactHandler* handler )
{
	GLASSERT( engine->GetMap() );
	Map* map = engine->GetMap();

	Rectangle3F bounds;
	bounds.Set( 0, 0, 0, (float)(map->Width()), (float)(map->Width())*0.25f, (float)(map->Height()) ); 
	float timeSlice = (float)delta / 1000.0f;

	int i=0;	
	ParticleSystem* ps = engine->particleSystem;

	while ( i < bolts->Size() ) {
		Bolt& b = (*bolts)[i];
 
		float distance = b.speed * timeSlice;
		Vector3F travel = b.dir * distance;
		Vector3F normal = { 0, 1, 0 };

		if ( !b.impact ) {
			// Check if we hit something in the world.
			// FIXME: add ignore list of the shooter, or move head away from model?
			Vector3F at;

			// Check ground hit.
			if ( (b.head + travel).y <= 0 ) {
				b.impact = true;
					
				if ( b.head.y > 0 ) {
					at = Lerp( b.head, b.head+travel, (b.head.y) / (travel.y));
				}
				else {
					at = b.head;	// not really sure how this happened.
				}
			}

			// Check model hit.
			Model* m = 0;
			if ( !b.impact ) {
				m = engine->IntersectModel( b.head, b.dir, distance, TEST_TRI, 0, 0, 0, &at );
				if ( m ) {
					b.impact = true;
					normal = b.dir;
				}
			}

			if ( b.impact ) {
				if ( handler ) {
					handler->HandleBolt( b, m, at );
				}
				b.head = at;

				ParticleDef def = *ps->GetPD( "boltImpact" );
				def.color = b.color;
				ps->EmitPD( def, at, normal, engine->camera.EyeDir3(), delta );

				if ( b.explosive ) {
					def = *ps->GetPD( "explosion" );
					def.color = b.color;
					ps->EmitPD( def, at, normal, engine->camera.EyeDir3(), delta );
				}
			}
		}

		if ( !b.impact ) {
			b.head += travel;
			b.len += distance;
			if ( b.len > 2.0f )
				b.len = 2.0f;
		}
		else {
			b.len -= distance;
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

	if ( nBolts == 0 )
		return;

	Vector3F eyeNormal = engine->camera.EyeDir3()[Camera::NORMAL];
	static const float HALF_WIDTH = 0.07f;

	nBolts = Min( nBolts, (int)MAX_BOLTS );
	ParticleSystem* ps = engine->particleSystem;
	ParticleDef def = *ps->GetPD( "smoketrail" );

	int count = 0;
	for( int i=0; i<nBolts; ++i ) {
		if ( bolts[i].particle ) {
			def.color = bolts[i].color;
			ps->EmitPD( def, bolts[i].head, V3F_UP, engine->camera.EyeDir3(), 0 );
		}
		else {
			++count;

			Vector3F n;
			Vector3F tail = bolts[i].head - bolts[i].len*bolts[i].dir;
			CrossProduct( eyeNormal, bolts[i].head - tail, &n );
			n.SafeNormalize( 1, 0, 0 );

			vertex[i*4+0].pos = tail - HALF_WIDTH*n;
			vertex[i*4+1].pos = tail + HALF_WIDTH*n;
			vertex[i*4+2].pos = bolts[i].head + HALF_WIDTH*n;
			vertex[i*4+3].pos = bolts[i].head - HALF_WIDTH*n;

			Vector4F color = bolts[i].color;
			vertex[i*4+0].color = color;
			vertex[i*4+1].color = color;
			vertex[i*4+2].color = color;
			vertex[i*4+3].color = color;
		}
	}

	Texture* texture = TextureManager::Instance()->GetTexture( "particle" );

	if ( count ) {
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

		shader.SetStream( stream, vertex, count*6, index );
		shader.Draw();
	}
}

