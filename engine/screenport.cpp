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

#include "screenport.h"
#include "gpustatemanager.h"
#include "enginelimits.h"

#include "../grinliz/glrectangle.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glmatrix.h"

#include "../gamui/gamui.h"

using namespace grinliz;

Screenport::Screenport(int w, int h)
{
	this->near = EL_NEAR;
	this->far = EL_FAR;
	physicalWidth = float(w);
	physicalHeight = float(h);
	uiMode = false;
	orthoCamera = false;
}


void Screenport::Resize(int w, int h, GPUDevice* device)
{
	if (w > 0 && h > 0) {
		physicalWidth = (float)w;
		physicalHeight = (float)h;
	}
	else {
		w = (int)physicalWidth;
		h = (int)physicalHeight;
	}

	device->SetViewport(w, h);
}


void Screenport::SetUI(GPUDevice* device)
{
	projection2D.SetIdentity();
	projection2D.SetOrtho(0, physicalWidth, physicalHeight, 0, -1, 1);
	device->SetOrthoTransform((int)physicalWidth, (int)physicalHeight);
	uiMode = true;
}


void Screenport::SetView(const Matrix4& _view, GPUDevice* device)
{
	GLASSERT(uiMode == false);
	view3D = _view;
	device->SetCameraTransform(view3D);
}


void Screenport::SetPerspective(GPUDevice* device)
{
	uiMode = false;

	GLASSERT( uiMode == false );
	GLASSERT( near > 0.0f );
	GLASSERT( far > near );

	frustum.zNear = near;
	frustum.zFar  = far;

	// Convert from the FOV to the half angle.
	float theta = ToRadian( EL_FOV * 0.5f );
	float tanTheta = tanf( theta );
	float halfLongSide = tanTheta * frustum.zNear;

	// left, right, top, & bottom are on the near clipping
	// plane. (Not an obvious point to my mind.)

	// Also, the 3D camera applies the rotation.
	
	// Since FOV is specified as the 1/2 width, the ratio
	// is the height/width (different than gluPerspective)
	float ratio = physicalHeight / physicalWidth;

	frustum.top		= ratio * halfLongSide;
	frustum.bottom	= -frustum.top;

	frustum.left	= -halfLongSide;
	frustum.right	=  halfLongSide;
	
	if ( orthoCamera ) {
		float w = 0;
		float h = 0;
		if ( orthoWidth > 0 ) {
			w = orthoWidth;
			h = orthoWidth * ratio;
		}
		else {
			w = orthoHeight / ratio;
			h = orthoHeight;
		}
		projection3D.SetOrtho( -w/2, w/2, -h/2, h/2, frustum.zNear, frustum.zFar );
		device->SetPerspectiveTransform( projection3D );
	}
	else {
		projection3D.SetFrustum( frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.zNear, frustum.zFar );
		device->SetPerspectiveTransform( projection3D );
	}
}


grinliz::Vector2F Screenport::WorldToUI(const grinliz::Vector3F& world, const gamui::Gamui& g) const
{
	Vector2F view = WorldToView(world);
	Vector2F window = ViewToWindow(view);
	Vector2F ui = { 0, 0 };
	ui.x = g.TransformPhysicalToVirtual(window.x);
	ui.y = g.TransformPhysicalToVirtual(window.y);
	return ui;
}


bool Screenport::ViewToWorld( const grinliz::Vector2F& v, const grinliz::Matrix4* _mvpi, grinliz::Ray* ray ) const
{
	Matrix4 mvpi;
	if ( _mvpi ) {
		mvpi = *_mvpi;
	}
	else {
		Matrix4 mvp;
		ViewProjection3D( &mvp );
		mvp.Invert( &mvpi );
	}

	// View normalized:
	Vector4F in = { 2.0f * v.x / physicalWidth - 1.0f,
					2.0f * v.y / physicalHeight - 1.0f,
					0.f, //v.z*2.0f-1.f,
					1.0f };

	Vector4F out0, out1;
	MultMatrix4( mvpi, in, &out0 );
	in.z = 1.0f;
	MultMatrix4( mvpi, in, &out1 );

	if ( out0.w == 0.0f ) {
		return false;
	}
	if ( ray ) {
		ray->origin.x = out0.x / out0.w;
		ray->origin.y = out0.y / out0.w;
		ray->origin.z = out0.z / out0.w;

		ray->direction.x = out1.x / out1.w - ray->origin.x;
		ray->direction.y = out1.y / out1.w - ray->origin.y;
		ray->direction.z = out1.z / out1.w - ray->origin.z;
	}
	return true;
}


void Screenport::WorldToView( const grinliz::Vector3F& world, grinliz::Vector2F* v ) const
{
	Matrix4 mvp;
	ViewProjection3D( &mvp );

	Vector4F p, r;
	p.Set( world, 1 );
	r = mvp * p;

	v->x = Interpolate( -1.0f, 0.0f,
						1.0f,  (float)physicalWidth,
						r.x/r.w );
	v->y = Interpolate( -1.0f, 0.0f,
						1.0f,  (float)physicalHeight,
						r.y/r.w );
}

