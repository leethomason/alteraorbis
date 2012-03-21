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

#include "../grinliz/glutil.h"
#include "../grinliz/glmath.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glperformance.h"

#include "gpustatemanager.h"
#include "engine.h"
#include "loosequadtree.h"
#include "renderQueue.h"
#include "surface.h"
#include "texture.h"
#include "particle.h"

/*
	Xenoengine-2 has a cleaned up render queue. The sorting of items probably makes the engine
	faster even when not instancing. Cleaning up code makes the underlying algorithm clearer.
*/

using namespace grinliz;

#define ENGINE_RENDER_MODELS
#define ENGINE_RENDER_SHADOWS
#define ENGINE_RENDER_MAP


Engine::Engine( Screenport* port, const gamedb::Reader* database ) 
	:	
		AMBIENT( 0.5f ),
		DIFFUSE( 0.6f ),
		DIFFUSE_SHADOW( 0.2f ),
		screenport( port ),
		initZoomDistance( 0 ),
		map( 0 )
{
	spaceTree = new SpaceTree( -0.1f, 3.0f, 64 );	// fixme: map size hardcoded
	renderQueue = new RenderQueue();

	SetLightDirection( 0 );
	enableMeta = mapMakerMode;
}


Engine::~Engine()
{
	delete renderQueue;
	delete spaceTree;
}


bool Engine::mapMakerMode = false;


void Engine::CameraIso( bool normal, bool sizeToWidth, float width, float height )
{
	if ( normal ) {
		camera.SetYRotation( -45.f );
		camera.SetTilt( -50.0f );
		MoveCameraHome();
		//camera.SetPosWC( 0, 10, 0 0 );
	}
	else {
		float h = 0;
		float theta = ToRadian( EL_FOV/2.0f );
		float ratio = screenport->UIAspectRatio();

		if ( sizeToWidth ) {
			h = (width) / tanf(theta);		
		}
		else {
			h = (height/2) / (tanf(theta)*ratio);
		}
		camera.SetYRotation( 0 );
		camera.SetTilt( -90.0f );
		camera.SetPosWC( width/2.0f, h, height/2.0f );
	}
}


void Engine::MoveCameraHome()
{
	camera.SetPosWC( EL_MAP_SIZE/2, 25.0f, EL_MAP_SIZE/2 );
	camera.SetYRotation( -45.f );
	camera.SetTilt( -50.0f );
}


void Engine::CameraLookAt( float x, float z, float heightOfCamera, float yRotation, float tilt )
{
	camera.SetPosWC( x, heightOfCamera, z );
	camera.SetYRotation( yRotation );
	camera.SetTilt( tilt );

	Vector3F at;
	CameraLookingAt( &at );
	
	camera.DeltaPosWC( x-at.x, 0, z-at.z );
}


void Engine::CameraLookingAt( grinliz::Vector3F* at )
{
	const Vector3F* eyeDir = camera.EyeDir3();
	float h = camera.PosWC().y;

	*at = camera.PosWC() - eyeDir[0]*(h/eyeDir[0].y);	
}


void Engine::MoveCameraXZ( float x, float z, Vector3F* calc )
{
	// Move the camera, but don't change tilt or rotation.
	// The distance is based on triangle ratios.
	//
	const Vector3F* eyeDir = camera.EyeDir3();

	Vector3F start = { x, 0.0f, z };
	
	float h = camera.PosWC().y;
	Vector3F pos = start + eyeDir[0]*(h/eyeDir[0].y);

	if ( calc ) {
		*calc = pos;
	}
	else {
		camera.SetPosWC( pos.x, pos.y, pos.z );
	}
}


void Engine::SetLightDirection( const grinliz::Vector3F* dir ) 
{
	lightDirection.Set( 2.0f, 3.0f, 1.0f );
	if ( dir ) {
		lightDirection = *dir;
	}
	lightDirection.Normalize();
}

