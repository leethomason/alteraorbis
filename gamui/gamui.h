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

#ifndef GAMUI_INCLUDED
#define GAMUI_INCLUDED

#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#if defined( _DEBUG ) || defined( DEBUG )
#	if defined( _MSC_VER )
#		define GAMUIASSERT( x )		if ( !(x)) { _asm { int 3 } }
#	else
#		include <assert.h>
#		define GAMUIASSERT assert
#	endif
#else
#	define GAMUIASSERT( x ) {}
#endif

class Texture;

namespace gamui
{
class Gamui;
class IGamuiRenderer;
class IGamuiText;
class UIItem;
class ToggleButton;
class TextLabel;
class PushButton;
class ToggleButton;
class Image;

template < class T >
class PODArray
{
	enum { CACHE = 4 };
public:
	typedef T ElementType;

	PODArray() : size( 0 ), capacity( CACHE ), nAlloc(0) {
		mem = reinterpret_cast<T*>(cache);
		GAMUIASSERT( CACHE_SIZE*sizeof(int) >= CACHE*sizeof(T) );
	}

	~PODArray() {
		Clear();
		if ( mem != reinterpret_cast<T*>(cache) ) {
			free( mem );
		}
		GAMUIASSERT( nAlloc == 0 );
	}

	T& operator[]( int i )				{ GAMUIASSERT( i>=0 && i<(int)size ); return mem[i]; }
	const T& operator[]( int i ) const	{ GAMUIASSERT( i>=0 && i<(int)size ); return mem[i]; }

	void Push( const T& t ) {
		EnsureCap( size+1 );
		mem[size] = t;
		++size;
		++nAlloc;
	}


	void PushFront( const T& t ) {
		EnsureCap( size+1 );
		for( int i=size; i>0; --i ) {
			mem[i] = mem[i-1];
		}
		mem[0] = t;
		++size;
		++nAlloc;
	}

	T* PushArr( int count ) {
		EnsureCap( size+count );
		T* result = &mem[size];
		size += count;
		nAlloc += count;
		return result;
	}

	T Pop() {
		GAMUIASSERT( size > 0 );
		--size;
		--nAlloc;
		return mem[size];
	}

	void Remove( int i ) {
		GAMUIASSERT( i < (int)size );
		// Copy down.
		for( int j=i; j<size-1; ++j ) {
			mem[j] = mem[j+1];
		}
		// Get rid of the end:
		Pop();
	}

	void SwapRemove( int i ) {
		if (i < 0) {
			GAMUIASSERT(i == -1);
			return;
		}
		GAMUIASSERT( i<(int)size );
		GAMUIASSERT( size > 0 );
		
		mem[i] = mem[size-1];
		Pop();
	}

	int Find( const T& t ) const {
		for( int i=0; i<size; ++i ) {
			if ( mem[i] == t )
				return i;
		}
		return -1;
	}

	int Size() const		{ return size; }
	
	void Clear()			{ 
		while( !Empty() ) 
			Pop();
		GAMUIASSERT( nAlloc == 0 );
		GAMUIASSERT( size == 0 );
	}
	bool Empty() const		{ return size==0; }
	const T* Mem() const	{ return mem; }
	T* Mem()				{ return mem; }
	const T* End() const	{ return mem + size; }	// mem never 0, because of cache

	void Reserve(int n) { EnsureCap(n); }

	void EnsureCap( int count ) {
		if ( count > capacity ) {
			if (count < 16) capacity = 16;
			while (capacity < count) capacity *= 2;
			if ( mem == reinterpret_cast<T*>(cache) ) {
				mem = (T*) malloc( capacity*sizeof(T) );
				memcpy( mem, cache, size*sizeof(T) );
			}
			else {
				void* oldMem = mem;
				mem = (T*)realloc( mem, capacity*sizeof(T) );
				if (!mem) {
					free(oldMem);
				}
			}
		}
	}

protected:
	PODArray( const PODArray<T>& );	// not allowed. Add a missing '&' in the code.
	void operator=( const PODArray< T >& rhs );	// hard to implement with ownership semantics

	T* mem;
	int size;
	int capacity;
	int nAlloc;
	enum { 
		CACHE_SIZE = (CACHE*sizeof(T)+sizeof(int)-1)/sizeof(int)
	};
	int cache[CACHE_SIZE];
};


/**
	The most basic unit of state. A set of vertices and indices are sent to the GPU with a given RenderAtom, which
	encapselates a state (renderState), texture (textureHandle), and coordinates. 
*/
struct RenderAtom 
{
	/// Creates a default that renders nothing.
	RenderAtom() : tx0( 0 ), ty0( 0 ), tx1( 0 ), ty1( 0 ), renderState( 0 ), textureHandle( 0 ), user( 0 ) {}
	
