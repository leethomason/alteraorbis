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

#ifndef UFOATTACK_ENGINE_INCLUDED
#define UFOATTACK_ENGINE_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glbitarray.h"

#include "modelvoxel.h"
#include "map.h"
#include "camera.h"
#include "enginelimits.h"
#include "model.h"
#include "screenport.h"
#include "lighting.h"
#include "shadermanager.h"
#include "uirendering.h"

#include "../gamui/gamui.h"

#include "../xegame/stackedsingleton.h"

class RenderQueue;
class RenderTarget;
class ParticleSystem;
class EngineShaders;

struct Bolt;
class BoltRenderer;
class Model;

static const grinliz::Vector3F V3F_UP   = { 0, 1, 0 };
static const grinliz::Vector3F V3F_DOWN = { 0, -1, 0 };
static const grinliz::Vector3F V3F_OUT	= { 0, 0, 1 };

void DebugLine(	const grinliz::Vector3F& tail, 
				const grinliz::Vector3F& head,
				float r=1, float g=1, float b=1, 
				U32 time=1000 );

void DrawDebugLines( U32 delta );

class IUITracker
{
public:
	virtual void UpdateUIElements(const Model* model[], int nModels) = 0;
};

/*
	Standard state:
	Z-Write		enabled
	Z-Test		enabled
	Blend		disabled
*/

/*
	Renderer

	Assets
		- alpha: transparency or emmissive

	Renderer
		- shadows, glow, colors

		Shadows
			- shadow casting models rendered to stencil buffer.
			- background plane can be multi-pass, checked against stencil		
*/

class Engine :	public IDeviceLossHandler,
				public StackedSingleton< Engine >
{
public:
	Engine( Screenport* screenport, const gamedb::Reader* database, Map* map );
	virtual ~Engine();

	Camera			camera;
	Lighting		lighting;
	ParticleSystem*	particleSystem;

	// Send everything to the GPU
	void Draw( U32 deltaTime, const Bolt* bolts=0, int nBolts=0, IUITracker* tracker=0 );

	enum {
		GLOW_BLACK,
		GLOW_EMISSIVE,
		SHADOW,
		MODELS,
		NUM_MODEL_DRAW_CALLS
	};
	int modelDrawCalls[NUM_MODEL_DRAW_CALLS];

	SpaceTree* GetSpaceTree()	{ return spaceTree; }

	void MoveCameraHome();
	void CameraIso(  bool normal, bool sizeToWidth, float width, float height );
	// Move the camera so that it points to x,z. If 'calc' is non-null,
	// the camera will *not* be moved, but the destination for the camera
	// is returned.
	void MoveCameraXZ( float x, float z, grinliz::Vector3F* calc=0 );

	// What the camera is looking at on the y=0 plane.
	void CameraLookingAt( grinliz::Vector3F* at );
	void CameraLookAt( float x, float z, float heightOfCamera, float yRotation=-45.0f, float tilt=-50.0f );
	void CameraLookAt( const grinliz::Vector3F& camera, const grinliz::Vector3F& target );
	void CameraLookAt( float x, float z );

	Model* AllocModel( const ModelResource* );
	Model* AllocModel( const char* );
	void FreeModel(Model*);

	Map* GetMap()						{ return map; }
	Texture* GetMiniMapTexture();

	const RenderQueue* GetRenderQueue()	{ return renderQueue; }

	bool RayFromViewToYPlane( const grinliz::Vector2F& view,
							  const grinliz::Matrix4& modelViewProjectionInverse, 
							  grinliz::Ray* ray,
							  grinliz::Vector3F* intersection );

	Model* IntersectModel(	const grinliz::Vector3F& origin,
							const grinliz::Vector3F& dir,
							float length,							// FLT_MAX 
							HitTestMethod testMethod,
							int required, int exclude, const Model* const * ignore,
							grinliz::Vector3F* intersection );

	ModelVoxel IntersectModelVoxel( const grinliz::Vector3F& origin,
									const grinliz::Vector3F& dir,
									float length, 
									HitTestMethod testMethod,
									int required, int exclude, const Model* const * ignore );

	enum {
		NEAR,
		FAR,
		LEFT,
		RIGHT,
		BOTTOM,
		TOP
	};
	void CalcFrustumPlanes( grinliz::Plane* planes );

	float GetZoom();
	void SetZoom( float zoom );

	void SetScreenport( Screenport* port )	{ screenport = port; }
	const Screenport& GetScreenport()		{ return *screenport; }
	Screenport* GetScreenportMutable()		{ return screenport; }
	void RestrictCamera( const grinliz::Rectangle2F* bounds = 0 );

	Texture* GetRenderTargetTexture( int id=0 );

	enum {
		STAGE_PARTICLE		= 0x01,
		STAGE_SHADOW		= 0x02,
		STAGE_GLOW			= 0x04,
		STAGE_VOXEL			= 0x08,
		STAGE_BOLT			= 0x10,
	};

	void SetStages( int s ) { stages = s; }
	int  Stages() const		{ return stages; }

	void SetTemperature( float temp ) { temperature = temp; }

	virtual void DeviceLoss();

	void LoadConfigFiles( const char* particle, const char* lighting );

	enum {
		RT_LIGHTS,
		RT_BLUR_0,
		RT_BLUR_1,
		RT_BLUR_2,
		RT_BLUR_3,
		RT_BLUR_4,
		RT_COUNT,
		BLUR_COUNT = 5
	};

	gamui::Gamui overlay;

private:
	enum ShadowState {
		IN_SHADOW,
		OPEN_LIGHT
	};

	void CalcCameraRotation( grinliz::Matrix4* );

	void Blur();
	void QueueSet(	EngineShaders* engineShaders, Model* root, 
					int requiredModelFlag,  int excludedModelFlag,
					int requiredShaderFlag, int excludedShaderFlag );

	Screenport* screenport;
	float	initZoom;
	int		initZoomDistance;
	int		stages;
	float	temperature;
	int		frame;
	
	Map*			map;
	SpaceTree*		spaceTree;
	RenderQueue*	renderQueue;
	BoltRenderer*	boltRenderer;

	RenderTarget* renderTarget[RT_COUNT];

	grinliz::CDynArray<const Model*> trackedModels;
	UIRenderer uiRenderer;
};

#endif // UFOATTACK_ENGINE_INCLUDED