Model* Engine::AllocModel( const ModelResource* resource )
{
	GLASSERT( resource );
	return spaceTree->AllocModel( resource );
}


void Engine::FreeModel( Model* model )
{
	spaceTree->FreeModel( model );
}


void Engine::Draw( U32 deltaTime )
{
	GRINLIZ_PERFTRACK;

	// -------- Camera & Frustum -------- //
	screenport->SetView( camera.ViewMatrix() );	// Draw the camera

#ifdef DEBUG
	{
		Vector3F at;
		CameraLookingAt( &at );
		//GLOUTPUT(( "View set. Camera at (%.1f,%.1f,%.1f) looking at (%.1f,%.1f,%.1f)\n",
		//	camera.PosWC().x, camera.PosWC().y, camera.PosWC().z,
		//	at.x, at.y, at.z ));
		if ( map ) {
			Rectangle2I b = map->Bounds();
			b.Outset( 2 );
			if ( !b.Contains( (int)at.x, (int)at.z ) ) {
				GLASSERT( 0 );	// looking at nothing.
			}
		}
	}
#endif
				

	// Compute the frustum planes and query the tree.
	Plane planes[6];
	CalcFrustumPlanes( planes );

	int exclude = Model::MODEL_INVISIBLE;
	exclude |= enableMeta ? 0 : Model::MODEL_METADATA;
	Model* modelRoot = spaceTree->Query( planes, 6, 0, exclude, false );
	
	Color4F ambient, diffuse;
	Vector4F dir;
	QueryLights( DAY_TIME, &ambient, &dir, &diffuse );

	LightShader lightShader( ambient, dir, diffuse );

	Rectangle2I mapBounds( 0, 0, EL_MAP_SIZE-1, EL_MAP_SIZE-1 );
	if ( map ) {
		mapBounds = map->Bounds();
	}


	// ------------ Process the models into the render queue -----------
	{
		GLASSERT( renderQueue->Empty() );

		for( Model* model=modelRoot; model; model=model->next ) {
			model->Queue( renderQueue, &lightShader, 0, 0 );
		}
	}


	// ----------- Render Passess ---------- //
	if ( map ) {
		// Draw shadows to stencil buffer.
		float shadowAmount = 1.0f;
		Color3F shadow, lighted;
		static const Vector3F groundNormal = { 0, 1, 0 };
		CalcLight( DAY_TIME, groundNormal, 1.0f, &lighted, &shadow );

#ifdef ENGINE_RENDER_SHADOWS
		if ( shadowAmount > 0.0f ) {
			FlatShader shadowShader;
			shadowShader.SetStencilMode( GPUShader::STENCIL_WRITE );
			shadowShader.SetDepthTest( false );	// flat plane. 1st pass.
			shadowShader.SetDepthWrite( false );
			shadowShader.SetColorWrite( false );
			shadowShader.SetColor( 1, 0, 0 );	// testing

			Matrix4 shadowMatrix;
			shadowMatrix.m12 = -lightDirection.x/lightDirection.y;
			shadowMatrix.m22 = 0.0f;
			shadowMatrix.m32 = -lightDirection.z/lightDirection.y;

			renderQueue->Submit(	&shadowShader,
									0,
									Model::MODEL_NO_SHADOW,
									&shadowMatrix );

			map->Draw3D( shadow, GPUShader::STENCIL_SET );
		}
#endif
		map->Draw3D( lighted, GPUShader::STENCIL_CLEAR );

#ifdef ENGINE_RENDER_MAP
		map->DrawOverlay();
#endif
	}

	// -------- Models ---------- //
#ifdef ENGINE_RENDER_MODELS
	{
		renderQueue->Submit( 0, 0, 0, 0 );
	}
#endif

	if ( map )
		map->DrawOverlay();
	renderQueue->Clear();

	const Vector3F* eyeDir = camera.EyeDir3();
	ParticleSystem* particleSystem = ParticleSystem::Instance();
	particleSystem->Update( deltaTime, eyeDir );
	particleSystem->Draw();
}


