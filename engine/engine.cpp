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

/*
void Engine::PushShadowSwizzleMatrix( GPUShader* shader )
{
	// A shadow matrix for a flat y=0 plane! heck yeah!
	shadowMatrix.m12 = -lightDirection.x/lightDirection.y;
	shadowMatrix.m22 = 0.0f;
	shadowMatrix.m32 = -lightDirection.z/lightDirection.y;

	// x'    1/64  0    0    0
	// y'      0   0  -1/64  -1
	//     =   0   0    0    0
	//		
	Matrix4 swizzle;
	swizzle.m11 = 1.f/((float)EL_MAP_SIZE);
	swizzle.m22 = 0;	swizzle.m23 = -1.f/((float)EL_MAP_SIZE);	swizzle.m24 = 1.0f;
	swizzle.m33 = 0.0f;

	shader->PushTextureMatrix( 3 );
	shader->MultTextureMatrix( 3, swizzle );
	shader->MultTextureMatrix( 3, shadowMatrix );

	shader->PushMatrix( GPUShader::MODELVIEW_MATRIX );
	shader->MultMatrix( GPUShader::MODELVIEW_MATRIX, shadowMatrix );
}
*/

/*
void Engine::PushLightSwizzleMatrix( GPUShader* shader )
{
	float w, h, dx, dz;
	iMap->LightFogMapParam( &w, &h, &dx, &dz );

	Matrix4 swizzle;
	swizzle.m11 = 1.f/w;
	swizzle.m22 = 0;	swizzle.m23 = -1.f/h;	swizzle.m24 = 1.0f;
	swizzle.m33 = 0.0f;

	swizzle.m14 = dx;
	swizzle.m34 = dz;

	shader->PushTextureMatrix( 2 );
	shader->MultTextureMatrix( 2, swizzle );
}
*/


void Engine::Draw()
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
	CalcLights( DAY_TIME, &ambient, &dir, &diffuse );

	LightShader lightShader( ambient, dir, diffuse, false );

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
//	Color4F color;

	if ( map ) {
		map->Draw3D();

		// If the map is enabled, we draw the basic map plane lighted. Then draw the model shadows.
		// The shadows are the tricky part: one matrix is used to transform the vertices to the ground
		// plane, and the other matrix is used to transform the vertices to texture coordinates.
		// Shaders make this much, much, much easier.

		// -------- Ground plane lighted -------- //

		// -------- Shadow casters/ground plane ---------- //
		// Set up the planar projection matrix, with a little z offset
		// to help with z resolution fighting.
		const float SHADOW_START_HEIGHT = 80.0f;
		const float SHADOW_END_HEIGHT   = SHADOW_START_HEIGHT + 5.0f;
		float shadowAmount = 1.0f;
		if ( camera.PosWC().y > SHADOW_START_HEIGHT ) {
			shadowAmount = 1.0f - ( camera.PosWC().y - SHADOW_START_HEIGHT ) / ( SHADOW_END_HEIGHT - SHADOW_START_HEIGHT );
		}
#ifdef ENGINE_RENDER_SHADOWS
		if ( shadowAmount > 0.0f ) {
			FlatShader shadowShader;
			shadowShader.SetColor( 1, 0, 0 );

			Matrix4 shadowMatrix;
			shadowMatrix.m12 = -lightDirection.x/lightDirection.y;
			shadowMatrix.m22 = 0.0f;
			shadowMatrix.m32 = -lightDirection.z/lightDirection.y;

			renderQueue->Submit(	&shadowShader,
									0,
									Model::MODEL_NO_SHADOW,
									&shadowMatrix );
		}
#endif

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
}


void Engine::CalcLights( DayNight dayNight, Color4F* ambient, Vector4F* dir, Color4F* diffuse )
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


void Engine::LightGroundPlane( DayNight dayNight, ShadowState shadows, float shadowAmount, Color4F* outColor )
{
	outColor->Set( 1, 1, 1, 1 );

	if ( shadows == IN_SHADOW ) {
		float delta = 1.0f - DIFFUSE_SHADOW*shadowAmount;
		outColor->r *= delta;
		outColor->g *= delta;
		outColor->b *= delta;
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


/*
void Engine::ResetRenderCache()
{
	renderQueue->ResetRenderCache();
	Model* model = spaceTree->Query( 0, 0, 0, 0 );
	for( ; model; model = model->next ) {
		for( int i=0; i<EL_MAX_MODEL_GROUPS; ++i )
			model->cacheStart[i] = Model::CACHE_UNINITIALIZED;
	}
}
*/
