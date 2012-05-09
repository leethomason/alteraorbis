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

#include "map.h"
#include "camera.h"
#include "enginelimits.h"
#include "model.h"
#include "screenport.h"
#include "lighting.h"
#include "shadermanager.h"

class RenderQueue;
class RenderTarget;
class ParticleSystem;
class EngineShaders;

/*
	Standard state:
	Z-Write		enabled
	Z-Test		enabled
	Blend		disabled
*/

/*
	Renderer

	Assets
		- TBD: texture atlasing in builder. (Atlased textures can't be repeated.)
		- alpha: transparency or emmissive

	Renderer
		- shadows, glow, colors

		Shadows
			- shadow casting models rendered to stencil buffer.
			- background plane can be multi-pass, checked against stencil

		
*/

class Engine :	public IDeviceLossHandler
{
public:
	Engine( Screenport* screenport, const gamedb::Reader* database, Map* map );
	virtual ~Engine();

	Camera			camera;
	Lighting		lighting;
	ParticleSystem*	particleSystem;

	// Send everything to the GPU
	void Draw( U32 deltaTime );

	SpaceTree* GetSpaceTree()	{ return spaceTree; }

	void MoveCameraHome();
	void CameraIso(  bool normal, bool sizeToWidth, float width, float height );
	// Move the camera so that it points to x,z. If 'calc' is non-null,
	// the camera will *not* be moved, but the destination for the camera
	// is returned.
	void MoveCameraXZ( float x, float z, grinliz::Vector3F* calc=0 );

	void CameraLookingAt( grinliz::Vector3F* at );
	void CameraLookAt( float x, float z, float heightOfCamera, float yRotation=-45.0f, float tilt=-50.0f );

	Model* AllocModel( const ModelResource* );
	void FreeModel( Model* );

	Map* GetMap()						{ return map; }
	Texture* GetMiniMapTexture();

	const RenderQueue* GetRenderQueue()	{ return renderQueue; }

	bool RayFromViewToYPlane( const grinliz::Vector2F& view,
							  const grinliz::Matrix4& modelViewProjectionInverse, 
							  grinliz::Ray* ray,
							  grinliz::Vector3F* intersection );

	Model* IntersectModel(	const grinliz::Ray& ray, 
							HitTestMethod testMethod,
							int required, int exclude, const Model* ignore[],
							grinliz::Vector3F* intersection );

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

	void SetScreenport( Screenport* port ) { screenport = port; }
	const Screenport& GetScreenport() { return *screenport; }
	Screenport* GetScreenportMutable() { return screenport; }
	void RestrictCamera();

	Texture* GetRenderTargetTexture( int id=0 );
	void SetGlow( bool b ) { glow = b; }
	bool GetGlow() const { return glow; }

	virtual void DeviceLoss();

private:
	enum ShadowState {
		IN_SHADOW,
		OPEN_LIGHT
	};

	void CalcCameraRotation( grinliz::Matrix4* );

	void Blur();
//	void PushShadowSwizzleMatrix( GPUShader* );
//	void PushLightSwizzleMatrix( GPUShader* );
	void CreateMiniMap();

	Screenport* screenport;
	float	initZoom;
	int		initZoomDistance;
	bool	glow;
	
	Map*	map;
	SpaceTree* spaceTree;
	RenderQueue* renderQueue;

	enum {
		RT_LIGHTS,
		RT_BLUR_X,
		RT_BLUR_Y,
		RT_COUNT
	};
	RenderTarget* renderTarget[RT_COUNT];
	RenderTarget* miniMapRenderTarget;

	EngineShaders* engineShaders;
};

#endif // UFOATTACK_ENGINE_INCLUDED