	RenderAtom( const void* _renderState, const void* _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1 ) {
		Init( _renderState, _textureHandle, _tx0, _ty0, _tx1, _ty1 );
	}

	RenderAtom(int state, Texture* texture, float _tx0, float _ty0, float _tx1, float _ty1) {
		Init((const void*)state, (const void*)texture, _tx0, _ty0, _tx1, _ty1);
	}
	
	RenderAtom( const RenderAtom& rhs, const void* _renderState ) {
		Init( _renderState, rhs.textureHandle, rhs.tx0, rhs.ty0, rhs.tx1, rhs.ty1 );
	}

	RenderAtom( const RenderAtom& rhs, int _renderState ) {
		Init( (const void*)_renderState, rhs.textureHandle, rhs.tx0, rhs.ty0, rhs.tx1, rhs.ty1 );
	}

	void Init( const void* _renderState, const void* _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1 ) {
		SetCoord( _tx0, _ty0, _tx1, _ty1 );
		renderState = (const void*)_renderState;
		textureHandle = (const void*) _textureHandle;
		user = 0;
	}

	bool Equal( const RenderAtom& atom ) const {
		return (    tx0 == atom.tx0
			     && ty0 == atom.ty0
				 && tx1 == atom.tx1
				 && ty1 == atom.ty1
				 && renderState == atom.renderState
				 && textureHandle == atom.textureHandle );
	}

	/*  These methods are crazy useful. Don't work with Android compiler, which doesn't seem to like template functions at all. Grr.
	/// Copy constructor that allows a different renderState. It's often handy to render the same texture in different states.
	template <class T > RenderAtom( const RenderAtom& rhs, const T _renderState ) {
		Init( _renderState, rhs.textureHandle, rhs.tx0, rhs.ty0, rhs.tx1, rhs.ty1, rhs.srcWidth, rhs.srcHeight );
	}

	/// Constructor with all parameters.
	template <class T0, class T1 > RenderAtom( const T0 _renderState, const T1 _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1, float _srcWidth, float _srcHeight ) {
		GAMUIASSERT( sizeof( T0 ) <= sizeof( const void* ) );
		GAMUIASSERT( sizeof( T1 ) <= sizeof( const void* ) );
		Init( (const void*) _renderState, (const void*) _textureHandle, _tx0, _ty0, _tx1, _ty1, _srcWidth, _srcHeight );
	}

	/// Initialize - works just like the constructor.
	template <class T0, class T1 > void Init( const T0 _renderState, const T1 _textureHandle, float _tx0, float _ty0, float _tx1, float _ty1, float _srcWidth, float _srcHeight ) {
		GAMUIASSERT( sizeof( T0 ) <= sizeof( const void* ) );
		GAMUIASSERT( sizeof( T1 ) <= sizeof( const void* ) );
		SetCoord( _tx0, _ty0, _tx1, _ty1 );
		renderState = (const void*)_renderState;
		textureHandle = (const void*) _textureHandle;
		srcWidth = _srcWidth;
		srcHeight = _srcHeight;
	}
*/

	/// Utility method to set the texture coordinates.
	void SetCoord( float _tx0, float _ty0, float _tx1, float _ty1 ) {
		tx0 = _tx0;
		ty0 = _ty0;
		tx1 = _tx1;
		ty1 = _ty1;
	}

	float tx0, ty0, tx1, ty1;		///< Texture coordinates to use within the atom.

	const void* renderState;		///< Application defined render state.
	const void* textureHandle;		///< Application defined texture handle, noting that 0 (null) is assumed to be non-rendering.

	void* user;						///< ignored by gamui
};


/**
	Main class of the Gamui system. All Items and objects are attached to an instance of 
	Gamui, which does rendering, hit testing, and organization.

	Coordinate system assumed by Gamui.

	Screen coordinates:
	+--- x
	|
	|
	y

	Texture coordinates:
	ty
	|
	|
	+---tx

	Note that you can use any coordinate system you want, this is only what Gamui assumes. 
	Gamui can be used to draw map overlays for instance, if projected into a 3D plane.
*/
class Gamui
{
public:
	enum {
		LEVEL_BACKGROUND = 0,
		LEVEL_FOREGROUND = 10,
		LEVEL_DECO		 = 20,
		LEVEL_ICON		 = 30,
		LEVEL_TEXT		 = 40,
		LEVEL_FOCUS		 = 50
	};

	/// Description of a vertex used by Gamui.
	struct Vertex {
		enum {
			POS_OFFSET = 0,
			TEX_OFFSET = 8
		};
		float x;
		float y;
		float tx;
		float ty;

		void Set( float _x, float _y, float _tx, float _ty ) { x = _x; y = _y; tx = _tx; ty = _ty; }
	};

	/// Construct and Init later.
	Gamui();
	~Gamui();

