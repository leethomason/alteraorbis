/*
 Copyright (c) 2010 Lee Thomason

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include "gamui.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

using namespace gamui;
using namespace std;

static const float PI = 3.1415926535897932384626433832795f;

RenderAtom Gamui::m_nullAtom;

void Matrix::SetXRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	x[5] = cosTheta;
	x[6] = sinTheta;

	x[9] = -sinTheta;
	x[10] = cosTheta;
}


void Matrix::SetYRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = 0.0f;
	x[2] = -sinTheta;
	
	// COLUMN 2
	x[4] = 0.0f;
	x[5] = 1.0f;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = sinTheta;
	x[9] = 0;
	x[10] = cosTheta;
}

void Matrix::SetZRotation( float theta )
{
	float cosTheta = cosf( theta*PI/180.0f );
	float sinTheta = sinf( theta*PI/180.0f );

	// COLUMN 1
	x[0] = cosTheta;
	x[1] = sinTheta;
	x[2] = 0.0f;
	
	// COLUMN 2
	x[4] = -sinTheta;
	x[5] = cosTheta;
	x[6] = 0.0f;

	// COLUMN 3
	x[8] = 0.0f;
	x[9] = 0.0f;
	x[10] = 1.0f;
}



UIItem::UIItem( int p_level ) 
	: userData(0),
	  m_x( 0 ),
	  m_y( 0 ),
	  m_level( p_level ),
	  m_visible( true ),
	  m_rotationX( 0 ),
	  m_rotationY( 0 ),
	  m_rotationZ( 0 ),
	  m_dialogID(0),
	  m_gamui( 0 ),
	  m_enabled( true ),
	  m_superItem( 0 )
{}


UIItem::~UIItem()
{
	if ( m_superItem ) {
		m_superItem->RemoveSubItem( this );
	}
	if ( m_gamui )
		m_gamui->Remove( this );
}


Gamui::Vertex* UIItem::PushQuad( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf )
{
	int base = vertexBuf->Size();
	uint16_t *index = indexBuf->PushArr( 6 );

	index[0] = base+0;
	index[1] = base+1;
	index[2] = base+2;
	index[3] = base+0;
	index[4] = base+2;
	index[5] = base+3;

	return vertexBuf->PushArr( 4 );
}


void UIItem::ApplyRotation( int nVertex, Gamui::Vertex* vertex )
{
	if ( m_rotationX != 0.0f || m_rotationY != 0.0f || m_rotationZ != 0.0f ) {
		Matrix m, mx, my, mz, t0, t1, temp;

		t0.SetTranslation( -(X()+Width()*0.5f), -(Y()+Height()*0.5f), 0 );
		t1.SetTranslation( (X()+Width()*0.5f), (Y()+Height()*0.5f), 0 );
		mx.SetXRotation( m_rotationX );
		my.SetYRotation( m_rotationY );
		mz.SetZRotation( m_rotationZ );

		m = t1 * mz * my * mx * t0;

		for( int i=0; i<nVertex; ++i ) {
			float in[3] = { vertex[i].x, vertex[i].y, 0 };
			MultMatrix( m, in, 2, &vertex[i].x );
		}
	}
}


TextLabel::TextLabel() : UIItem( Gamui::LEVEL_TEXT ),
	m_width( -1 ),
	m_height( -1 )
{
	m_str = m_buf;
	m_str[0] = 0;
	m_allocated = ALLOCATED;
	m_boundsWidth = 0;
	m_boundsHeight = 0;
	m_tabWidth = 0;
}


TextLabel::~TextLabel()
{
	if ( m_str != m_buf )
		delete [] m_str;
}


void TextLabel::Init( Gamui* gamui )
{
	m_gamui = gamui;
	SetDialogID(gamui->CurrentDialogID());
	m_gamui->Add( this );
}


void TextLabel::ClearText()
{
	if ( m_str[0] ) {
		m_str[0] = 0;
		m_width = m_height = -1;
		Modify();
	}
}


const RenderAtom& TextLabel::GetRenderAtom() const
{
	GAMUIASSERT( m_gamui );
	return Enabled() ? m_gamui->GetTextAtom() : m_gamui->GetDisabledTextAtom();
}


void TextLabel::SetBounds( float width, float height )	
{ 
	if ( m_boundsWidth != width || m_boundsHeight != height ) { 
		m_boundsWidth = width; 
		m_boundsHeight = height;
		m_width = m_height = -1; 
		Modify(); 
	}
}



const char* TextLabel::GetText() const
{
	return m_str;
}


void TextLabel::SetText( const char* t )
{
	if ( t ) 
		SetText( t, t+strlen(t) );
	else
		SetText( "" );
}


void TextLabel::SetText( const char* start, const char* end )
{
	if (    memcmp( start, m_str, end-start ) == 0	// contain the same thing
		 && m_str
		 && m_str[end-start] == 0 )					// have the same length
	{
		// They are the same.
	}
	else {
		m_width = m_height = -1;
		int len = end - start;
		int allocatedNeeded = len+1;

		if ( m_allocated < allocatedNeeded ) {
			m_allocated = (allocatedNeeded+ALLOCATED)*3/2;
			if ( m_str != m_buf )
				delete m_str;
			m_str = new char[m_allocated];
		}
		memcpy( m_str, start, len );
		m_str[len] = 0;
		Modify();
	}
}


bool TextLabel::DoLayout() 
{
	return m_str[0] != 0;
}


float TextLabel::WordWidth( const char* p, IGamuiText* iText ) const
{
	float w = 0;
	IGamuiText::GlyphMetrics metrics;
	IGamuiText::FontMetrics font;

	iText->GamuiFont(&font);

	char prev = ' ';

	while( *p && !isspace( *p )) {
		iText->GamuiGlyph( *p, prev, &metrics );
		w += metrics.advance;
		prev = *p;
		++p;
	}
	return w;
}


void TextLabel::Queue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf ) 
{
	ConstQueue( indexBuf, vertexBuf );
}


void TextLabel::ConstQueue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf ) const
{
	if ( !m_gamui )
		return;

	/*	This routine gets done in physical pixels, not virtual.
		Hence all the Transform() calls to convert from virtual to physical.
	*/

	IGamuiText* iText = m_gamui->GetTextInterface();
	if (!iText) return;
	IGamuiText::FontMetrics font;
	iText->GamuiFont(&font);

	const char* p = m_str;
	const float X0 = floorf(m_gamui->TransformVirtualToPhysical(X()));
	const float Y0 = floorf(m_gamui->TransformVirtualToPhysical(Y()));
	float x = X0;
	float y = Y0 + float(font.ascent);	// move to the baseline
	const float tabWidth = m_gamui->TransformVirtualToPhysical(m_tabWidth);
	const float boundsWidth = m_gamui->TransformVirtualToPhysical(m_boundsWidth);
	const float boundsHeight = m_gamui->TransformVirtualToPhysical(m_boundsHeight);

	float xmax = x;
	IGamuiText::GlyphMetrics metrics;
	const float height = float(font.linespace);
	int tab = 0;

	while ( p && *p ) {
		// Text layout. Written this algorithm so many times, and it's
		// always tricky. This is at least a very simple version.

		// Respect line breaks, even if we aren't in multi-line mode.
		while ( *p == '\n' ) {
			y += height;
			tab = 0;
			x = X0;
			++p;
		}
		if ( !*p ) break;

		// Tabs are implemented as tables.
		if ( *p == '\t' ) {
			++p;
			if ( !*p ) break;

			if ( tabWidth > 0 ) {
				++tab;
				x = X0 + float(tab)*tabWidth;
			}
		}

		// Throw away space after a line break.
		if ( boundsWidth && ( x == X0 && y > Y0 )) {
			while ( *p && *p == ' ' ) {
				++p;
			}
			if ( !*p || *p == '\n' ) {
				continue;	// we need to hit the previous case..
			}
		}

		// If we aren't the first word, do we need to break?
		if ( boundsWidth && (x > X0) && (*p != ' ') ) {
			float w = WordWidth( p, iText );
			if ( x + w > X0 + boundsWidth ) { 
				y += height;
				x = X0;
				tab = 0;
				continue;
			}
		}

		// And finally, have we exceeded the y bound?
		if ( boundsHeight && ( y + font.descent >= Y0 + boundsHeight) ) {
			break;
		}

		// Everything above is about a word; now we are
		// committed and can run in a tight loop.
		y = floorf(y);
		while( p && *p && *p != '\n' && *p != '\t' ) {
			x = floorf(x);

			iText->GamuiGlyph( *p, p>m_str ? *(p-1):0, &metrics );

			if ( vertexBuf ) {
				Gamui::Vertex* vertex = PushQuad( indexBuf, vertexBuf );
		
				float x0 = x + (metrics.x);
				float x1 = x + (metrics.x + metrics.w);
				float y0 = y + (metrics.y);
				float y1 = y + (metrics.y + metrics.h);

				vertex[0].Set( x0, y0, metrics.tx0, metrics.ty0 );
				vertex[1].Set( x0, y1, metrics.tx0, metrics.ty1 );
				vertex[2].Set( x1, y1, metrics.tx1, metrics.ty1 );
				vertex[3].Set( x1, y0, metrics.tx1, metrics.ty0 );
			}
			int done = isspace( *p );

			++p;
			x += metrics.advance;
			xmax = x > xmax ? x : xmax;

			// We processed a space; leave the (inner) word loop.
			if ( done )
				break;
		}
	}

	m_width  = m_gamui->TransformPhysicalToVirtual(xmax - X0);
	m_height = m_gamui->TransformPhysicalToVirtual(y + height - (Y0 + float(font.ascent)));
}


