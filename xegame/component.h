#ifndef XENOENGINE_COMPONENT_INCLUDED
#define XENOENGINE_COMPONENT_INCLUDED

// The primary components:
class PositionComponent;
class RenderComponent;


class Component
{
public:
	virtual void OnAdd( Chit* chit )	{}
	virtual void OnRemove()				{}

	virtual SpatialComponent*	ToSpatial()		{ return 0; }
	virtual RenderComponent*	ToRender()		{ return 0; }

	virtual bool NeedsTick()					{ return false; }
	virtual void DoTick( U32 delta )			{}
};


#endif // XENOENGINE_COMPONENT_INCLUDED