	/// Initialize with a renderer. Called once.
	void Init(IGamuiRenderer* renderer);
	/** Initialize the text system. Can be called multiple times. Generally called in conjunction with SetScale().
		The size of the text is from the iText interface. It isn't tracked by gamui directly.

		'textEnabled' the render atom to use, usually the text texture and render state
		'textDisabled' with a lighter render effect
		'iText' the pointer to the metrics interface
	*/
	void SetText(const RenderAtom& textEnabled, const RenderAtom& textDisabled, IGamuiText* iText);

	/// Sets the physical vs. virtual coordinate system. Generally called in conjunction with SetText()
	void SetScale(int pixelWidth, int pixelHeight, int virtualHeight);

	int PixelWidth() const { return m_physicalWidth; }
	int PixelHeight() const { return m_physicalHeight; }
	float Height() const { return float(m_virtualHeight); }
	float Width() const { return TransformPhysicalToVirtual(float(m_physicalWidth)); }
	float AspectRatio() const { return float(m_physicalHeight) / float(m_physicalWidth); }

	void StartDialog(const char* name);
	void EndDialog();
	unsigned CurrentDialogID() const { return m_currentDialog; }

	void PushDialog(const char* name);
	void PopDialog();
	bool DialogDisplayed(const char* name) { return m_dialogStack.Find(Hash(name)) >= 0; }

	// normally not called by user code.
	void Add( UIItem* item );
	// normally not called by user code.
	void Remove( UIItem* item );
	void OrderChanged() { m_orderChanged = true; m_modified = true; }
	void Modify()		{ m_modified = true; }

	/// Call to begin the rendering pass and commit all the UIItems to the display.
	void Render();

	const RenderAtom& GetTextAtom() 			{ return m_textAtomEnabled; }
	const RenderAtom& GetDisabledTextAtom()		{ return m_textAtomDisabled; }

	IGamuiText* GetTextInterface() const			{ return m_iText; }

	/// Query the text height in physical pixels.
	int   TextHeightInPixels() const;
	/// Query the text height in virtual pixels.
	float TextHeightVirtual() const;

	bool PixelSnap() const { return m_pixelSnap; }
	void SetPixelSnap(bool snap) { m_pixelSnap = snap; }

	/** Feed touch/mouse events to Gamui. You should use TapDown/TapUp as a pair, OR just use Tap. TapDown/Up
		is richer, but you need device support. (Mice certainly, fingers possibly.) 
		* TapDown return the item tapped on. This does not activate anything except capture.
		* TapUp returns if the item was activated (that is, the up and down item are the same.)
	*/
	void			TapDown( float xPhysical, float yPhysical );		
	const UIItem*	TapUp( float xPhysical, float yPhysical );		///< Used as a pair with TapDown
	void			TapCancel();									///< Cancel a tap (generally in response to the OS)
	const UIItem*	TapCaptured() const { return m_itemTapped; }	///< The item that received the TapDown, while in move.
	const UIItem*	Tap( float xPhysical, float yPhysical );		///< Used to send events on systems that have a simple tap without up/down symantics.
	void			GetRelativeTap( float* x, float* y );			///< Get the tap location, in item coordinates, of the last tap down. [0,1]

	/** The return of a Tap...() or TapCaptured() is a valid UI Item.
		This is a bit of an issue; what to do if you need an item tap
		for a disabled item? (Notably to display help text.) Valid
		from TapDown() to the next TapUp() or TapCancel().
	*/
	const UIItem*	DisabledTapCaptured() const { return m_disabledItemTapped; }
 
	/** During (or after) a TapUp, this will return the starting object and ending object of the drag. One
		or both may be null, and it is commonly the same object.
	*/
	void GetDragPair( const UIItem** start, const UIItem** end )				{ *start = m_dragStart; *end = m_dragEnd; }
	void GetDragPair(float* startX, float* startY, float* endX, float* endY)	{ 
		*startX = m_dragStartX; *startY = m_dragStartY; *endX = m_dragEndX; *endY = m_dragEndY;
	}

	void			SetFocusLook( const RenderAtom& atom, float zRotation );
	void			AddToFocusGroup( const UIItem* item, int id );
	void			SetFocus( const UIItem* item );
	const UIItem*	GetFocus() const;
	void			MoveFocus( float x, float y );	// FIXME: in virtual coordinates?
	float			GetFocusX();
	float			GetFocusY();

	void			TransformVirtualToPhysical(Vertex* v, int n) const;
	float			TransformVirtualToPhysical(float x) const;
	float			TransformPhysicalToVirtual(float x) const;

	static const RenderAtom& NullAtom() { return m_nullAtom; }

private:
	static int SortItems( const void* a, const void* b );
	unsigned Hash(const char* _p)
	{
		const unsigned char* p = (const unsigned char*)_p;
		unsigned h = 2166136261U;
		for (; *p; ++p) {
			h ^= *p;
			h *= 16777619;
		}
		return h;
	}