float TextLabel::Width() const
{
	if ( !m_gamui )
		return 0;

	if ( m_width < 0 ) {
		ConstQueue( 0, 0 );
	}
	return m_width;
}


float TextLabel::Height() const
{
	if ( !m_gamui )
		return 0;

	if ( m_height < 0 ) {
		ConstQueue( 0, 0 );
	}
	return m_height;
}


Image::Image() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( DEFAULT_SIZE ),
	  m_height( DEFAULT_SIZE ),
	  m_slice( false ),
	  m_capturesTap( false )
{
}

Image::~Image()
{
}


void Image::Init( Gamui* gamui, const RenderAtom& atom, bool foreground )
{
	m_atom = atom;
	m_width  = DEFAULT_SIZE;
	m_height = DEFAULT_SIZE;

	m_gamui = gamui;
	SetDialogID(gamui->CurrentDialogID());
	gamui->Add(this);
	this->SetForeground( foreground );
}


void Image::SetSize( float width, float height )							
{ 
	if ( m_width != width || m_height != height ) {
		m_width = width; 
		m_height = height; 
		Modify(); 
	}
}


bool Image::HandleTap( TapAction action, float x, float y )
{
	if ( action == TAP_DOWN ) {
		return true;
	}
	else if ( action == TAP_CANCEL ) {
		return true;
	}
	else if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
		return true;
	}
	return false;
}


void Image::SetAtom( const RenderAtom& atom )
{
	if ( !atom.Equal( m_atom ) ) {
		m_atom = atom;
		Modify();
	}
}


void Image::SetSlice( bool enable )
{
	if ( enable != m_slice ) { 
		m_slice = enable;
		Modify();
	}
}


void Image::SetForeground( bool foreground )
{
	this->SetLevel( foreground ? Gamui::LEVEL_FOREGROUND : Gamui::LEVEL_BACKGROUND );
}


bool Image::DoLayout()
{
	return true;
}


Canvas::Canvas() : UIItem( Gamui::LEVEL_FOREGROUND )
{
}

Canvas::~Canvas()
{
}


void Canvas::Init( Gamui* gamui, const RenderAtom& atom )
{
	m_atom = atom;
	m_gamui = gamui;
	gamui->Add(this);
}


void Canvas::Clear()
{
	m_cmds.Clear();
	Modify();
}


void Canvas::DrawLine(float x0, float y0, float x1, float y1, float thick)
{
	Cmd cmd = { LINE, x0, y0, x1, y1, thick };
	m_cmds.Push(cmd);
	Modify();
}


void Canvas::DrawRectangle(float x, float y, float w, float h)
{
	Cmd cmd = { RECTANGLE, x, y, w, h };
	m_cmds.Push(cmd);
	Modify();
}


