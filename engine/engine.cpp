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
#include "../Shiny/include/Shiny.h"

#include "../audio/xenoaudio.h"

#include "gpustatemanager.h"
#include "shadermanager.h"
#include "engine.h"
#include "loosequadtree.h"
#include "renderQueue.h"
#include "surface.h"
#include "texture.h"
#include "particle.h"
#include "rendertarget.h"
#include "engineshaders.h"
#include "bolt.h"

#include "../game/layout.h"

/*
	XenoEngine-2 has a cleaned up render queue. The sorting of items probably makes the engine
	faster even when not instancing. Cleaning up code makes the underlying algorithm clearer.
*/

using namespace grinliz;

#define ENGINE_RENDER_MODELS
#define ENGINE_RENDER_SHADOWS
#define ENGINE_RENDER_MAP
#define ENGINE_RENDER_GLOW

// Debugging:
//#define ENGINE_GLOW_TO_FRAMEBUFFER
//#define ENGINE_RT1_TO_FRAMEBUFFER

//#define ENGINE_DETAILED_PROFILE PROFILE_BLOCK
#define ENGINE_DETAILED_PROFILE(name)

Engine* StackedSingleton<Engine>::instance = 0;


Engine::Engine( Screenport* port, const gamedb::Reader* database, Map* m ) 
	:	
		screenport( port ),
		initZoomDistance( 0 ),
		stages( 0xffff ),
		map( 0 ),
		uiRenderer( GPUDevice::ENGINE )
{
	map = m;
	temperature = 0;
	frame = 0;
	if ( map ) 
		spaceTree = new SpaceTree( -0.1f, 3.1f, Max( m->Width(), m->Height() ) );
	else
		spaceTree = new SpaceTree( -0.1f, 3.1f, 64 );

	renderQueue = new RenderQueue();
	for( int i=0; i<RT_COUNT; ++i )
		renderTarget[i] = 0;
	ShaderManager::Instance()->AddDeviceLossHandler( this );
	particleSystem = new ParticleSystem();
	boltRenderer = new BoltRenderer();

	overlay.Init(&uiRenderer);
	overlay.SetScale(port->PhysicalWidth(), port->PhysicalHeight(), LAYOUT_VIRTUAL_HEIGHT);
	FontSingleton* bridge = FontSingleton::Instance();
	overlay.SetText(bridge->TextAtom(false), bridge->TextAtom(true), FontSingleton::Instance());
	overlay.SetPixelSnap(false);
}


Engine::~Engine()
{
	delete particleSystem;
	ShaderManager::Instance()->RemoveDeviceLossHandler( this );
	delete renderQueue;
	delete spaceTree;
	for( int i=0; i<RT_COUNT; ++i )
		delete renderTarget[i];
	delete boltRenderer;
}


void Engine::DeviceLoss()
{
	for( int i=0; i<RT_COUNT; ++i ) {
		delete renderTarget[i];
		renderTarget[i] = 0;
	}
}


void Engine::CameraIso( bool normal, bool sizeToWidth, float width, float height )
{
	if ( normal ) {
		camera.TiltRotationToQuat( -45.f, -50.f );
		MoveCameraHome();
	}
	else {
		float h = 0;
		float theta = ToRadian( EL_FOV/2.0f );
		float ratio = 0.5f;	// FIXME screenport->UIAspectRatio();

		if ( sizeToWidth ) {
			h = (width) / tanf(theta);		
		}
		else {
			h = (height/2) / (tanf(theta)*ratio);
		}
		camera.TiltRotationToQuat( -90.0f, 0 );
		camera.SetPosWC( width/2.0f, h, height/2.0f );
	}
}



void Engine::MoveCameraHome()
{
	camera.SetPosWC( 1, 25.0f, 1 );
	camera.TiltRotationToQuat( -50.f, -45.f );
}