	static RenderAtom m_nullAtom;

	UIItem*							m_itemTapped;
	UIItem*							m_disabledItemTapped;
	RenderAtom						m_textAtomEnabled;
	RenderAtom						m_textAtomDisabled;
	IGamuiRenderer*					m_iRenderer;
	IGamuiText*						m_iText;

	int				m_physicalWidth, m_physicalHeight, m_virtualHeight;
	bool			m_orderChanged;
	bool			m_modified;
	PODArray<UIItem*> m_itemArr;
	const UIItem*	m_dragStart;
	const UIItem*	m_dragEnd;
	float			m_dragStartX, m_dragStartY, m_dragEndX, m_dragEndY;
	float			m_relativeX;
	float			m_relativeY;
	int				m_focus;
	Image*			m_focusImage;
	int				m_currentDialog;
	bool			m_pixelSnap;

	struct State {
		uint16_t	vertexStart;
		uint16_t	nVertex;
		uint16_t	indexStart;
		uint16_t	nIndex;
		const void* renderState;
		const void* textureHandle;
	};

	struct FocusItem {
		const UIItem*	item;
		int				group;
	};

	PODArray< unsigned >			m_dialogStack;
	PODArray< FocusItem >			m_focusItems;
	PODArray< State >				m_stateBuffer;
	PODArray< uint16_t >			m_indexBuffer;
	PODArray< Vertex >				m_vertexBuffer;
};


class IGamuiRenderer
{
public:
	virtual ~IGamuiRenderer()	{}
	virtual void BeginRender( int nIndex, const uint16_t* index, int nVertex, const Gamui::Vertex* vertex ) = 0;
	virtual void EndRender() = 0;

	virtual void BeginRenderState( const void* renderState ) = 0;
	virtual void BeginTexture( const void* textureHandle ) = 0;
	virtual void Render( const void* renderState, const void* textureHandle, int start, int count ) = 0;
};


class IGamuiText
{
public:
	struct GlyphMetrics {
		GlyphMetrics() : advance(0), x(0), y(0), w(0), h(0), tx0(0), ty0(0), tx1(0), ty1(0) {}

		int advance;
		float x, y;					// position. Offset by x, y.
		float w, h;					// w & h. should be a ration of (tx1-tx0) and (ty1-ty0)
		float tx0, ty0, tx1, ty1;	// texture coordinates of the glyph
	};

	struct FontMetrics {
		int ascent;
		int descent;
		int linespace;				// distance between lines.
	};

	virtual ~IGamuiText()	{}
	virtual void GamuiGlyph( int c0, int cPrev,		// character, prev character
							 gamui::IGamuiText::GlyphMetrics* metric ) = 0;

	virtual void GamuiFont(gamui::IGamuiText::FontMetrics* metric) = 0;
};


/*
	The minimal interface needed to implement a widget, and 
	have it work with layout.
*/
class IWidget
{
public:
	virtual float X() const			= 0;
	virtual float Y() const			= 0;
	virtual float Width() const		= 0;
	virtual float Height() const	= 0;

	virtual void SetPos( float x, float y )			= 0;
	virtual void SetSize( float width, float h )	= 0;	// recommendation; not required
	virtual bool Visible() const					= 0;
	virtual void SetVisible( bool visible )			= 0;
};

class UIItem : public IWidget
{
public:
	enum {
		DEFAULT_SIZE = 64
	};

	virtual void SetPos( float x, float y )		
	{ 
		if ( m_x != x || m_y != y ) {
			m_x = x; m_y = y; 
			Modify(); 
		}
	}
	void SetPos( const float* x )				{ SetPos( x[0], x[1] ); }
	void SetCenterPos( float x, float y )		{ SetPos( x - Width()*0.5f, y - Height()*0.5f ); }
	
	virtual float X() const						{ return m_x; }
	virtual float Y() const						{ return m_y; }
	virtual float Width() const = 0;
	virtual float Height() const = 0;
	float CenterX() const						{ return X() + Width()*0.5f; }
	float CenterY() const						{ return Y() + Height()*0.5f; }

	// Not always supported.
	virtual void SetSize( float sx, float sy )	{}

	int Level() const							{ return m_level; }

	virtual void SetEnabled( bool enabled )		{}		// does nothing for many items.
	bool Enabled() const						{ return m_enabled; }
	virtual void SetVisible( bool visible )		
	{ 
		if ( m_visible != visible ) { 
			m_visible = visible; 
			Modify();
		}
	}
	bool Visible() const						{ return m_visible; }
	
	void SetLevel( int level )					
	{
		if ( m_level != level ) {
			m_level = level; 
			OrderChanged();
			Modify();
		}
	}

	void SetDialogID(unsigned id)				{ m_dialogID = id; }
	unsigned DialogID() const					{ return m_dialogID; }