void Canvas::Queue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf )
{
	if ( m_atom.textureHandle == 0 ) {
		return;
	}

	const int startVertex = vertexBuf->Size();
	for (int i = 0; i < m_cmds.Size(); ++i) {

		const Cmd& cmd = m_cmds[i];
		
		switch (cmd.type) {
			case LINE:
			{
				Gamui::Vertex* vertex = PushQuad(indexBuf, vertexBuf);
				float half = cmd.thickness * 0.5f;
				float nX = cmd.x1 - cmd.x0;
				float nY = cmd.y1 - cmd.y0;
				float len = sqrt(nX*nX + nY*nY);
				nX /= len;
				nY /= len;

				float rX = nY * half;
				float rY = -nX * half;

				vertex[0].Set(X() + cmd.x0 + rX, Y() + cmd.y0 + rY, m_atom.tx0, m_atom.ty0);
				vertex[1].Set(X() + cmd.x0 - rX, Y() + cmd.y0 - rY, m_atom.tx0, m_atom.ty0);
				vertex[2].Set(X() + cmd.x1 - rX, Y() + cmd.y1 - rY, m_atom.tx1, m_atom.ty1);
				vertex[3].Set(X() + cmd.x1 + rX, Y() + cmd.y1 + rY, m_atom.tx1, m_atom.ty1);
			}
			break;

			case RECTANGLE:
			{
				Gamui::Vertex* vertex = PushQuad(indexBuf, vertexBuf);

				float x0 = cmd.x0;
				float x1 = cmd.x0 + cmd.w;
				float y0 = cmd.y0;
				float y1 = cmd.y0 + cmd.h;

				vertex[0].Set(X() + x0, Y() + y0, m_atom.tx0, m_atom.ty0);
				vertex[1].Set(X() + x0, Y() + y1, m_atom.tx0, m_atom.ty0);
				vertex[2].Set(X() + x1, Y() + y1, m_atom.tx1, m_atom.ty1);
				vertex[3].Set(X() + x1, Y() + y0, m_atom.tx1, m_atom.ty1);
			}
			break;
		}
	}
	m_gamui->TransformVirtualToPhysical(vertexBuf->Mem() + startVertex, vertexBuf->Size() - startVertex);
}



TiledImageBase::TiledImageBase() : UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 )
{
}


TiledImageBase::TiledImageBase( Gamui* gamui ): UIItem( Gamui::LEVEL_BACKGROUND ),
	  m_width( 0 ),
	  m_height( 0 )
{
	Init( gamui );
}


TiledImageBase::~TiledImageBase()
{
}


void TiledImageBase::Init( Gamui* gamui )
{
	m_width = DEFAULT_SIZE;
	m_height = DEFAULT_SIZE;

	m_gamui = gamui;
	SetDialogID(gamui->CurrentDialogID());
	gamui->Add(this);
}


void TiledImageBase::SetTile( int x, int y, const RenderAtom& atom )
{
	GAMUIASSERT( x<CX() );
	GAMUIASSERT( y<CY() );
	if ( x < 0 || x >= CX() || y < 0 || y >= CY() )
		return;

	int index = 0;

	if ( atom.textureHandle == 0 ) {
		// Can always add a null atom.
		index = 0;
	}
	else if ( m_atom[1].textureHandle == 0 ) {
		// First thing added.
		index = 1;
		m_atom[1] = atom;
	}
	else {
		GAMUIASSERT( atom.renderState == m_atom[1].renderState );
		GAMUIASSERT( atom.textureHandle == m_atom[1].textureHandle );
		for ( index=1; index<MAX_ATOMS && m_atom[index].textureHandle; ++index ) {
			if (    m_atom[index].tx0 == atom.tx0
				 && m_atom[index].ty0 == atom.ty0
				 && m_atom[index].tx1 == atom.tx1
				 && m_atom[index].ty1 == atom.ty1 )
			{
				break;
			}
		}
		m_atom[index] = atom;
	}
	*(Mem()+y*CX()+x) = index;
	Modify();
}


void TiledImageBase::SetForeground( bool foreground )
{
	this->SetLevel( foreground ? Gamui::LEVEL_FOREGROUND : Gamui::LEVEL_BACKGROUND );
}


const RenderAtom& TiledImageBase::GetRenderAtom() const
{
	return m_atom[1];
}


void TiledImageBase::Clear()														
{ 
	memset( Mem(), 0, CX()*CY() ); 
	memset( m_atom, 0, sizeof(RenderAtom)*MAX_ATOMS );
	Modify();
}

bool TiledImageBase::DoLayout()
{
	return true;
}


void TiledImageBase::Queue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf )
{
	int startVertex = vertexBuf->Size();

	int cx = CX();
	int cy = CY();
	float x = X();
	float y = Y();
	float dx = Width() / (float)cx;
	float dy = Height() / (float)cy;
	int8_t* mem = Mem();
	int count = 0;

	for( int j=0; j<cy; ++j ) {
		for( int i=0; i<cx; ++i ) {
			if (*mem >= 0 && *mem < MAX_ATOMS && m_atom[*mem].textureHandle ) {
				Gamui::Vertex* vertex = PushQuad( indexBuf, vertexBuf );

				vertex[0].Set( x,	y,		m_atom[*mem].tx0, m_atom[*mem].ty1 );
				vertex[1].Set( x,	y+dy,	m_atom[*mem].tx0, m_atom[*mem].ty0 );
				vertex[2].Set( x+dx, y+dy,	m_atom[*mem].tx1, m_atom[*mem].ty0 );
				vertex[3].Set( x+dx, y,		m_atom[*mem].tx1, m_atom[*mem].ty1 );

				++count;
			}
			x += dx;
			++mem;
		}
		x = X();
		y += dy;
	}
	ApplyRotation( count*4, vertexBuf->Mem() + startVertex );
	m_gamui->TransformVirtualToPhysical(vertexBuf->Mem() + startVertex, vertexBuf->Size() - startVertex);
}


const RenderAtom& Image::GetRenderAtom() const
{
	return m_atom;
}


void Image::Queue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf )
{
	if ( m_atom.textureHandle == 0 ) {
		return;
	}
	const int startVertex = vertexBuf->Size();

	// Dislike magic numbers, but also dislike having to track atom sizes.
	//float sliceSize = 0.75f * Min( m_width, m_height );
	// FIXME: need to set a default size in the Gamui object
	float sliceSize = 30;

	if (    !m_slice
		 || ( m_width <= sliceSize && m_height <= sliceSize ) )
	{
		Gamui::Vertex* vertex = PushQuad( indexBuf, vertexBuf );

		float x0 = X();
		float y0 = Y();
		float x1 = X() + m_width;
		float y1 = Y() + m_height;

		vertex[0].Set( x0, y0, m_atom.tx0, m_atom.ty1 );
		vertex[1].Set( x0, y1, m_atom.tx0, m_atom.ty0 );
		vertex[2].Set( x1, y1, m_atom.tx1, m_atom.ty0 );
		vertex[3].Set( x1, y0, m_atom.tx1, m_atom.ty1 );
		ApplyRotation( 4, vertex );
	}
	else {
		float x[4] = { X(), X()+(sliceSize*0.5f), X()+(m_width-sliceSize*0.5f), X()+m_width };
		if ( x[2] < x[1] ) {
			x[2] = x[1] = X() + (sliceSize*0.5f);
		}
		float y[4] = { Y(), Y()+(sliceSize*0.5f), Y()+(m_height-sliceSize*0.5f), Y()+m_height };
		if ( y[2] < y[1] ) {
			y[2] = y[1] = Y() + (sliceSize*0.5f);
		}

		float tx[4] = { m_atom.tx0, Mean( m_atom.tx0, m_atom.tx1 ), Mean( m_atom.tx0, m_atom.tx1 ), m_atom.tx1 };
		float ty[4] = { m_atom.ty1, Mean( m_atom.ty0, m_atom.ty1 ), Mean( m_atom.ty0, m_atom.ty1 ), m_atom.ty0 };

		int base = vertexBuf->Size();
		Gamui::Vertex* vertex = vertexBuf->PushArr( 16 );

		for( int j=0; j<4; ++j ) {
			for( int i=0; i<4; ++i ) {
				vertex[j*4+i].Set( x[i], y[j], tx[i], ty[j] );
			}
		}
		ApplyRotation( 16, vertex );

		uint16_t* index = indexBuf->PushArr( 6*3*3 );
		int count=0;

		for( int j=0; j<3; ++j ) {
			for( int i=0; i<3; ++i ) {
				index[count++] = base + j*4+i;
				index[count++] = base + (j+1)*4+i;
				index[count++] = base + (j+1)*4+(i+1);

				index[count++] = base + j*4+i;
				index[count++] = base + (j+1)*4+(i+1);
				index[count++] = base + j*4+(i+1);
			}
		}
	}
	m_gamui->TransformVirtualToPhysical(vertexBuf->Mem() + startVertex, vertexBuf->Size() - startVertex);
}




