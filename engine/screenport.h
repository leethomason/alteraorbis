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

#ifndef UFOATTACK_SCREENPORT_INCLUDED
#define UFOATTACK_SCREENPORT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glgeometry.h"

namespace gamui { class Gamui; };

struct Frustum
{
	float left, right, bottom, top, zNear, zFar;
};

/*
	Window:	pixel coordinates, origin upper left.
	View: pixel coordinate, origin lower left
	ViewNormalized: (-1,-1) - (1,1) normalized pixels, origin lower left (not generally exposed)
*/
class Screenport
{
public:
	Screenport( int width, int height, int virtualHeight ); 

	// Needs to be set before engine is set up
	void SetNearFar( float n, float f ) { near = n; far = f; GLASSERT( far > near ); }

	void Resize( int w, int h );

	bool ViewToWorld( const grinliz::Vector2F& view, const grinliz::Matrix4* mvpi, grinliz::Ray* world ) const;
	void WorldToView( const grinliz::Vector3F& world, grinliz::Vector2F* view ) const;
	grinliz::Vector2F WorldToView(const grinliz::Vector3F& world) const {
		grinliz::Vector2F view = { 0, 0 };
		WorldToView(world, &view);
		return view;
	}

	grinliz::Vector2F WorldToUI(const grinliz::Vector3F& world, const gamui::Gamui& g) const;

	void ViewToWindow(const grinliz::Vector2F& view, grinliz::Vector2F* window) const {
		window->x = view.x;
		window->y = float(physicalHeight) - view.y;
	}
	void WindowToView(const grinliz::Vector2F& window, grinliz::Vector2F* view) const {
		view->x = window.x;
		view->y = float(physicalHeight) - window.y;
	}

	// UI: origin in lower left, oriented with device.
	// Sets both the MODELVIEW and the PROJECTION for UI coordinates. (The view is not set.)
	// The clip is interpreted as the location where the UI can be.
	// FIXME: render to texture uses weird coordinates. Once clip is cleaned up (or removed)
	// it would be nice to be able to have a lower left origin.
	void SetUI();

	// Set the perspective PROJECTION.
	void SetPerspective();
	float Near() const { return near; }
	float Far() const { return far; }

	// Uses an ortho camera in perspective mode. Either the width or height should be
	// 0, and the other will be calculated.
	void SetOrthoCamera( bool ortho, float w, float h )				{ orthoCamera = ortho; orthoWidth = w; orthoHeight = h; }

	// Set the MODELVIEW from the camera.
	void SetView( const grinliz::Matrix4& view );
	void SetViewMatrices( const grinliz::Matrix4& _view )			{ view3D = _view; }

	const grinliz::Matrix4& ProjectionMatrix3D() const				{ return projection3D; }
	const grinliz::Matrix4& ViewMatrix3D() const					{ return view3D; }

	void ViewProjection3D( grinliz::Matrix4* vp ) const				{ grinliz::MultMatrix4( projection3D, view3D, vp ); }
	void ViewProjectionInverse3D( grinliz::Matrix4* vpi ) const		{ grinliz::Matrix4 vp;
																	  ViewProjection3D( &vp );
																	  vp.Invert( vpi );
																	}

	const Frustum&	GetFrustum()		{ GLASSERT( uiMode == false ); return frustum; }

	//float UIWidth() const									{ return screenWidth; }
	//float UIHeight() const									{ return screenHeight; }
	int PhysicalWidth() const								{ return (int)physicalWidth; }
	int PhysicalHeight() const								{ return (int)physicalHeight; }

//	const grinliz::Rectangle2F UIBoundsClipped3D() const	{ grinliz::Rectangle2F r; r.Set( 0,0,UIWidth(),UIHeight() ); return r; }
//	const grinliz::Rectangle2F UIBoundsClipped2D() const	{ grinliz::Rectangle2F r; r.Set( 0,0,UIWidth(),UIHeight() ); return r; }

	bool UIMode() const										{ return uiMode; }

private:
	//void UIToWindow( const grinliz::Rectangle2F& ui, grinliz::Rectangle2F* clip ) const;
	void CleanScissor( const grinliz::Rectangle2F& scissor, grinliz::Rectangle2I* clean );

	//float screenWidth; 
	//float screenHeight;		// if rotation==0, 320
	//float virtualHeight;	// used to be 320. Used for UI layout.
	float near;
	float far;

	float physicalWidth;
	float physicalHeight;

	bool uiMode;
	bool orthoCamera;
	float orthoWidth;
	float orthoHeight;
	Frustum frustum;
	grinliz::Matrix4 projection2D;
	grinliz::Matrix4 projection3D, view3D;
};

#endif	// UFOATTACK_SCREENPORT_INCLUDED