	void SetRotationX( float degrees )			{ if ( m_rotationX != degrees ) { m_rotationX = degrees; Modify(); } }
	void SetRotationY( float degrees )			{ if ( m_rotationY != degrees ) { m_rotationY = degrees; Modify(); } }
	void SetRotationZ( float degrees )			{ if ( m_rotationZ != degrees ) { m_rotationZ = degrees; Modify(); } }
	float RotationX() const						{ return m_rotationX; }
	float RotationY() const						{ return m_rotationY; }
	float RotationZ() const						{ return m_rotationZ; }

	enum TapAction {
		TAP_DOWN,
		TAP_UP,
		TAP_CANCEL
	};
	
	virtual bool CanHandleTap()											{ return false; }
	virtual bool HandleTap( TapAction action, float x, float y )		{ return false; }

	virtual const RenderAtom& GetRenderAtom() const = 0;
	virtual bool DoLayout() = 0;
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex ) = 0;

	virtual void Clear()	{ m_gamui = 0; }
	bool Initialized() const { return m_gamui != 0; }

	// internal
	void SetSuperItem( ToggleButton* tb ) { m_superItem = tb; }
	ToggleButton* SuperItem() const		  { return m_superItem; }
	void DoPreLayout();

	const void* userData;

private:
	void operator=( const UIItem& );	// private, not implemented.

	float	m_x;
	float	m_y;
	int		m_level;
	bool	m_visible;
	float	m_rotationX;
	float	m_rotationY;
	float	m_rotationZ;
	unsigned m_dialogID;

protected:
	template <class T> T Min( T a, T b ) const		{ return a<b ? a : b; }
	template <class T> T Max( T a, T b ) const		{ return a>b ? a : b; }
	float Mean( float a, float b ) const			{ return (a+b)*0.5f; }
	static Gamui::Vertex* PushQuad( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );
	void Modify()		{ if ( m_gamui ) m_gamui->Modify(); }
	void OrderChanged()	{ if ( m_gamui ) m_gamui->OrderChanged(); }

	void ApplyRotation( int nVertex, Gamui::Vertex* vertex );

	UIItem( int level=0 );
	virtual ~UIItem();

	Gamui*			m_gamui;
	bool			m_enabled;
	ToggleButton*	m_superItem;	
};


class TextLabel : public UIItem
{
public:
	TextLabel();
	TextLabel( Gamui* );
	virtual ~TextLabel();

	void Init( Gamui* );

	virtual float Width() const;
	virtual float Height() const;

	// If width=0 or height=0, interpreted as unbounded.
	void SetBounds( float width, float height );
	void SetTab( float tabWidth )	{ if ( m_tabWidth != tabWidth )		{ m_tabWidth = tabWidth; m_width = m_height = -1; Modify(); }}

	void SetText( const char* t );
	void SetText( const char* start, const char* end );	///< Adds text from [start,end)

	const char* GetText() const;
	void ClearText();
	virtual void SetEnabled( bool enabled )		
	{ 
		if ( m_enabled != enabled ) {
			m_enabled = enabled; 
			Modify(); 
		}
	}

	virtual const RenderAtom& GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );

private:
	float WordWidth( const char* p, IGamuiText* iText ) const;
	// Always sets the width and height (which are mutable for that reason.)
	void ConstQueue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex ) const;

	enum { ALLOCATED = 16 };
	char  m_buf[ALLOCATED];
	char* m_str;
	int	  m_allocated;
	float m_boundsWidth;
	float m_boundsHeight;
	float m_tabWidth;

	mutable float m_width;
	mutable float m_height;
};


class Image : public UIItem
{
public:
	Image();
	virtual ~Image();
	void Init( Gamui*, const RenderAtom& atom, bool foreground );

	void SetAtom( const RenderAtom& atom );
	void SetSlice( bool enable );

	virtual void SetSize( float width, float height );
	void SetForeground( bool foreground );
	void SetCapturesTap( bool captures )								{ m_capturesTap = captures; }

	virtual bool CanHandleTap()											{ return m_capturesTap; }

	virtual float Width() const											{ return m_width; }
	virtual float Height() const										{ return m_height; }

	virtual const RenderAtom& GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );
	virtual bool HandleTap( TapAction action, float x, float y );

private:
	RenderAtom m_atom;
	float m_width;
	float m_height;
	bool m_slice;
	bool m_capturesTap;
};


class Canvas : public UIItem
{
public:
	Canvas();
	virtual ~Canvas();
	void Init( Gamui*, const RenderAtom& atom );

	void Clear();
	void DrawLine(float x0, float y0, float x1, float y1, float thickness);
	void DrawRectangle(float x, float y, float w, float h);
	void DrawRectangleOutline(float x, float y, float w, float h, float thickness, float arc);

	void SetAtom(const RenderAtom& atom)								{ m_atom = atom; Modify(); }
	virtual const RenderAtom& GetRenderAtom() const						{ return m_atom; }