Button::Button() : UIItem( Gamui::LEVEL_FOREGROUND ),
	m_up( true )
{
}


void Button::Init(	Gamui* gamui,
					const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled )
{
	m_gamui = gamui;

	m_atoms[UP] = atomUpEnabled;
	m_atoms[UP_D] = atomUpDisabled;
	m_atoms[DOWN] = atomDownEnabled;
	m_atoms[DOWN_D] = atomDownDisabled;
	m_atoms[DECO] = decoEnabled;
	m_atoms[DECO_D] = decoDisabled;

	m_textLayout = CENTER;
	m_decoLayout = CENTER;
	m_textDX = 0;
	m_textDY = 0;
	m_decoDX = 0;
	m_decoDY = 0;	
	SetDialogID(gamui->CurrentDialogID());

	m_face.Init( gamui, atomUpEnabled, true );
	m_face.SetSlice( true );

	m_deco.Init( gamui, decoEnabled, false );	// does nothing; we set the level to deco
	m_deco.SetLevel( Gamui::LEVEL_DECO );

	RenderAtom nullAtom;
	m_icon.Init( gamui, nullAtom, false );
	m_icon.SetLevel( Gamui::LEVEL_ICON );

	m_label.Init( gamui );
	gamui->Add( this );
}


float Button::Width() const
{
	float x0 = m_face.X() + m_face.Width();
	float x1 = m_deco.X() + m_deco.Width();
	float x2 = m_label.X() + m_label.Width();
	float x3 = x2;

	float x = Max( x0, Max( x1, Max( x2, x3 ) ) );
	return x - X();
}


float Button::Height() const
{
	float y0 = m_face.Y() + m_face.Height();
	float y1 = m_deco.Y() + m_deco.Height();
	float y2 = m_label.Y() + m_label.Height();
	float y3 = y2;

	float y = Max( y0, Max( y1, Max( y2, y3 ) ) );
	return y - Y();
}


void Button::PositionChildren()
{
	// --- deco ---
	if ( m_face.Width() > m_face.Height() ) {
		m_deco.SetSize( m_face.Height() , m_face.Height() );

		if ( m_decoLayout == LEFT ) {
			m_deco.SetPos( X() + m_decoDX, Y() + m_decoDY );
		}
		else if ( m_decoLayout == RIGHT ) {
			m_deco.SetPos( X() + m_face.Width() - m_face.Height() + m_decoDX, Y() + m_decoDY );
		}
		else {
			m_deco.SetPos( X() + (m_face.Width()-m_face.Height())*0.5f + m_decoDX, Y() + m_decoDY );
		}
	}
	else {
		m_deco.SetSize( m_face.Width() , m_face.Width() );
		m_deco.SetPos( X() + m_decoDX, Y() + (m_face.Height()-m_face.Width())*0.5f + m_decoDY );
	}

	// --- face ---- //
	float x=0, y0=0, y1=0;

	float h = m_label.Height();
	y0 = Y() + (m_face.Height() - h)*0.5f;

	// --- icon -- //
	float iconSize = Min( m_face.Height(), m_face.Width() ) * 0.5f;
	m_icon.SetSize( iconSize, iconSize );
	m_icon.SetPos( m_face.X() + m_face.Width() - iconSize, m_face.Y() + m_face.Height() - iconSize );

	// --- text --- //
	IGamuiText* itext = m_gamui->GetTextInterface();
	if (itext) {
		IGamuiText::FontMetrics font;
		itext->GamuiFont(&font);

		float w = m_label.Width();

		if (m_textLayout == LEFT) {
			x = X();
		}
		else if (m_textLayout == RIGHT) {
			x = X() + m_face.Width() - w;
		}
		else {
			x = X() + (m_face.Width() - w)*0.5f;
		}

		x += m_textDX;
		y0 += m_textDY;
		y1 += m_textDY;

		m_label.SetPos(x, y0);
	}
	m_label.SetVisible( Visible() );
	m_deco.SetVisible( Visible() );
	m_face.SetVisible( Visible() );
	m_icon.SetVisible( Visible() );

	// Modify(); don't call let sub-functions check
}


void Button::SetPos( float x, float y )
{
	UIItem::SetPos( x, y );
	m_face.SetPos( x, y );
	m_deco.SetPos( x, y );
	m_icon.SetPos( x, y );
	m_label.SetPos( x, y );
	// Modify(); don't call let sub-functions check
}

void Button::SetSize( float width, float height )
{
	m_face.SetSize( width, height );
	float size = Min( width, height );
	m_deco.SetSize( size, size );
	m_icon.SetSize( size*0.5f, size*0.5f );
	// Modify(); don't call let sub-functions check
}


void Button::SetText( const char* text )
{
	GAMUIASSERT( text );
	m_label.SetText( text );	// calls Modify()
}


void Button::SetState()
{
	int faceIndex = UP;
	int decoIndex = DECO;
	int iconIndex = ICON;
	if ( m_enabled ) {
		if ( m_up ) {
			// defaults set
		}
		else {
			faceIndex = DOWN;
		}
	}
	else {
		if ( m_up ) {
			faceIndex = UP_D;
			decoIndex = DECO_D;
			iconIndex = ICON_D;
		}
		else {
			faceIndex = DOWN_D;
			decoIndex = DECO_D;
			iconIndex = ICON_D;
		}
	}
	m_face.SetAtom( m_atoms[faceIndex] );
	m_deco.SetAtom( m_atoms[decoIndex] );
	m_icon.SetAtom( m_atoms[iconIndex] );
	m_label.SetEnabled( m_enabled );
	//Modify(); sub functions should call
}


