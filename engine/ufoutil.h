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

#ifndef UFOATTACK_UTIL_INCLUDED
#define UFOATTACK_UTIL_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glfixed.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glmemorypool.h"


/* Implements a stack as a linked list items.
   Must be c-structs (No constructor, no destructor, can be copied.)
 */
template < class T >
class CStack
{
public:
	CStack() : root( 0 ), current( 0 ), size( 0 ), pool( "CStackPool", sizeof( Node ) ) {
	}
	~CStack() {
		Clear();
	}

	T* Push() {
		Node* n = (Node*)pool.Alloc();
		n->next = root;
		root = n;
		++size;
		return &n->data;
	}
	
	void Pop()		{ 
		GLASSERT( root );
		Node* next = root->next;
		pool.Free( root );
		--size;
		root = next;
	}

	void Clear()	{
		while( !Empty() )
			Pop();
	}
	bool Empty()	{ return root == 0; }

	T* Top()  {
		GLASSERT( root );
		return &root->data;
	}

	T* Bottom() {
		GLASSERT( root );
		Node* b = root;
		while( b->next )
			b = b->next;
		return &b->data;
	}

	int Size() { return size; }

	// in place iteration...with all its problems
	T* BeginTop()	{ current = root; return Current(); }
	T* Current()	{ return current ? &current->data : 0; }
	T* Next()		{ current = current->next; return Current(); }

private:
	struct Node {
		Node* next;
		T	  data;
	};
	Node* root;
	Node* current;
	int size;
	grinliz::MemoryPool pool;
};


class Matrix2I
{
  public:
	int a, b, c, d, x, y;

	Matrix2I()								{	SetIdentity();	}
	Matrix2I( const Matrix2I& rhs )			{	a=rhs.a; b=rhs.b; c=rhs.c; d=rhs.d; x=rhs.x; y=rhs.y; }
	void operator=( const Matrix2I& rhs )	{	a=rhs.a; b=rhs.b; c=rhs.c; d=rhs.d; x=rhs.x; y=rhs.y; }

	// Set the matrix to identity
	void SetIdentity()		{	a=1; b=0; c=0; d=1; x=0; y=0; }

	// valid inputs are 0, 90, 180, and 270
	void SetRotation( int r );
	void SetXZRotation( int r );	// set the rotation, accounting that we are in the xz plane (vs. xy) and the sign of rotation is flipped.
	void SetTranslation( int x, int y )					{ this->x = x;		this->y = y;	}
	void SetTranslation( const grinliz::Vector2I& v )	{ this->x = v.x;	this->y = v.y;	}
	
	void Invert( Matrix2I* inverse ) const;
	//int CalcXZRotation() const;
	void SinCos( int rotation, int* sinTheta, int* cosTheta ) const;


	bool operator==( const Matrix2I& rhs ) const	{ 
		return ( a==rhs.a && b==rhs.b && c==rhs.c && d==rhs.d && x==rhs.x && y==rhs.y );
	}

	bool operator!=( const Matrix2I& rhs ) const	{ 
		return !( a==rhs.a && b==rhs.b && c==rhs.c && d==rhs.d && x==rhs.x && y==rhs.y );
	}

	friend void MultMatrix2I( const Matrix2I& a, const Matrix2I& b, Matrix2I* c );
	friend void MultMatrix2I( const Matrix2I& a, const grinliz::Vector3I& b, grinliz::Vector3I* c );
	friend void MultMatrix2I( const Matrix2I& a, const grinliz::Rectangle2I& b, grinliz::Rectangle2I* c );
	
	friend Matrix2I operator*( const Matrix2I& a, const Matrix2I& b )
	{	
		Matrix2I result;
		MultMatrix2I( a, b, &result );
		return result;
	}
	friend grinliz::Vector3I operator*( const Matrix2I& a, const grinliz::Vector3I& b )
	{
		grinliz::Vector3I result;
		MultMatrix2I( a, b, &result );
		return result;
	}
	friend grinliz::Vector2I operator*( const Matrix2I& a, const grinliz::Vector2I& b )
	{
		grinliz::Vector3I b3 = { b.x, b.y, 1 };
		grinliz::Vector3I r3;
		MultMatrix2I( a, b3, &r3 );

		grinliz::Vector2I result = { r3.x, r3.y };
		return result;
	}
	friend grinliz::Rectangle2I operator*( const Matrix2I& a, const grinliz::Rectangle2I& r ) 
	{
		grinliz::Vector2I arr[3] = { r.max, { r.min.x, r.max.y }, { r.max.x, r.min.y } };
		grinliz::Vector2I v = a * r.min;
		grinliz::Rectangle2I out;
		out.min = out.max = v;

		for( int i=0; i<3; ++i ) {
			v = a * arr[i];
			out.min.x = grinliz::Min( out.min.x, v.x );
			out.min.y = grinliz::Min( out.min.y, v.y );
			out.max.x = grinliz::Max( out.max.x, v.x );
			out.max.y = grinliz::Max( out.max.y, v.y );
		}
		return out;
	}
};


inline void MultMatrix2I( const Matrix2I& x, const Matrix2I& y, Matrix2I* w )
{
	// a b x
	// c d y
	// 0 0 1

	if ( w == &x || w == &y ) {
		Matrix2I copyX = x;
		Matrix2I copyY = y;
		MultMatrix2I( copyX, copyY, w );
	}
	else {
		w->a = x.a*y.a + x.b*y.c;
		w->b = x.a*y.b + x.b*y.d;
		w->c = x.c*y.a + x.d*y.c;
		w->d = x.c*y.b + x.d*y.d;
		w->x = x.a*y.x + x.b*y.y + x.x;
		w->y = x.c*y.x + x.d*y.y + x.y;
	}
}


inline void MultMatrix2I( const Matrix2I& x, const grinliz::Vector3I& y, grinliz::Vector3I* w )
{
	GLASSERT( w != &y );
	w->x = x.a*y.x + x.b*y.y + x.x*y.z;
	w->y = x.c*y.x + x.d*y.y + x.y*y.z;
	w->z =                         y.z;
}


/*
	A class to walk a line between 2 integer points.
*/
class LineWalk
{
public:
	LineWalk( int x0, int y0, int x1, int y1 );
	// (x,y) is the current location, (nx,ny) is the next location.
	int X() const { return (int)p.x; }
	int Y() const { return (int)p.y; }
	int NX() const { return (int)q.x; }
	int NY() const { return (int)q.y; }

	grinliz::Vector2I P() const { grinliz::Vector2I p = { X(), Y() }; return p; }
	grinliz::Vector2I Q() const { grinliz::Vector2I q = { NX(), NY() }; return q; }

	void Step();
	bool Done() const			{ return step > nSteps; }

private:
	int axis;		// 1 if y is major axis. 0 if x.
	int axisDir;	// +1 if positive on the major axis, -1 if negative on the major axis
	int nSteps;		// # of steps in the walk
	int step;		// step==nSteps then done
	grinliz::Fixed delta;	// step in fixed point
	grinliz::Vector2<grinliz::Fixed> p;
	grinliz::Vector2<grinliz::Fixed> q;
};

#endif