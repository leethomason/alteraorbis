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

using namespace grinliz;

Screenport::Screenport( int w, int h, int virtualHeight )
{
//	this->virtualHeight = (float)virtualHeight;
	this->near = EL_NEAR;
	this->far  = EL_FAR;
	Resize( w, h );
	uiMode = false;
	orthoCamera = false;
}


void Screenport::Resize( int w, int h )
{
	if (w > 0 && h > 0) {
		physicalWidth = (float)w;
		physicalHeight = (float)h;
	}
	else {
		w = (int)physicalWidth;
		h = (int)physicalHeight;
	}

	GPUDevice::Instance()->SetViewport(w, h);
}


void Screenport::SetUI()	
{
	projection2D.SetIdentity();
	projection2D.SetOrtho(0, physicalWidth, physicalHeight, 0, -1, 1);
	GPUDevice::Instance()->SetOrthoTransform((int)physicalWidth, (int)physicalHeight);
	uiMode = true;
}


void Screenport::SetView( const Matrix4& _view )
{
	GLASSERT( uiMode == false );
	view3D = _view;
	GPUDevice::Instance()->SetCameraTransform( view3D );
}


void Screenport::SetPerspective()
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
		GPUDevice::Instance()->SetPerspectiveTransform( projection3D );
	}
	else {
		projection3D.SetFrustum( frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.zNear, frustum.zFar );
		GPUDevice::Instance()->SetPerspectiveTransform( projection3D );
	}
}


void Screenport::ViewToUI( const grinliz::Vector2F& view, grinliz::Vector2F* ui ) const
{
	GLASSERT(0);	// not correct
	ui->x = view.x;
	ui->y = physicalHeight-view.y;
}


void Screenport::UIToView( const grinliz::Vector2F& ui, grinliz::Vector2F* view ) const
{
	GLASSERT(0);	// not correct
	view->x = ui.x;
	view->y = physicalHeight-ui.y;
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

	Vector2F v0, v1;
	Rectangle2F clipInView;
	Vector2F min = { 0, 0 };
	Vector2F max = { physicalWidth, physicalHeight };
	UIToView( min, &v0 );
	UIToView( max, &v1 );

	clipInView.FromPair( v0.x, v0.y, v1.x, v1.y );
	
	v->x = Interpolate( -1.0f, (float)clipInView.min.x,
						1.0f,  (float)clipInView.max.x,
						r.x/r.w );
	v->y = Interpolate( -1.0f, (float)clipInView.min.y,
						1.0f,  (float)clipInView.max.y,
						r.y/r.w );
}


void Screenport::ViewToWindow( const Vector2F& view, Vector2F* window ) const
{
	GLASSERT(0);	// not correct
	window->x = view.x * physicalWidth;	// / screenWidth;
	window->y = view.y * physicalHeight;	// / screenHeight;
}


void Screenport::WindowToView( const Vector2F& window, Vector2F* view ) const
{
	GLASSERT(0);	// not correct
	view->x = window.x;	// *screenWidth / physicalWidth;
	view->y = window.y; // *screenHeight / physicalHeight;
}


void Screenport::UIToWindow( const grinliz::Rectangle2F& ui, grinliz::Rectangle2F* clip ) const
{	
	Vector2F v;
	Vector2F w;
	
	UIToView( ui.min, &v );
	ViewToWindow( v, &w );
	clip->min = clip->max = w;

	UIToView( ui.max, &v );
	ViewToWindow( v, &w );
	clip->DoUnion( w );
}


void Screenport::CleanScissor( const grinliz::Rectangle2F& scissor, grinliz::Rectangle2I* clean )
{
	if ( scissor.min.x == 0 && scissor.min.y == 0 && scissor.max.x == physicalWidth && scissor.max.y == physicalHeight ) {
		clean->min.Set( 0, 0 );
		clean->max.Set( (int)physicalWidth, (int)physicalHeight );
		return;
	}

	clean->min.x = (int)scissor.min.x;
	clean->max.x = (int)scissor.max.x;
	if ( abs( clean->max.x - physicalWidth ) < 4 ) {
		clean->max.x = (int)physicalWidth;
	}
	clean->min.y = (int)scissor.min.y;
	clean->max.y = (int)scissor.max.y;
	if ( abs( clean->max.y - physicalHeight ) < 4 ) {
		clean->max.y = (int)physicalHeight;
	}
}