void Button::SetEnabled( bool enabled )
{
	m_enabled = enabled;
	SetState();
}


const RenderAtom& Button::GetRenderAtom() const
{
	return Gamui::NullAtom();
}


bool Button::DoLayout()
{
	PositionChildren();
	return false;	// children render, not this
}


void Button::Queue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf )
{
	// does nothing - children draw
}


bool PushButton::HandleTap( TapAction action, float x, float y )
{
	bool activated = false;
	if ( action == TAP_DOWN ) {
		m_up = false;
		activated = true;
	}
	else if ( action == TAP_UP || action == TAP_CANCEL ) {
		m_up = true;
		if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
			activated = true;
		}
	}
	SetState();
	return activated;
}


ToggleButton::~ToggleButton()		
{ 
	RemoveFromToggleGroup();
	if ( m_subItemArr ) {
		for( int i=0; i<m_subItemArr->Size(); ++i ) {
			(*m_subItemArr)[i]->SetSuperItem( 0 );
		}
		delete m_subItemArr;
	}
}


void ToggleButton::Clear()
{
	RemoveFromToggleGroup();
	Button::Clear();
}


void ToggleButton::RemoveFromToggleGroup()
{
	if ( m_next ) {
		ToggleButton* other = m_next;
		
		// unlink
		m_prev->m_next = m_next;
		m_next->m_prev = m_prev;

		// clean up the group just left.
		if ( other != this ) {
			other->ProcessToggleGroup();
		}
		m_next = m_prev = this;
	}
}


void ToggleButton::AddToToggleGroup( ToggleButton* button )
{
	// Used to assert, but easy mistake and sometimes useful to not have to check.
	if ( !button || button == this )
		return;

	GAMUIASSERT( this->m_next && button->m_next );
	if ( this->m_next && button->m_next ) {

		button->RemoveFromToggleGroup();
		
		button->m_prev = this;
		button->m_next = this->m_next;

		this->m_next->m_prev = button;
		this->m_next = button;

		ProcessToggleGroup();
	}
}


bool ToggleButton::InToggleGroup()
{
	return m_next != this;
}


void ToggleButton::ProcessToggleGroup()
{
	if ( m_next && m_next != this ) {
		// One and only one button can be down.
		ToggleButton* firstDown = 0;
		ToggleButton* candidate = 0;

		if ( this->Down() )
			firstDown = this;

		for( ToggleButton* it = this->m_next; it != this; it = it->m_next ) {
			if ( firstDown )
				it->PriSetUp();
			else if ( it->Down() && it->Visible() && it->Enabled() )
				firstDown = it;
			else
				it->PriSetUp();

			if ( !firstDown && it->Visible() && it->Enabled() )
				candidate = it;
		}

		if ( !firstDown && Visible() && Enabled() )
			this->PriSetDown();
		else if ( candidate )
			candidate->PriSetDown();
		else
			this->PriSetDown();
	}
}


void UIItem::DoPreLayout()
{
	if ( m_superItem ) {
		SetVisible( m_superItem->Visible() && m_superItem->Down() );
	}
}

void ToggleButton::AddSubItem( UIItem* item )
{
	GAMUIASSERT( item->SuperItem() == 0 );

	if ( !m_subItemArr ) {
		m_subItemArr = new PODArray< UIItem* >();
	}
	m_subItemArr->Push( item );
	item->SetSuperItem( this );
}


void ToggleButton::RemoveSubItem( UIItem* item )
{
	GAMUIASSERT( m_subItemArr );
	int index = m_subItemArr->Find( item );
	GAMUIASSERT( index >= 0 );
	item->SetSuperItem( 0 );
	m_subItemArr->Remove( index );
}


bool ToggleButton::HandleTap( TapAction action, float x, float y )
{
	bool activated = false;
	if ( action == TAP_DOWN ) {
		m_wasUp = m_up;
		m_up = false;
		activated = true;
	}
	else if ( action == TAP_UP || action == TAP_CANCEL ) {
		m_up = m_wasUp;
		if ( action == TAP_UP && x >= X() && x < X()+Width() && y >= Y() && y < Y()+Height() ) {
			activated = true;
			m_up = !m_wasUp;
			ProcessToggleGroup();
		}
	}
	SetState();
	return activated;
}


DigitalBar::DigitalBar() : UIItem( Gamui::LEVEL_FOREGROUND ),
	m_double( false ),
	m_width( DEFAULT_SIZE ),
	m_height( DEFAULT_SIZE )
{
}


void DigitalBar::Init(	Gamui* gamui,
						const RenderAtom& atom0,
						const RenderAtom& atom1 )
{
	m_gamui = gamui;
	SetDialogID(gamui->CurrentDialogID());
	m_gamui->Add(this);

	m_t[0] = m_t[1] = 0;

	m_image[0].Init( gamui, atom0, true );
	m_image[1].Init( gamui, atom1, true );
	m_image[2].Init( gamui, atom0, true );
	m_image[3].Init( gamui, atom1, true );

	m_textLabel.Init( gamui );
	SetRange( 0 );
}


void DigitalBar::SetSize( float w, float h )
{
	m_width = w;
	m_height = h;
	Modify();
}


void DigitalBar::SetAtom( int which, const RenderAtom& atom, int n )
{
	GAMUIASSERT(n == 0 || n == 1);
	if ( !atom.Equal( m_image[n*2+which].GetRenderAtom() )) {
		m_image[n * 2 + which].SetAtom(atom);
		Modify();
	}
}


void DigitalBar::SetRange( float t0, int n )
{
	if ( t0 < 0 ) t0 = 0;
	if ( t0 > 1 ) t0 = 1;
	GAMUIASSERT(n == 0 || n == 1);
	if ( t0 != m_t[n] ) {
		m_t[n] = t0;
		Modify();
	}
}


float DigitalBar::Width() const
{
	return m_width;
}


float DigitalBar::Height() const
{
	return m_height;
}


void DigitalBar::SetVisible( bool visible )
{
	UIItem::SetVisible( visible );
	m_image[0].SetVisible( visible );
	m_image[1].SetVisible( visible );
	m_image[2].SetVisible( visible && m_double);
	m_image[3].SetVisible( visible && m_double);
	m_textLabel.SetVisible( visible );
}


const RenderAtom& DigitalBar::GetRenderAtom() const
{
	return Gamui::NullAtom();
}


