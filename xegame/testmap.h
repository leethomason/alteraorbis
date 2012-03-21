#ifndef TESTMAP_INCLUDED
#define TESTMAP_INCLUDED

#include "../engine/map.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glcolor.h"

class TestMap : public Map
{
public:
	TestMap( int w, int h );
	~TestMap();

	virtual void Draw3D(  const grinliz::Color3F& colorMult, GPUShader::StencilMode );

	void SetColor( const grinliz::Color3F& color ) { this->color = color; }

private:
	grinliz::Vector3F*	vertex;	// 4 vertex quads
	U16*				index;	// 6 index / quad
	grinliz::Color3F	color;
};

#endif // TESTMAP_INCLUDED