	virtual void SetSize(float width, float height)						{}

	virtual float Width() const											{ return DEFAULT_SIZE; }
	virtual float Height() const										{ return DEFAULT_SIZE; }

	void CopyCmdsTo(Canvas* target);

	virtual bool DoLayout()												{ return true; }
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );

private:
	void PushRectangle(PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf, 
					   float x, float y, float x1, float y1);
	void PushArc(PODArray< uint16_t > *indexBuf, PODArray< Gamui::Vertex > *vertexBuf,
				 float x, float y, float angle0, float angle1, float rad, float width);

	enum {
		LINE, RECTANGLE, RECTANGLE_OUTLINE
	};
	struct Cmd {
		int type;
		float x, y, w, h;
		float thickness;
		float arc;
	};
	RenderAtom m_atom;
	PODArray<Cmd> m_cmds;
};


class TiledImageBase : public UIItem
{
public:

	virtual ~TiledImageBase();
	void Init( Gamui* );

	void SetTile( int x, int y, const RenderAtom& atom );

	void SetSize( float width, float height )							{ m_width = width; m_height = height; Modify(); }
	void SetForeground( bool foreground );

	virtual float Width() const											{ return m_width; }
	virtual float Height() const										{ return m_height; }
	void Clear();

	virtual const RenderAtom& GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );

	virtual int CX() const = 0;
	virtual int CY() const = 0;

protected:
	TiledImageBase();
	TiledImageBase( Gamui* );

	virtual int8_t* Mem() = 0;

private:
	enum {
		MAX_ATOMS = 32
	};
	RenderAtom m_atom[MAX_ATOMS];
	float m_width;
	float m_height;
};

template< int _CX, int _CY > 
class TiledImage : public TiledImageBase
{
public:
	TiledImage() : TiledImageBase() {}
	TiledImage( Gamui* g ) : TiledImageBase( g )	{}

	virtual ~TiledImage() {}

	virtual int CX() const			{ return _CX; }
	virtual int CY() const			{ return _CY; }

protected:
	virtual int8_t* Mem()			{ return mem; }

private:
	int8_t mem[_CX*_CY];
};


struct ButtonLook
{
	ButtonLook()	{};
	ButtonLook( const RenderAtom& _atomUpEnabled,
				const RenderAtom& _atomUpDisabled,
				const RenderAtom& _atomDownEnabled,
				const RenderAtom& _atomDownDisabled ) 
		: atomUpEnabled( _atomUpEnabled ),
		  atomUpDisabled( _atomUpDisabled ),
		  atomDownEnabled( _atomDownEnabled ),
		  atomDownDisabled( _atomDownDisabled )
	{}
	void Init( const RenderAtom& _atomUpEnabled, const RenderAtom& _atomUpDisabled, const RenderAtom& _atomDownEnabled, const RenderAtom& _atomDownDisabled ) {
		atomUpEnabled = _atomUpEnabled;
		atomUpDisabled = _atomUpDisabled;
		atomDownEnabled = _atomDownEnabled;
		atomDownDisabled = _atomDownDisabled;
	}

	RenderAtom atomUpEnabled;
	RenderAtom atomUpDisabled;
	RenderAtom atomDownEnabled;
	RenderAtom atomDownDisabled;
};

class Button : public UIItem
{
public:
	virtual ~Button()	{}

	virtual float Width() const;
	virtual float Height() const;

	virtual void SetPos( float x, float y );
	void SetSize( float width, float height );

	virtual void SetEnabled( bool enabled );

	bool Up() const		{ return m_up; }
	bool Down() const	{ return !m_up; }
	void SetDeco( const RenderAtom& atom, const RenderAtom& atomD )			{ m_atoms[DECO] = atom; m_atoms[DECO_D] = atomD; SetState(); Modify(); }
	void GetDeco( RenderAtom* atom, RenderAtom* atomD )						{ if ( atom ) *atom = m_atoms[DECO]; if ( atomD ) *atomD = m_atoms[DECO_D]; }
	void SetDecoRotationY( float degrees )									{ m_deco.SetRotationY( degrees ); }
	void SetIcon( const RenderAtom& atom, const RenderAtom& atomD )			{ m_atoms[ICON] = atom; m_atoms[ICON_D] = atomD; SetState(); Modify(); }

	void SetText( const char* text );
	const char* GetText() const { return m_label.GetText(); }

	enum {
		CENTER, LEFT, RIGHT
	};
	void SetTextLayout( int alignment, float dx=0.0f, float dy=0.0f )		{ m_textLayout = alignment; m_textDX = dx; m_textDY = dy; Modify(); }
	void SetDecoLayout( int alignment, float dx=0.0f, float dy=0.0f )		{ m_decoLayout = alignment; m_decoDX = dx; m_decoDY = dy; Modify(); }