bool DigitalBar::DoLayout()
{
	float mult = m_double ? 0.5f : 1.0f;
	m_image[0].SetSize(m_width * m_t[0], m_height*mult);
	m_image[1].SetSize(m_width * (1.0f - m_t[0]), m_height*mult);

	m_image[0].SetPos(X(), Y());
	m_image[1].SetPos(X() + m_width*m_t[0], Y());

	if (m_double) {
		m_image[2].SetSize(m_width * m_t[1], m_height*mult);
		m_image[3].SetSize(m_width * (1.0f - m_t[1]), m_height*mult);

		m_image[2].SetPos(X(), Y() + m_height*mult);
		m_image[3].SetPos(X() + m_width*m_t[1], Y() + m_height*mult);

		m_image[2].SetVisible(this->Visible());
		m_image[3].SetVisible(this->Visible());
	}
	else {
		m_image[2].SetVisible(false);
		m_image[3].SetVisible(false);
	}

	m_textLabel.SetCenterPos(X() + Width()*0.5f, Y() + Height()*0.5f);
	return false;
}


void DigitalBar::Queue( PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf )
{
	// does nothing - children draw
}


Gamui::Gamui()
	:	m_itemTapped( 0 ),
		m_disabledItemTapped(0),
		m_iText( 0 ),
		m_orderChanged( true ),
		m_physicalWidth(800),
		m_physicalHeight(600),
		m_virtualHeight(600),
		m_modified( true ),
		m_dragStart( 0 ),
		m_dragEnd( 0 ),
		//m_textHeight( 16 ),
		m_relativeX( 0 ),
		m_relativeY( 0 ),
		m_focus( -1 ),
		m_focusImage( 0 ),
		m_currentDialog(0)
{
}


Gamui::~Gamui()
{
	delete m_focusImage;
	for( int i=0; i<m_itemArr.Size(); ++i ) {
		m_itemArr[i]->Clear();
	}
}


void Gamui::Init(	IGamuiRenderer* renderer )
{
	m_iRenderer = renderer;
	m_iText = 0;
}


void Gamui::SetText(	const RenderAtom& textEnabled,
						const RenderAtom& textDisabled,
						IGamuiText* iText)
{
	//m_textHeight = size;
	m_textAtomEnabled = textEnabled;
	m_textAtomDisabled = textDisabled;
	m_iText = iText;
}


void Gamui::SetScale(int pixelWidth, int pixelHeight, int virtualHeight)
{
	if (pixelWidth != m_physicalWidth || pixelHeight != m_physicalHeight || virtualHeight != m_virtualHeight) {
		m_physicalWidth = pixelWidth;
		m_physicalHeight = pixelHeight;
		m_virtualHeight = virtualHeight;
		Modify();
	}
}


void Gamui::Add( UIItem* item )
{
	m_itemArr.Push( item );
	OrderChanged();
}


void Gamui::Remove( UIItem* item )
{
	if (m_itemTapped == item) {
		m_itemTapped = 0;
	}
	if (m_disabledItemTapped == item) {
		m_disabledItemTapped = 0;
	}
	int index = m_itemArr.Find( item );
	GAMUIASSERT( index >= 0 );
	if ( index >= 0 ) {
		m_itemArr.SwapRemove( index );
	}
	OrderChanged();
}


void Gamui::GetRelativeTap( float* x, float* y )
{
	*x = m_relativeX;
	*y = m_relativeY;
}


const UIItem* Gamui::Tap(float xPhysical, float yPhysical)
{
	TapDown(xPhysical, yPhysical);
	return TapUp(xPhysical, yPhysical);
}


void Gamui::TapDown( float xPhysical, float yPhysical )
{
	float x = TransformPhysicalToVirtual(xPhysical);
	float y = TransformPhysicalToVirtual(yPhysical);

	GAMUIASSERT( m_itemTapped == 0 );
	m_itemTapped = 0;
	m_disabledItemTapped = 0;

	for( int i=0; i<m_itemArr.Size(); ++i ) {
		UIItem* item = m_itemArr[i];

		if (	item->CanHandleTap()    
			 && ((item->DialogID() == 0) || (!m_dialogStack.Empty() && m_dialogStack[m_dialogStack.Size() - 1] == item->DialogID()))
			 && item->Visible()
			 && x >= item->X() && x < item->X()+item->Width()
			 && y >= item->Y() && y < item->Y()+item->Height() )
		{
			if (item->Enabled()) {
				if (item->HandleTap(UIItem::TAP_DOWN, x, y)) {
					m_itemTapped = item;
					m_relativeX = (x - item->X()) / item->Width();
					m_relativeY = (y - item->Y()) / item->Height();
					break;
				}
			}
			else {
				m_disabledItemTapped = item;
			}
		}
	}
}


const UIItem* Gamui::TapUp( float xPhysical, float yPhysical )
{
	float x = TransformPhysicalToVirtual(xPhysical);
	float y = TransformPhysicalToVirtual(yPhysical);

	m_dragStart = m_itemTapped;

	const UIItem* result = 0;
	if ( m_itemTapped ) {
		if ( m_itemTapped->HandleTap( UIItem::TAP_UP, x, y ) )
			result = m_itemTapped;
	}
	m_itemTapped = 0;
	m_disabledItemTapped = 0;

	m_dragEnd = 0;
	for( int i=0; i<m_itemArr.Size(); ++i ) {
		UIItem* item = m_itemArr[i];

		if (    item->CanHandleTap()
			 &&	item->Enabled() 
			 && ((item->DialogID() == 0) || (!m_dialogStack.Empty() && m_dialogStack[m_dialogStack.Size() - 1] == item->DialogID()))
			 && item->Visible()
			 && x >= item->X() && x < item->X()+item->Width()
			 && y >= item->Y() && y < item->Y()+item->Height() )
		{
			m_dragEnd = item;
			break;
		}
	}
	return result;
}


void Gamui::TapCancel()
{
	if ( m_itemTapped ) {
		m_itemTapped->HandleTap( UIItem::TAP_CANCEL, 0, 0 );
	}
	m_itemTapped = 0;
	m_disabledItemTapped = 0;
}


int Gamui::SortItems( const void* _a, const void* _b )
{ 
	const UIItem* a = *((const UIItem**)_a);
	const UIItem* b = *((const UIItem**)_b);
	// Priorities:
	// 1. Level
	// 2. RenderState
	// 3. Texture

	// Level wins.
	if ( a->Level() < b->Level() )
		return -1;
	else if ( a->Level() > b->Level() )
		return 1;

	const RenderAtom& atomA = a->GetRenderAtom();
	const RenderAtom& atomB = b->GetRenderAtom();

	const void* rStateA = atomA.renderState;
	const void* rStateB = atomB.renderState;

	// If level is the same, look at the RenderAtom;
	// to get to the state:
	if ( rStateA < rStateB )
		return -1;
	else if ( rStateA > rStateB )
		return 1;

	const void* texA = atomA.textureHandle;
	const void* texB = atomB.textureHandle;

	// finally the texture.
	if ( texA < texB )
		return -1;
	else if ( texA > texB )
		return 1;

	// just to guarantee a consistent order.
	return a - b;
}


