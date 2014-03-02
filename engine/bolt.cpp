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
#include "particle.h"

#include "../game/gameitem.h"
#include "../xarchive/glstreamer.h"
#include "../script/worldgen.h"

using namespace grinliz;


void Bolt::Serialize( XStream* xs )
{
	XarcOpen( xs, "bolt" );
	XARC_SER( xs, len );
	XARC_SER( xs, impact );
	XARC_SER( xs, speed );
	XARC_SER( xs, particle );
	XARC_SER( xs, chitID );
	XARC_SER( xs, damage );
	XARC_SER( xs, effect );
	XARC_SER( xs, head );
	XARC_SER( xs, dir );
	XARC_SER( xs, color );
	XarcClose( xs );
}


void Bolt::TickAll( grinliz::CDynArray<Bolt>* bolts, U32 delta, Engine* engine, IBoltImpactHandler* handler )
{
	GLASSERT( engine->GetMap() );
	Map* map = engine->GetMap();

	float timeSlice = (float)delta / 1000.0f;

	int i=0;	
	ParticleSystem* ps = engine->particleSystem;
	Rectangle3F bounds = map->Bounds3();
	bool usingSectors = map->UsingSectors();

	while ( i < bolts->Size() ) {
		Bolt& b = (*bolts)[i];
 
		if ( usingSectors ) {
			// Get bounds before moving, so it can't "tunnel" through.
			bounds = SectorData::SectorBounds3( b.head.x, b.head.z );
		}
		
		float distance = b.speed * timeSlice;
		Vector3F travel = b.dir * distance;
		Vector3F normal = { 0, 1, 0 };
		ModelVoxel mv;

		if ( !b.impact ) {
			// Check if we hit something in the world.
			// FIXME: add ignore list of the shooter, or move head away from model?

			// Check ground hit.
			if ( (b.head + travel).y <= 0 ) {
				b.impact = true;
					
				if ( b.head.y > 0 ) {
					mv.at = Lerp( b.head, b.head+travel, (b.head.y) / (travel.y));
				}
				else {
					mv.at = b.head;	// not really sure how this happened.
				}
			}

			// Check model hit.
			if ( !b.impact ) {
				mv = engine->IntersectModelVoxel( b.head, b.dir, distance, TEST_TRI, 0, 0, 0 );
				if ( mv.Hit() ) {
					b.impact = true;
					normal = b.dir;
				}
			}

			if ( b.impact ) {
				GLASSERT( !mv.at.IsZero() );
				if ( handler ) {
					handler->HandleBolt( b, mv );
				}
				b.head = mv.at;

				ParticleDef def = ps->GetPD( "boltImpact" );
				def.color = b.color;
				ps->EmitPD( def, mv.at, -normal, delta );

				if ( b.effect & GameItem::EFFECT_EXPLOSIVE ) {
					def = ps->GetPD( "explosion" );
					def.color = b.color;
					ps->EmitPD( def, mv.at, V3F_UP, delta );
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
	vbo = new GPUVertexBuffer( 0, MAX_BOLTS*4*sizeof(PTCVertex) );

	float u0 = (float)ParticleDef::BOLT / (float)ParticleDef::NUM_TEX;
	float u1 = (float)(ParticleDef::BOLT+1) / (float)ParticleDef::NUM_TEX;

	for( int i=0; i<MAX_BOLTS; ++i ) {
		vertex[i*4+0].tex.Set( u0, 0 );
		vertex[i*4+1].tex.Set( u1, 0 );
		vertex[i*4+2].tex.Set( u1, 1 );
		vertex[i*4+3].tex.Set( u0, 1 );

		for( int k=0; k<4; ++k ) {
			vertex[i*4+k].color.Set( 1,1,1,1 );
		}
	}
	ShaderManager::Instance()->AddDeviceLossHandler( this );
}


BoltRenderer::~BoltRenderer()
{
	ShaderManager::Instance()->RemoveDeviceLossHandler( this );
	delete vbo;
}


void BoltRenderer::DeviceLoss()
{
	delete vbo;
	vbo = new GPUVertexBuffer( 0, MAX_BOLTS*4*sizeof(PTCVertex));
}


void BoltRenderer::DrawAll( const Bolt* bolts, int nBolts, Engine* engine )
{
	if ( nBolts == 0 )
		return;

	const Vector3F origin = engine->camera.PosWC();
	const float RAD2 = EL_FAR*EL_FAR;

	Vector3F eyeNormal = engine->camera.EyeDir3()[Camera::NORMAL];
	static const float HALF_WIDTH = 0.07f;

	ParticleSystem* ps = engine->particleSystem;
	ParticleDef def = ps->GetPD( "smoketrail" );

	int count = 0;
	for( int i=0; i<nBolts && count<MAX_BOLTS; ++i ) {

		Vector3F head = bolts[i].head;

		if ( ( head - origin ).LengthSquared() > RAD2 )
			continue;

		if ( bolts[i].particle ) {
			def.color = bolts[i].color;
			ps->EmitPD( def, bolts[i].head, V3F_UP, 0 );
		}
		else {
			Vector3F n;
			Vector3F tail = head - bolts[i].len*bolts[i].dir;
			CrossProduct( eyeNormal, head - tail, &n );
			n.Normalize();

			int base = count*4;
			vertex[base+0].pos = tail - HALF_WIDTH*n;
			vertex[base+1].pos = tail + HALF_WIDTH*n;
			vertex[base+2].pos = head + HALF_WIDTH*n;
			vertex[base+3].pos = head - HALF_WIDTH*n;

			Vector4F color = bolts[i].color;
			vertex[base+0].color = color;
			vertex[base+1].color = color;
			vertex[base+2].color = color;
			vertex[base+3].color = color;

			++count;
		}
	}

	Texture* texture = TextureManager::Instance()->GetTexture( "particle" );

	if ( count ) {

		vbo->Upload( vertex, count*4*sizeof(vertex[0]), 0 );

		GPUStream stream;
		stream.stride			= sizeof( vertex[0] );
		stream.nPos				= 3;
		stream.posOffset		= PTCVertex::POS_OFFSET;
		stream.texture0Offset	= PTCVertex::TEXTURE_OFFSET;
		stream.nColor			= 4;
		stream.colorOffset		= PTCVertex::COLOR_OFFSET;

		ParticleShader shader;
		GPUStreamData data;
		data.texture0 = texture;
		data.vertexBuffer = vbo->ID();
		GPUDevice::Instance()->DrawQuads( shader, stream, data, count );
	}
}