void Engine::QueryLights( DayNight dayNight, Color4F* ambient, Vector4F* dir, Color4F* diffuse )
{
	ambient->Set( AMBIENT, AMBIENT, AMBIENT, 1.0f );
	diffuse->Set( DIFFUSE, DIFFUSE, DIFFUSE, 1.0f );

	if ( dayNight == NIGHT_TIME ) {
		diffuse->r *= EL_NIGHT_RED;
		diffuse->g *= EL_NIGHT_GREEN;
		diffuse->b *= EL_NIGHT_BLUE;
	}
	dir->Set( lightDirection.x, lightDirection.y, lightDirection.z, 0 );	// '0' in last term is parallel
}


void Engine::CalcLight( DayNight dayNight, const Vector3F& normal, float shadowAmount, Color3F* light, Color3F* shadow )
{
	Color4F ambient, diffuse;
	Vector4F dir;
	QueryLights( dayNight, &ambient, &dir, &diffuse );
	Vector3F dir3 = { dir.x, dir.y, dir.z };

	float nDotL = Max( 0.0f, DotProduct( normal, dir3 ) );
	for( int i=0; i<3; ++i ) {
		light->X(i)  = ambient.X(i) + diffuse.X(i)*nDotL;
		shadow->X(i) = ambient.X(i);

		shadow->X(i) = InterpolateUnitX( shadow->X(i), light->X(i), 1.f-shadowAmount ); 
	}
}


bool Engine::RayFromViewToYPlane( const Vector2F& v, const Matrix4& mvpi, Ray* ray, Vector3F* out )
{	
	screenport->ViewToWorld( v, &mvpi, ray );

	Plane plane;
	plane.n.Set( 0.0f, 1.0f, 0.0f );
	plane.d = 0.0;
	
	int result = REJECT;
	if ( out ) {
		float t;
		result = IntersectLinePlane( ray->origin, ray->origin+ray->direction, plane, out, &t );
	}
	return ( result == INTERSECT );
}


void Engine::CalcFrustumPlanes( grinliz::Plane* planes )
{
	// --------- Compute the view frustum ----------- //
	// A strange and ill-documented algorithm from Real Time Rendering, 2nd ed, pg.613
	Matrix4 m;
	screenport->ViewProjection3D( &m );

	// m is the matrix from multiplying projection and model. The
	// subscript is the row.

	// Left
	// -(m3 + m0) * (x,y,z,0) = 0
	planes[0].n.x = ( m.x[3+0]  + m.x[0+0] );
	planes[0].n.y = ( m.x[3+4]  + m.x[0+4] );
	planes[0].n.z = ( m.x[3+8]  + m.x[0+8] );
	planes[0].d   = ( m.x[3+12] + m.x[0+12] );
	planes[0].Normalize();

	// right
	planes[1].n.x = ( m.x[3+0]  - m.x[0+0] );
	planes[1].n.y = ( m.x[3+4]  - m.x[0+4] );
	planes[1].n.z = ( m.x[3+8]  - m.x[0+8] );
	planes[1].d   = ( m.x[3+12] - m.x[0+12] );
	planes[1].Normalize();

	// bottom
	planes[2].n.x = ( m.x[3+0]  + m.x[1+0] );
	planes[2].n.y = ( m.x[3+4]  + m.x[1+4] );
	planes[2].n.z = ( m.x[3+8]  + m.x[1+8] );
	planes[2].d   = ( m.x[3+12] + m.x[1+12] );
	planes[2].Normalize();

	// top
	planes[3].n.x = ( m.x[3+0]  - m.x[1+0] );
	planes[3].n.y = ( m.x[3+4]  - m.x[1+4] );
	planes[3].n.z = ( m.x[3+8]  - m.x[1+8] );
	planes[3].d   = ( m.x[3+12] - m.x[1+12] );
	planes[3].Normalize();

	// near
	planes[4].n.x = ( m.x[3+0]  + m.x[2+0] );
	planes[4].n.y = ( m.x[3+4]  + m.x[2+4] );
	planes[4].n.z = ( m.x[3+8]  + m.x[2+8] );
	planes[4].d   = ( m.x[3+12] + m.x[2+12] );
	planes[4].Normalize();

	// far
	planes[5].n.x = ( m.x[3+0]  - m.x[2+0] );
	planes[5].n.y = ( m.x[3+4]  - m.x[2+4] );
	planes[5].n.z = ( m.x[3+8]  - m.x[2+8] );
	planes[5].d   = ( m.x[3+12] - m.x[2+12] );
	planes[5].Normalize();

	GLASSERT( DotProduct( planes[LEFT].n, planes[RIGHT].n ) < 0.0f );
	GLASSERT( DotProduct( planes[TOP].n, planes[BOTTOM].n ) < 0.0f );
	GLASSERT( DotProduct( planes[NEAR].n, planes[FAR].n ) < 0.0f );
}