void Engine::CameraLookAt( float x, float z )
{
	Vector3F target = { x, 0, z };
	Vector3F at = { 0, 0, 0 };
	CameraLookingAt( &at );
	if ( Equal( at.y, 0.0f, 0.01f )) {
		Vector3F delta = target - at;
		camera.DeltaPosWC( delta.x, 0, delta.z );
	}
}


void Engine::CameraLookAt( const grinliz::Vector3F& c, const grinliz::Vector3F& target )
{
	camera.SetPosWC( c );

	Vector3F dir = target - c;
	dir.Normalize();
	camera.SetDir( dir, V3F_UP );
}


void Engine::CameraLookAt( float x, float z, float heightOfCamera, float yRotation, float tilt )
{
	camera.SetPosWC( x, heightOfCamera, z );
	camera.TiltRotationToQuat( tilt, yRotation );

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


Texture* Engine::GetMiniMapTexture()
{
	return TextureManager::Instance()->GetTexture( "miniMap" );
}


void Engine::QueueSet(	EngineShaders* engineShaders, 
					    const CDynArray<Model*>& models,
						int requiredModelFlag, int excludedModelFlag,						
						int requiredShaderFlag, int excludedShaderFlag )
{
	renderQueue->Clear();
	for (int i = 0; i < models.Size(); ++i) {
		Model* model = models[i];
		int flag = model->Flags();
		if (    (( requiredModelFlag & flag ) == requiredModelFlag )
			 && (( excludedModelFlag & flag ) == 0 ) )
		{
			model->Queue( renderQueue, engineShaders, requiredShaderFlag, excludedShaderFlag );
		}
	}
}


void Engine::Draw(U32 deltaTime, const Bolt* bolts, int nBolts, IUITracker* tracker)
{
	PROFILE_FUNC();
	++frame;
	for (int i = 0; i < NUM_MODEL_DRAW_CALLS; ++i)
		modelDrawCalls[i] = 0;

	// Engine should probably have a re-size.
	if (overlay.PixelWidth() != screenport->PhysicalWidth() || overlay.PixelHeight() != screenport->PhysicalHeight()) {
		overlay.SetScale(screenport->PhysicalWidth(), screenport->PhysicalHeight(), LAYOUT_VIRTUAL_HEIGHT);
	}

	// -------- Camera & Frustum -------- //
	screenport->SetView(camera.ViewMatrix());	// Draw the camera

	Rectangle3F particleClip;
	particleClip.Set(camera.PosWC().x - EL_FAR, -10.0f, camera.PosWC().z - EL_FAR,
					 camera.PosWC().x + EL_FAR,  100.0f, camera.PosWC().z + EL_FAR);
	particleSystem->SetClip(particleClip);

	if (XenoAudio::Instance()) {
		Vector3F lookingAt = { 0, 0, 0 };
		this->CameraLookingAt(&lookingAt);
		const Vector3F* camDir = camera.EyeDir3();
		Vector3F dir = camDir[0];
		dir.y = 0;
		XenoAudio::Instance()->SetListener(lookingAt, dir);
	}

#ifdef DEBUG
#if 0	// turn back on if camera problems
	{
		Vector3F at;
		CameraLookingAt(&at);
		//GLOUTPUT(( "View set. Camera at (%.1f,%.1f,%.1f) looking at (%.1f,%.1f,%.1f)\n",
		//	camera.PosWC().x, camera.PosWC().y, camera.PosWC().z,
		//	at.x, at.y, at.z ));
		if (map) {
			Rectangle2I b = map->Bounds();
			b.Outset(20);
			if (!b.Contains((int)at.x, (int)at.z)) {
				GLASSERT(0);	// looking at nothing.
			}
		}
	}
#endif
#endif

	// Compute the frustum planes and query the tree.
	Plane planes[6];
	CalcFrustumPlanes(planes);

	// Get the working set of models.
	modelCache.Clear();
	spaceTree->Query(&modelCache, planes, 6, 0, true, 0, Model::MODEL_INVISIBLE);

	// Track the UI relevant ones:
	trackedModels.Clear();
	for (int i = 0; i < modelCache.Size(); ++i) {
		Model* m = modelCache[i];
		if (m->Flags() & Model::MODEL_UI_TRACK) {
			trackedModels.Push(m);
		}
	}

	if ( map && (stages & STAGE_VOXEL) ) {
		ENGINE_DETAILED_PROFILE(MapPrep);
		map->PrepVoxels(spaceTree, &modelCache, planes);
		map->PrepGrid( spaceTree );
	}
	
	GPUDevice* device = GPUDevice::Instance();

	Color4F ambient, diffuse, shadow;
	Vector4F dir;
	lighting.Query( &diffuse, &ambient, &shadow, temperature, &dir );
	device->ambient = ambient;
	device->directionWC = dir;
	device->diffuse = diffuse;

	EngineShaders engineShaders;
	{
		int flag = ShaderManager::LIGHTING;
		LightShader light( flag );
		LightShader em( flag );
		em.SetShaderFlag( ShaderManager::EMISSIVE );
		LightShader blend( flag, BLEND_NORMAL );

		engineShaders.Push( EngineShaders::LIGHT, light );
		engineShaders.Push( EngineShaders::EMISSIVE, em );
		engineShaders.Push( EngineShaders::BLEND, blend );
	}
	Rectangle2I mapBounds( 0, 0, EL_MAP_SIZE-1, EL_MAP_SIZE-1 );
	if ( map ) {
		mapBounds = map->Bounds();
	}

	// ----------- Render Passess ---------- //
#ifdef ENGINE_RENDER_GLOW
	if (stages & STAGE_GLOW) {
		ENGINE_DETAILED_PROFILE(Glow);
		if (!renderTarget[RT_LIGHTS]) {
			renderTarget[RT_LIGHTS] = new RenderTarget(screenport->PhysicalWidth(), screenport->PhysicalHeight(), true);
		}
		renderTarget[RT_LIGHTS]->SetActive(true, this);
		renderTarget[RT_LIGHTS]->Clear();
		renderTarget[RT_LIGHTS]->screenport->SetPerspective();
		// ---------- Pass 1 -----------
		{
			// Tweak the shaders for glow-only rendering.
			FlatShader emEx;
			//emEx.SetColor(1, 0, 0);
			emEx.SetShaderFlag(ShaderManager::EMISSIVE);			// Interpret alpha as emissive.
			emEx.SetShaderFlag(ShaderManager::EMISSIVE_EXCLUSIVE);	// Render emmissive colors or black.

			// Replace all kinds of lighting with emissive:
			engineShaders.PushAll(emEx);
			QueueSet(&engineShaders, modelCache, 0, 0, 0, 0);

			int dc = device->DrawCalls();
			if (map) map->Submit(&emEx);
			if (map && (stages & STAGE_VOXEL)) map->DrawVoxels(&emEx, 0);
			renderQueue->Submit(0, 0, 0);
			modelDrawCalls[GLOW_EMISSIVE] = dc - device->DrawCalls();

			engineShaders.PopAll();
		}
		renderTarget[RT_LIGHTS]->SetActive(false, this);
	}
#endif

	if ( map ) {
		ENGINE_DETAILED_PROFILE(Map);
#ifdef ENGINE_RENDER_MAP
		// Draw shadows to stencil buffer.
		float shadowAmount = 1.0f;
		Color3F shadow, lighted;
		static const Vector3F groundNormal = { 0, 1, 0 };
		lighting.CalcLight( groundNormal, 1.0f, &lighted, &shadow, temperature );

#ifdef ENGINE_RENDER_SHADOWS
		if ( shadowAmount > 0.0f && (stages & STAGE_SHADOW) ) {

			FlatShader shadowShader;
			shadowShader.SetStencilMode( STENCIL_WRITE );
			shadowShader.SetDepthTest( false );	// flat plane. 1st pass.
			shadowShader.SetDepthWrite( false );
			shadowShader.SetColorWrite( false );

			engineShaders.PushAll( shadowShader );
			QueueSet( &engineShaders, modelCache, 0, Model::MODEL_NO_SHADOW, 0, EngineShaders::BLEND );

			Matrix4 shadowMatrix;
			shadowMatrix.SetCol(1,
								-lighting.direction.x / lighting.direction.y,
								0.0f,
								-lighting.direction.z / lighting.direction.y,
								0);

			{
				modelDrawCalls[SHADOW] = device->DrawCalls();
				if ( map && (stages && STAGE_VOXEL) ) {
					map->DrawVoxels( &shadowShader, &shadowMatrix );
				}
				renderQueue->Submit( 0, 0, &shadowMatrix );
				modelDrawCalls[SHADOW] = device->DrawCalls() - modelDrawCalls[SHADOW];
			}
			engineShaders.PopAll();

			map->Draw3D( shadow, STENCIL_SET, true );
		}
		map->Draw3D( lighted, STENCIL_CLEAR, true );
#else
		map->Draw3D( lighted, STENCIL_OFF, true );
#endif
		map->DrawOverlay(0);
#endif
	}

	// -------- Models ---------- //
#ifdef ENGINE_RENDER_MODELS
	{
		ENGINE_DETAILED_PROFILE(Models);
		if ( map && (stages & STAGE_VOXEL) ) {
			GPUState state;
			engineShaders.GetState( EngineShaders::LIGHT, 0, &state );

			state.SetShaderFlag( ShaderManager::SATURATION );
			map->DrawVoxels( &state, 0 );
		}
		QueueSet( &engineShaders, modelCache, 0, 0, 0, 0  );
		{
			modelDrawCalls[MODELS] = device->DrawCalls();
			renderQueue->Submit( 0, 0, 0 );
			modelDrawCalls[MODELS] = device->DrawCalls() - modelDrawCalls[MODELS];
		}
	}
#endif

	engineShaders.PopAll();
	renderQueue->Clear();

	if ( map ) {
		map->DrawOverlay(1);
	}

	// --------- Composite Glow -------- //
#ifdef ENGINE_RENDER_GLOW
	if ( stages & STAGE_GLOW ) {
		ENGINE_DETAILED_PROFILE(CompositeGlow);
		Blur();

		screenport->SetUI();

	#endif

#ifdef ENGINE_GLOW_TO_FRAMEBUFFER
		CompositingShader shader( BLEND_NONE );
		Vector3F p0 = { 0, (float)screenport->PhysicalHeight(), 0 };
		Vector3F p1 = { (float)screenport->PhysicalWidth(), 0, 0 };

		device->DrawQuad( shader, renderTarget[RT_LIGHTS]->GetTexture(), p0, p1 );
#elif defined(ENGINE_RT1_TO_FRAMEBUFFER)
		CompositingShader shader( BLEND_NONE );
		Vector3F p0 = { 0, (float)screenport->PhysicalHeight(), 0 };
		Vector3F p1 = { (float)screenport->PhysicalWidth(), 0, 0 };

		device->DrawQuad( shader, renderTarget[RT_BLUR_1]->GetTexture(), p0, p1 );
#else
	#ifdef EL_USE_MRT_BLUR
		CompositingShader shader( BLEND_ADD );
		for( int i=0; i<BLUR_COUNT; ++i ) {
			shader.SetColor( lighting.glow.x, lighting.glow.y, lighting.glow.z, 0 );
			Vector3F p0 = { 0, (float)screenport->PhysicalHeight(), 0 };
			Vector3F p1 = { (float)screenport->PhysicalWidth(), 0, 0 };

			device->DrawQuad( shader, renderTarget[RT_BLUR_0+i]->GetTexture(), p0, p1 );
		}
	#else
		shader.SetTexture0( renderTarget[RT_BLUR_Y]->GetTexture() );
		shader.SetColor( lighting.glow.r, lighting.glow.g, lighting.glow.b, 0 );
		Vector3F p0 = { 0, screenport->UIHeight(), 0 };
		Vector3F p1 = { screenport->UIWidth(), 0, 0 };
		shader.DrawQuad( p0, p1 );
	#endif
#endif
		screenport->SetPerspective();
		screenport->SetView( camera.ViewMatrix() );	// Draw the camera
	}

	// ------ Particle system ------------- //

	if ( stages & STAGE_BOLT ) {
		boltRenderer->DrawAll( bolts, nBolts, this );
	}
	particleSystem->Update( deltaTime, &camera );
	
	if ( stages & STAGE_PARTICLE ) {
		particleSystem->Draw();
	}
	// ---- Debugging ---
	{
		DrawDebugLines( deltaTime );
	}

	// Overlay elements
	if (tracker) {
		tracker->UpdateUIElements(trackedModels.Mem(), trackedModels.Size());
	}

	screenport->SetUI();
	overlay.Render();
	screenport->SetPerspective();
}


void Engine::Blur()
{
	ENGINE_DETAILED_PROFILE(Blur);
	GPUDevice* device = GPUDevice::Instance();

#ifdef EL_USE_MRT_BLUR
	for( int i=0; i<BLUR_COUNT; ++i ) {
		if ( !renderTarget[i+RT_BLUR_0] ) {
			int shift = i+1;
			renderTarget[i+RT_BLUR_0] = new RenderTarget( screenport->PhysicalWidth()>>shift, screenport->PhysicalHeight()>>shift, false );
		}
	}
	for( int i=0; i<BLUR_COUNT; ++i ) {
		renderTarget[i+RT_BLUR_0]->SetActive( true, this );
		renderTarget[i + RT_BLUR_0]->Clear();
		renderTarget[i+RT_BLUR_0]->screenport->SetUI();
		//int shift = i+1;

		FlatShader shader;
		shader.SetDepthTest(false);
		shader.SetDepthWrite(false);

		Vector3F p0 = { 0, (float)screenport->PhysicalHeight(), 0 };
		Vector3F p1 = { (float)screenport->PhysicalWidth(), 0, 0 };
		device->DrawQuad( shader, renderTarget[i+RT_BLUR_0-1]->GetTexture(), p0, p1 );

		renderTarget[i+RT_BLUR_0]->SetActive( false, this );
	}

#else
	if ( !renderTarget[RT_BLUR_X] ) {
		renderTarget[RT_BLUR_X] = new RenderTarget( screenport->PhysicalWidth(), screenport->PhysicalHeight(), false );
	}
	if ( !renderTarget[RT_BLUR_Y] ) {
		renderTarget[RT_BLUR_Y] = new RenderTarget( screenport->PhysicalWidth(), screenport->PhysicalHeight(), false );
	}

	// --- x ---- //
	renderTarget[RT_BLUR_X]->SetActive( true, this );
	renderTarget[RT_BLUR_X]->screenport->SetUI();

	{
		FlatShader shader;
		shader.SetTexture0( renderTarget[RT_LIGHTS]->GetTexture() );
		shader.SetShaderFlag( ShaderManager::BLUR );
		shader.SetRadius( lighting.glowRadius );
		Vector3F p0 = { 0, screenport->UIHeight(), 0 };
		Vector3F p1 = { screenport->UIWidth(), 0, 0 };
		shader.DrawQuad( p0, p1 );
	}
	renderTarget[RT_BLUR_X]->SetActive( false, this );

	// --- y ---- //
	renderTarget[RT_BLUR_Y]->SetActive( true, this );
	renderTarget[RT_BLUR_Y]->screenport->SetUI();
	{
		FlatShader shader;
		shader.SetTexture0( renderTarget[RT_BLUR_X]->GetTexture() );
		shader.SetShaderFlag( ShaderManager::BLUR );
		shader.SetRadius( lighting.glowRadius );
		shader.SetShaderFlag( ShaderManager::BLUR_Y );
		Vector3F p0 = { 0, screenport->UIHeight(), 0 };
		Vector3F p1 = { screenport->UIWidth(), 0, 0 };
		shader.DrawQuad( p0, p1 );
	}
	renderTarget[RT_BLUR_Y]->SetActive( false, this );
#endif
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
	planes[0].n.x = ( m.X(3+0)  + m.X(0+0) );
	planes[0].n.y = ( m.X(3+4)  + m.X(0+4) );
	planes[0].n.z = ( m.X(3+8)  + m.X(0+8) );
	planes[0].d   = ( m.X(3+12) + m.X(0+12) );
	planes[0].Normalize();

	// right
	planes[1].n.x = ( m.X(3+0)  - m.X(0+0) );
	planes[1].n.y = ( m.X(3+4)  - m.X(0+4) );
	planes[1].n.z = ( m.X(3+8)  - m.X(0+8) );
	planes[1].d   = ( m.X(3+12) - m.X(0+12) );
	planes[1].Normalize();

	// bottom
	planes[2].n.x = ( m.X(3+0)  + m.X(1+0) );
	planes[2].n.y = ( m.X(3+4)  + m.X(1+4) );
	planes[2].n.z = ( m.X(3+8)  + m.X(1+8) );
	planes[2].d   = ( m.X(3+12) + m.X(1+12) );
	planes[2].Normalize();

	// top
	planes[3].n.x = ( m.X(3+0)  - m.X(1+0) );
	planes[3].n.y = ( m.X(3+4)  - m.X(1+4) );
	planes[3].n.z = ( m.X(3+8)  - m.X(1+8) );
	planes[3].d   = ( m.X(3+12) - m.X(1+12) );
	planes[3].Normalize();

	// near
	planes[4].n.x = ( m.X(3+0)  + m.X(2+0) );
	planes[4].n.y = ( m.X(3+4)  + m.X(2+4) );
	planes[4].n.z = ( m.X(3+8)  + m.X(2+8) );
	planes[4].d   = ( m.X(3+12) + m.X(2+12) );
	planes[4].Normalize();

	// far
	planes[5].n.x = ( m.X(3+0)  - m.X(2+0) );
	planes[5].n.y = ( m.X(3+4)  - m.X(2+4) );
	planes[5].n.z = ( m.X(3+8)  - m.X(2+8) );
	planes[5].d   = ( m.X(3+12) - m.X(2+12) );
	planes[5].Normalize();

	GLASSERT( DotProduct( planes[PLANE_LEFT].n, planes[PLANE_RIGHT].n ) < 0.0f );
	GLASSERT( DotProduct( planes[PLANE_TOP].n, planes[PLANE_BOTTOM].n ) < 0.0f );
	GLASSERT( DotProduct( planes[PLANE_NEAR].n, planes[PLANE_FAR].n ) < 0.0f );
}


Model* Engine::IntersectModel( const Vector3F& origin, const Vector3F& dir, float length,
							   HitTestMethod method, 
							   int required, int exclude, const Model * const * ignore, Vector3F* intersection )
{
	//GRINLIZ_PERFTRACK

	Model* model = spaceTree->QueryRay(	origin, dir, length,
										required, exclude, ignore,
										method,
										intersection );
	return model;
}


ModelVoxel Engine::IntersectModelVoxel( const Vector3F& origin,
										const Vector3F& _dir,
										float length, 
										HitTestMethod testMethod,
										int required, int exclude, const Model* const * ignore )
{
	ModelVoxel modelVoxel;
	Vector3F dir = _dir;
	dir.Normalize();

	// Check voxel first; intersections limit the length.
	if ( map ) {
		modelVoxel.voxel = map->IntersectVoxel( origin, dir, length, &modelVoxel.at );
		if ( modelVoxel.voxel.x >= 0 ) {
			// We did hit a voxel.
			float voxelLength = ( origin - modelVoxel.at ).Length();
			length = Min( length, voxelLength );	// don't need to check past the voxel.
		}
	}
	Model* m = IntersectModel( origin, dir, length, testMethod, required, exclude, ignore, &modelVoxel.at );
	if ( m ) {
		m->AABB();	// does nothing except check for valid memory pointer
		// Clear the voxel! can only have one state.
		modelVoxel.model = m;
		modelVoxel.voxel.Set( -1, -1, -1 );
	}
	return modelVoxel;
}


void Engine::RestrictCamera( const grinliz::Rectangle2F* bounds )
{
	const Vector3F* eyeDir = camera.EyeDir3();

	Vector3F intersect;
	IntersectRayPlane( camera.PosWC(), eyeDir[0], XZ_PLANE, 0.0f, &intersect );
//	GLOUTPUT(( "Intersect %.1f, %.1f, %.1f\n", intersect.x, intersect.y, intersect.z ));

	Rectangle2F b;
	b.Set( 0, 0, (float)map->Width(), (float)map->Height() );
	if ( bounds ) {
		b = *bounds;
	}

	if ( intersect.x < b.min.x ) {
		camera.DeltaPosWC( (b.min.x - intersect.x), 0.0f, 0.0f );
	}
	if ( intersect.x > b.max.x ) {
		camera.DeltaPosWC( (b.max.x - intersect.x), 0.0f, 0.0f );
	}
	if ( intersect.z < b.min.y ) {
		camera.DeltaPosWC( 0.0f, 0.0f, ( b.min.y - intersect.z) );
	}
	if ( intersect.z > b.max.y ) {
		camera.DeltaPosWC( 0.0f, 0.0f, ( b.max.y - intersect.z) );
	}
}



void Engine::SetZoom( float z )
{
	z = Clamp( z, GAME_ZOOM_MIN, GAME_ZOOM_MAX );
	float d = Interpolate(	GAME_ZOOM_MIN, EL_CAMERA_MIN,
							GAME_ZOOM_MAX, EL_CAMERA_MAX,
							z );
	//GLOUTPUT(("Zoom %.1f\n", z));

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


Texture* Engine::GetRenderTargetTexture( int i ) 
{ 
	GLASSERT( i >= 0 && i < RT_COUNT );
	return renderTarget[i] ? renderTarget[i]->GetTexture() : 0; 
}


void Engine::LoadConfigFiles( const char* particleName, const char* lightName )
{
	particleSystem->LoadParticleDefs( particleName );

	tinyxml2::XMLDocument doc;
	doc.LoadFile( lightName );
	GLASSERT( !doc.Error() );
	lighting.Load( doc.FirstChildElement( "lighting" ));

	spaceTree->SetLightDir( lighting.direction );
}


struct DLine {
	Vector3F tail;
	Vector3F head;
	Vector3F color;
	int		 time;
};

grinliz::CArray<DLine, 100> debugLines;

void DebugLine(	const grinliz::Vector3F& tail, 
						const grinliz::Vector3F& head,
						float r, float g, float b, 
						U32 time )
{
	if ( debugLines.HasCap() ) {
		DLine* dl = debugLines.PushArr(1);
		dl->tail = tail;
		dl->head = head;
		dl->color.Set( r, g, b );
		dl->time = time;
	}
}


void DrawDebugLines( U32 delta )
{
	FlatShader flatShader;
	int i=0;
	while( i<debugLines.Size() ) {
		DLine* dl = &debugLines[i];
		if ( dl->time > 0 ) {
			dl->time -= delta;
			flatShader.SetColor( dl->color.x, dl->color.y, dl->color.z );
			//GPUDevice::Instance()->DrawLine( flatShader, dl->tail, dl->head );
			++i;
		}
		else {
			debugLines.SwapRemove(i);
		}
	}
}