	virtual const RenderAtom& GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );

	virtual PushButton* ToPushButton() { return 0; }
	virtual ToggleButton* ToToggleButton() { return 0; }

protected:

	void Init( Gamui* gamui, const ButtonLook& look ) {
		RenderAtom nullAtom;
		Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
	}

	void Init(	Gamui*,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled );

	Button();

	bool m_up;
	void SetState();


private:

	void PositionChildren();

	enum {
		UP,
		UP_D,
		DOWN,
		DOWN_D,
		DECO,
		DECO_D,
		ICON,
		ICON_D,
		COUNT
	};
	RenderAtom m_atoms[COUNT];
	
	Image		m_face;
	Image		m_deco;
	Image		m_icon;

	int			m_textLayout;
	int			m_decoLayout;
	float		m_textDX;
	float		m_textDY;
	float		m_decoDX;
	float		m_decoDY;	
	TextLabel	m_label;
};


class PushButton : public Button
{
public:
	PushButton() : Button()	{}
	PushButton( Gamui* gamui,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled) : Button()	
	{
		Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
	}

	PushButton( Gamui* gamui, const ButtonLook& look ) : Button()
	{
		RenderAtom nullAtom;
		Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
	}


	void Init( Gamui* gamui, const ButtonLook& look ) {
		RenderAtom nullAtom;
		Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
	}

	void Init(	Gamui* gamui,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled )
	{
		Button::Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
	}

	virtual ~PushButton()	{}

	virtual bool CanHandleTap()											{ return true; }
	virtual bool HandleTap( TapAction action, float x, float y );
	virtual PushButton* ToPushButton() { return this; }
};


class ToggleButton : public Button
{
public:
	ToggleButton() : Button(), m_next( 0 ), m_prev( 0 ), m_wasUp( true ), m_subItemArr(0)		{}
	ToggleButton(	Gamui* gamui,
					const RenderAtom& atomUpEnabled,
					const RenderAtom& atomUpDisabled,
					const RenderAtom& atomDownEnabled,
					const RenderAtom& atomDownDisabled,
					const RenderAtom& decoEnabled, 
					const RenderAtom& decoDisabled) : Button(), m_next( 0 ), m_prev( 0 ), m_wasUp( true ), m_subItemArr(0)
	{
		Button::Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
		m_prev = m_next = this;
	}

	ToggleButton( Gamui* gamui, const ButtonLook& look ) : Button(), m_next( 0 ), m_prev( 0 ), m_wasUp( true ), m_subItemArr(0)
	{
		RenderAtom nullAtom;
		Button::Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
		m_prev = m_next = this;
	}

	void Init( Gamui* gamui, const ButtonLook& look ) {
		RenderAtom nullAtom;
		Button::Init( gamui, look.atomUpEnabled, look.atomUpDisabled, look.atomDownEnabled, look.atomDownDisabled, nullAtom, nullAtom );
		m_prev = m_next = this;
	}

	void Init(	Gamui* gamui,
				const RenderAtom& atomUpEnabled,
				const RenderAtom& atomUpDisabled,
				const RenderAtom& atomDownEnabled,
				const RenderAtom& atomDownDisabled,
				const RenderAtom& decoEnabled, 
				const RenderAtom& decoDisabled )
	{
		Button::Init( gamui, atomUpEnabled, atomUpDisabled, atomDownEnabled, atomDownDisabled, decoEnabled, decoDisabled );
		m_prev = m_next = this;
	}

	virtual ~ToggleButton();

	virtual bool CanHandleTap()											{ return true; }
	virtual bool HandleTap(	TapAction action, float x, float y );

	void AddToToggleGroup( ToggleButton* );
	void RemoveFromToggleGroup();
	bool InToggleGroup();

	// Add a sub-menu that only appears when this is Down. Can
	// Can be anything; additional buttons, icons, etc.
	void AddSubItem( UIItem* item );
	void RemoveSubItem( UIItem* item );

	void SetUp()								{ m_up = true; SetState(); ProcessToggleGroup(); }
	void SetDown()								{ m_up = false; SetState(); ProcessToggleGroup(); }

	virtual void Clear();
	virtual ToggleButton* ToToggleButton() { return this; }

	//virtual bool DoLayout();

private:
	void ProcessToggleGroup();
	//void ProcessSubGroup();
	void PriSetUp()								{ m_up = true;  SetState(); }
	void PriSetDown()							{ m_up = false; SetState(); }

	ToggleButton* m_next;
	ToggleButton* m_prev;
	bool m_wasUp;
	PODArray< UIItem* >* m_subItemArr;
};


class DigitalBar : public UIItem
{
public:
	DigitalBar();
	virtual ~DigitalBar()		{}

	void Init(	Gamui* gamui, 
				const RenderAtom& atomLower,		// lit
				const RenderAtom& atomHigher );		// un-lit

