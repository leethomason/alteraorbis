#include "lumosgame.h"

using namespace grinliz;
using namespace gamui;



LumosGame::LumosGame(  int width, int height, int rotation, const char* savepath ) : Game( width, height, rotation, savepath )
{
}


LumosGame::~LumosGame()
{}


Scene* LumosGame::CreateScene( int id, SceneData* data )
{
	GLASSERT( 0 );
	return 0;
}


RenderAtom LumosGame::CalcParticleAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 16 );
	Texture* texture = TextureManager::Instance()->GetTexture( "particleQuad" );
	int y = id / 4;
	int x = id - y*4;
	float tx0 = (float)x / 4.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/4.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom LumosGame::CalcIconAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons" );
	int y = id / 8;
	int x = id - y*8;
	float tx0 = (float)x / 8.f;
	float ty0 = (float)y / 4.f;
	float tx1 = tx0 + 1.f/8.f;
	float ty1 = ty0 + 1.f/4.f;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}


RenderAtom LumosGame::CalcIcon2Atom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id < 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "icons2" );

	static const int CX = 4;
	static const int CY = 4;

	int y = id / CX;
	int x = id - y*CX;
	float tx0 = (float)x / (float)CX;
	float ty0 = (float)y / (float)CY;;
	float tx1 = tx0 + 1.f/(float)CX;
	float ty1 = ty0 + 1.f/(float)CY;

	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}

RenderAtom LumosGame::CalcDecoAtom( int id, bool enabled )
{
	GLASSERT( id >= 0 && id <= 32 );
	Texture* texture = TextureManager::Instance()->GetTexture( "iconDeco" );
	float tx0 = 0;
	float ty0 = 0;
	float tx1 = 0;
	float ty1 = 0;

	if ( id != DECO_NONE ) {
		int y = id / 8;
		int x = id - y*8;
		tx0 = (float)x / 8.f;
		ty0 = (float)y / 4.f;
		tx1 = tx0 + 1.f/8.f;
		ty1 = ty0 + 1.f/4.f;
	}
	return RenderAtom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_DECO : UIRenderer::RENDERSTATE_UI_DECO_DISABLED), (const void*)texture, tx0, ty0, tx1, ty1 );
}

RenderAtom LumosGame::CalcPaletteAtom( int c0, int c1, int blend, bool enabled )
{
	Vector2I c = { 0, 0 };
	Texture* texture = TextureManager::Instance()->GetTexture( "palette" );

	static const int offset[5] = { 75, 125, 175, 225, 275 };
	static const int subOffset[3] = { 0, -12, 12 };

	if ( blend == PALETTE_NORM ) {
		if ( c1 > c0 )
			Swap( &c1, &c0 );
		c.x = offset[c0];
		c.y = offset[c1];
	}
	else {
		if ( c0 > c1 )
			Swap( &c0, &c1 );

		if ( c0 == c1 ) {
			// first column special:
			c.x = 25 + subOffset[blend];
			c.y = offset[c1];
		}
		else {
			c.x = offset[c0] + subOffset[blend];;
			c.y = offset[c1];
		}
	}
	RenderAtom atom( (const void*)(enabled ? UIRenderer::RENDERSTATE_UI_NORMAL : UIRenderer::RENDERSTATE_UI_DISABLED), (const void*)texture, 0, 0, 0, 0 );
	UIRenderer::SetAtomCoordFromPixel( c.x, c.y, c.x, c.y, 300, 300, &atom );
	return atom;
}