void Gamui::Render()
{
	if ( m_modified ) {
		for( int i=0; i<m_itemArr.Size(); ++i ) {
			m_itemArr[i]->DoPreLayout();
		}
	}

	if ( m_focusImage ) {
		const UIItem* focused = GetFocus();
		if ( focused ) {
			m_focusImage->SetVisible( true );
			m_focusImage->SetSize(TextHeightVirtual()*2.5f, TextHeightVirtual()*2.5f);
			m_focusImage->SetCenterPos( focused->X() + focused->Width()*0.5f, focused->Y() + focused->Height()*0.5f );
		}
		else {
			m_focusImage->SetVisible( false );
		}
	}

	if ( m_orderChanged ) {
		qsort( m_itemArr.Mem(), m_itemArr.Size(), sizeof(UIItem*), SortItems );
		m_orderChanged = false;
	}

	if ( m_modified ) {
		State* state = 0;

		m_stateBuffer.Clear();
		m_indexBuffer.Clear();
		m_vertexBuffer.Clear();

		for( int i=0; i<m_itemArr.Size(); ++i ) {
			UIItem* item = m_itemArr[i];
			const RenderAtom& atom = item->GetRenderAtom();

			// Requires() does layout / sets child visibility. Can't skip this step:
			bool needsToRender = item->DoLayout();

			if ( !needsToRender || !item->Visible() || !atom.textureHandle )
				continue;

			// Dialog boxes must be pushed to show up:
			if (item->DialogID()) {
				if (m_dialogStack.Find(item->DialogID()) < 0) {
					continue;
				}
			}

			// Do we need a new state?
			if (    !state
				 || atom.renderState   != state->renderState
				 || atom.textureHandle != state->textureHandle ) 
			{
				state = m_stateBuffer.PushArr( 1 );
				state->vertexStart = m_vertexBuffer.Size();
				state->indexStart = m_indexBuffer.Size();
				state->renderState = atom.renderState;
				state->textureHandle = atom.textureHandle;
			}
			item->Queue( &m_indexBuffer, &m_vertexBuffer );

			state->nVertex = (uint16_t)m_vertexBuffer.Size() - state->vertexStart;
			state->nIndex  = (uint16_t)m_indexBuffer.Size() - state->indexStart;
		}
		m_modified = false;
	}

	if ( m_indexBuffer.Size() >= 65536 || m_vertexBuffer.Size() >= 65536 ) {
		GAMUIASSERT( 0 );
		return;
	}

	const void* renderState = 0;
	const void* textureHandle = 0;

	if ( m_indexBuffer.Size() ) {
		m_iRenderer->BeginRender( m_indexBuffer.Size(), &m_indexBuffer[0], m_vertexBuffer.Size(), &m_vertexBuffer[0] );

		for( int i=0; i<m_stateBuffer.Size(); ++i ) {
			const State& state = m_stateBuffer[i];

			if ( state.renderState != renderState ) {
				m_iRenderer->BeginRenderState( state.renderState );
				renderState = state.renderState;
			}
			if ( state.textureHandle != textureHandle ) {
				m_iRenderer->BeginTexture( state.textureHandle );
				textureHandle = state.textureHandle;
			}

			// The vertexBuffer is a little non-obvious: since
			// the index array references vertices from the beginning,
			// they are the same each time.
			m_iRenderer->Render(	renderState, 
									textureHandle, 
									state.indexStart,
									state.nIndex );
		}
		m_iRenderer->EndRender();
	}
}


const char* SkipSpace( const char* p ) {
	while( p && *p && *p == ' ' ) {
		++p;
	}
	return p;
}

const char* EndOfWord( const char* p ) {
	while( p && *p && *p != ' ' && *p != '\n' ) {
		++p;
	}
	return p;
}


#if 0
void Gamui::LayoutTextBlock(	const char* text,
								TextLabel* textLabels, int nText,
								float originX, float originY,
								float width )
{
	GAMUIASSERT( text );
	const char* p = text;
	int i = 0;

	TextLabel label;
	label.Init( this );
	label.SetText( "X" );
	float lineHeight = label.Height();

	while ( i < nText && *p ) {
		label.ClearText();
		
		// Add first word: always get at least one.		
		p = SkipSpace( p );
		const char* end = EndOfWord( p );
		end = SkipSpace( end );

		// (p,end) definitely gets added. The question is how much more?
		const char* q = end;
		while ( *q && *q != '\n' ) {
			q = EndOfWord( q );
			q = SkipSpace( q );
			label.SetText( p, q );
			if ( label.Width() > width ) {
				break;
			}
			else {
				end = q;
			}
		}
		
		textLabels[i].SetText( p, end );
		textLabels[i].SetPos( originX, originY + (float)i*lineHeight );
		p = end;
		++i;
		// We just put in a carriage return, so the first is free:
		if ( *p == '\n' )
			++p;

		// The rest advance i:
		while ( *p == '\n' && i < nText ) {
			textLabels[i].ClearText();
			++i;
			++p;
		}
	}
	while( i < nText ) {
		textLabels[i].ClearText();
		++i;
	}
}
#endif


void Gamui::AddToFocusGroup( const UIItem* item, int id )
{
	FocusItem* fi = m_focusItems.PushArr(1);
	fi->item = item;
	fi->group = id;
}


void Gamui::SetFocus( const UIItem* item )
{
	m_focus = -1;
	for( int i=0; i<m_focusItems.Size(); ++i ) {
		if ( m_focusItems[i].item == item ) {
			m_focus = i;
			break;
		}
	}
}


const UIItem* Gamui::GetFocus() const
{
	if ( m_focus >= 0 && m_focus < m_focusItems.Size() ) {
		return m_focusItems[m_focus].item;
	}
	return 0;
}


float Gamui::GetFocusX()
{
	const UIItem* item = GetFocus();
	if ( item ) {
		return item->CenterX();
	}
	return -1;
}


float Gamui::GetFocusY()
{
	const UIItem* item = GetFocus();
	if ( item ) {
		return item->CenterY();
	}
	return -1;
}


int Gamui::TextHeightInPixels() const
{
	IGamuiText* iText = GetTextInterface();
	GAMUIASSERT(iText);
	if (!iText) return 0;
	IGamuiText::FontMetrics font;
	iText->GamuiFont(&font);
	return font.linespace;
}


float Gamui::TextHeightVirtual() const
{
	return TransformPhysicalToVirtual(float(TextHeightInPixels()));
}