	// t between 0 and 1. The lower atom will be used
	// below t, the higher atom above
	void SetRange( float t, int bar=0 );
	float GetRange(int bar = 0) const					{ GAMUIASSERT(bar == 0 || bar == 1); return m_t[bar]; }

	virtual float Width() const;
	virtual float Height() const;
	virtual void SetVisible( bool visible );
	void SetSize( float w, float h );

	void EnableDouble(bool doubleBar)	{ if (doubleBar != m_double) { m_double = doubleBar; Modify(); } }
	enum {LOWER, HIGHER};
	void SetAtom( int which, const RenderAtom&, int bar=0 );

	void SetText( const char* text )		{ m_textLabel.SetText( text ); }

	virtual const RenderAtom& GetRenderAtom() const;
	virtual bool DoLayout();
	virtual void Queue( PODArray< uint16_t > *index, PODArray< Gamui::Vertex > *vertex );

private:
	float		m_t[2];
	bool		m_double;
	float		m_width, m_height;
	TextLabel	m_textLabel;
	enum {NUM_IMAGE = 4};
	Image		m_image[NUM_IMAGE];
};


class LayoutCalculator
{
public:
	LayoutCalculator( float screenWidth=1000, float screenHeight=1000 );
	~LayoutCalculator();

	float Width() const									{ return width; }
	float Height() const								{ return height; }

	void SetSize( float width, float height )			{ this->width = width; this->height = height; }
	void SetGutter( float gutterX, float gutterY )		{ this->gutterX = gutterX; this->gutterY = gutterY; }
	float GutterX() const								{ return gutterX; }
	float GutterY() const								{ return gutterY; }

	void SetSpacing( float spacing )					{ this->spacingX = this->spacingY = spacing; }
	void SetSpacing( float spacingX, float spacingY )	{ this->spacingX = spacingX; this->spacingY = spacingY; }
	float SpacingX() const								{ return spacingX; }
	float SpacingY() const								{ return spacingY; }

	void SetOffset( float x, float y )			{ this->offsetX = x; this->offsetY = y; }
	void SetTextOffset( float x, float y )		{ this->textOffsetX = x; this->textOffsetY = y; }

	void PosAbs( IWidget* item, int x, int y, int dx=1, int dy=1, bool setSize=true ) const;
	void PosAbs( TextLabel* label, int x, int y ) const {
		useTextOffset = true;
		PosAbs( (UIItem*) label, x, y ); 
		useTextOffset = false;
	}

private:
	float Max( float a, float b ) const { return a > b ? a : b; }
	float Min( float a, float b ) const { return a < b ? a : b; }

	float screenWidth;
	float screenHeight;
	float width;
	float height;
	float gutterX;
	float gutterY;
	float spacingX;
	float spacingY;
	float textOffsetX;
	float textOffsetY;
	float offsetX;
	float offsetY;
	mutable bool  useTextOffset;
};

class Matrix
{
  public:
	Matrix()		{	x[0] = x[5] = x[10] = x[15] = 1.0f;
						x[1] = x[2] = x[3] = x[4] = x[6] = x[7] = x[8] = x[9] = x[11] = x[12] = x[13] = x[14] = 0.0f; 
					}

	void SetTranslation( float _x, float _y, float _z )		{	x[12] = _x;	x[13] = _y;	x[14] = _z;	}

	void SetXRotation( float thetaDegree );
	void SetYRotation( float thetaDegree );
	void SetZRotation( float thetaDegree );

	float x[16];

	inline friend Matrix operator*( const Matrix& a, const Matrix& b )
	{	
		Matrix result;
		MultMatrix( a, b, &result );
		return result;
	}


	inline friend void MultMatrix( const Matrix& a, const Matrix& b, Matrix* c )
	{
	// This does not support the target being one of the sources.
		GAMUIASSERT( c != &a && c != &b && &a != &b );

		// The counters are rows and columns of 'c'
		for( int i=0; i<4; ++i ) 
		{
			for( int j=0; j<4; ++j ) 
			{
				// for c:
				//	j increments the row
				//	i increments the column
				*( c->x + i +4*j )	=   a.x[i+0]  * b.x[j*4+0] 
									  + a.x[i+4]  * b.x[j*4+1] 
									  + a.x[i+8]  * b.x[j*4+2] 
									  + a.x[i+12] * b.x[j*4+3];
			}
		}
	}

	inline friend void MultMatrix( const Matrix& m, const float* v, int components, float* out )
	{
		float w = 1.0f;
		for( int i=0; i<components; ++i )
		{
			*( out + i )	=		m.x[i+0]  * v[0] 
								  + m.x[i+4]  * v[1]
								  + m.x[i+8]  * v[2]
								  + m.x[i+12] * w;
		}
	}
};


};


#endif // GAMUI_INCLUDED