Model* Engine::IntersectModel( const grinliz::Ray& ray, HitTestMethod method, int required, int exclude, const Model* ignore[], Vector3F* intersection )
{
	GRINLIZ_PERFTRACK

	Model* model = spaceTree->QueryRay(	ray.origin, ray.direction, 
										required, exclude, ignore,
										method,
										intersection );
	return model;
}


void Engine::RestrictCamera()
{
	const Vector3F* eyeDir = camera.EyeDir3();

	Vector3F intersect;
	IntersectRayPlane( camera.PosWC(), eyeDir[0], XZ_PLANE, 0.0f, &intersect );
//	GLOUTPUT(( "Intersect %.1f, %.1f, %.1f\n", intersect.x, intersect.y, intersect.z ));

	const float SIZEX = (float)map->Width();
	const float SIZEZ = (float)map->Height();

	if ( intersect.x < 0.0f ) {
		camera.DeltaPosWC( -intersect.x, 0.0f, 0.0f );
	}
	if ( intersect.x > SIZEX ) {
		camera.DeltaPosWC( -(intersect.x-SIZEX), 0.0f, 0.0f );
	}
	if ( intersect.z < 0.0f ) {
		camera.DeltaPosWC( 0.0f, 0.0f, -intersect.z );
	}
	if ( intersect.z > SIZEZ ) {
		camera.DeltaPosWC( 0.0f, 0.0f, -(intersect.z-SIZEZ) );
	}
}



void Engine::SetZoom( float z )
{
//	float startY = camera.PosWC().y;

	z = Clamp( z, GAME_ZOOM_MIN, GAME_ZOOM_MAX );
	float d = Interpolate(	GAME_ZOOM_MIN, EL_CAMERA_MIN,
							GAME_ZOOM_MAX, EL_CAMERA_MAX,
							z );

	const Vector3F* eyeDir = camera.EyeDir3();
	Vector3F origin;
	int result = IntersectRayPlane( camera.PosWC(), eyeDir[0], 1, 0.0f, &origin );
	if ( result != grinliz::INTERSECT ) {
		MoveCameraHome();
	}
	else {
		// The 'd' is the actual height. Adjust for the angle of the camera.
		float len = ( d ) / eyeDir[0].y;
		Vector3F pos = origin + len*eyeDir[0];
		camera.SetPosWC( pos );
	}
	//GLOUTPUT(( "Engine set zoom. y=%f to y=%f", startY, camera.PosWC().y ));
}


float Engine::GetZoom() 
{
	float z = Interpolate(	EL_CAMERA_MIN, GAME_ZOOM_MIN,
							EL_CAMERA_MAX, GAME_ZOOM_MAX,
							camera.PosWC().y );
	return z;
}