float Gamui::TransformVirtualToPhysical(float x) const
{
	const float M = float(m_physicalHeight) / float(m_virtualHeight);
	return x * M;
}


float Gamui::TransformPhysicalToVirtual(float x) const
{
	const float M = float(m_physicalHeight) / float(m_virtualHeight);
	return x / M;
}


void Gamui::TransformVirtualToPhysical(Vertex* v, int n) const
{ 
	const float M = float(m_physicalHeight) / float(m_virtualHeight);
	while (n) {
		v->x *= M;
		v->y *= M;
		--n;
		++v;
	}
}


void Gamui::SetFocusLook( const RenderAtom& atom, float zRotation )
{
	if ( !m_focusImage ) {
		m_focusImage = new Image();
		m_focusImage->Init(this, atom, true );
		m_focusImage->SetLevel( LEVEL_FOCUS );
	}
	m_focusImage->SetAtom( atom );
	m_focusImage->SetRotationZ( zRotation );
}


void Gamui::MoveFocus( float x, float y )
{
	if ( m_focusItems.Size() == 0 ) return;
	if ( m_focusItems.Size() == 1 ) SetFocus( m_focusItems[0].item );

	float bestDist = 1000000.0f;
	int bestIndex  = -1;

	const UIItem* focused = GetFocus();
	for( int i=0; i<m_focusItems.Size(); ++i ) {
		const UIItem* item = m_focusItems[i].item;
		if ( item == focused ) {
			continue;
		}
		if ( !item->Enabled() || !item->Visible() ) {
			continue; 
		}
		float dx = item->CenterX() - focused->CenterX();
		float dy = item->CenterY() - focused->CenterY();

		float score = dx*x + dy*y;
		if ( score > 0 ) {
			float dist = sqrt( dx*dx + dy*dy );
			if ( dist < bestDist ) {
				bestDist = dist;
				bestIndex = i;
			}
		}
	}
	if ( bestIndex >= 0 ) {
		SetFocus( m_focusItems[bestIndex].item );
	}
}


void Gamui::StartDialog(const char* name)
{
	GAMUIASSERT(!m_currentDialog);
	m_currentDialog = Hash(name);
}


void Gamui::EndDialog()
{
	GAMUIASSERT(m_currentDialog);
	m_currentDialog = 0;
}


void Gamui::PushDialog(const char* name)
{
	GAMUIASSERT(name && *name);
	m_dialogStack.Push(Hash(name));
}


void Gamui::PopDialog()
{
	m_dialogStack.Pop();
}


LayoutCalculator::LayoutCalculator( float w, float h ) 
	: screenWidth( w ),
	  screenHeight( h ),
	  width( 10 ),
	  height( 10 ),
	  gutterX( 0 ), 
	  gutterY( 0 ), 
	  spacingX( 0 ),
	  spacingY( 0 ),
	  textOffsetX( 0 ),
	  textOffsetY( 0 ),
	  offsetX( 0 ),
	  offsetY( 0 ),
	  useTextOffset( false ),
	  innerX0( 0 ),
	  innerY0( 0 ),
	  innerX1( w ),
	  innerY1( h )
{
}


LayoutCalculator::~LayoutCalculator()
{
}


void LayoutCalculator::PosAbs( IWidget* item, int _x, int _y, int _cx, int _cy, bool setSize )
{
	float pos[2] = { 0, 0 };
	int xArr[2] = { _x, _y };
	float size[2] = { width, height };
	float screen[2] = { screenWidth, screenHeight };
	float gutter[2] = { gutterX, gutterY };
	float spacing[2] = { spacingX, spacingY };

	for( int i=0; i<2; ++i ) {
		if ( xArr[i] >= 0 ) {
			float x = (float)xArr[i];	// 0 based
			float space = spacing[i]*x;
			pos[i] = gutter[i] + space + size[i]*x;
		}
		else {
			float x = -(float)xArr[i]; // 1 based
			float space = spacing[i]*(x-1.0f);
			pos[i] = screen[i] - gutter[i] - space - size[i]*x; 
		}
	}
	if ( useTextOffset ) {
		pos[0] += textOffsetX;
		pos[1] += textOffsetY;
	}
	item->SetPos( pos[0]+offsetX, pos[1]+offsetY );

	if ( setSize ) {
		float w = width * float(_cx)  + spacingX * float(_cx-1);
		float h = height * float(_cy) + spacingY * float(_cy-1);
		item->SetSize( w, h );
	}

	if ( item->Visible() ) {
		// Track the inner rectangle.
		float x0, x1, y0, y1;

		if ( _x >= 0 ) {
			x0 = item->X() + item->Width() + gutter[0];
			x1 = screenWidth;
		}
		else {
			x0 = 0;
			x1 = item->X() - gutter[0];
		}
		if ( _y >= 0 ) {
			y0 = item->Y() + item->Height() + gutter[1];
			y1 = screenHeight;
		}
		else {
			y0 = 0;
			y1 = item->Y() - gutter[1];
		}
	
		innerX0 = Max( innerX0, gutter[0] );
		innerY0 = Max( innerY0, gutter[1] );
		innerX1 = Min( innerX1, screenWidth  - gutter[0] );
		innerY1 = Min( innerY1, screenHeight - gutter[1] );

		// Can trim to y or x. Which one?
		float areaX = ( Min( innerX1, x1 ) - Max( innerX0, x0 ) ) * ( innerY1 - innerY0 );
		float areaY = ( innerX1 - innerX0 ) * ( Min( innerY1, y1 ) - Max( innerY0, y0 ));
		if ( areaX > areaY ) {
			innerX0 = Max( innerX0, x0 );
			innerX1 = Min( innerX1, x1 );
		}
		else {
			innerY0 = Max( innerY0, y0 );
			innerY1 = Min( innerY1, y1 );
		}
	}
}


void LayoutCalculator::PosInner( IWidget* item, float wDivH )
{
	innerX0 = Max( innerX0, gutterX );
	innerX1 = Min( innerX1, screenWidth - gutterX );
	innerY0 = Max( innerY0, gutterY );
	innerY1 = Min( innerY1, screenHeight - gutterY );

	float dx = innerX1 - innerX0;
	float dy = innerY1 - innerY0;

	if ( wDivH == 0 ) {
		item->SetPos( innerX0, innerY0 );
		item->SetSize( dx, dy );
	}
	else {
		if ( wDivH > (dx/dy) ) {
			float cy = dx / wDivH;
			item->SetPos( innerX0, innerY0 + (dy-cy)*0.5f );
			item->SetSize( dx, cy );
		}
		else {
			float cx = dy * wDivH;
			item->SetPos( innerX0 + (dx-cx)*0.5f, innerY0 );
			item->SetSize( cx, dy );
		}
	}
}

