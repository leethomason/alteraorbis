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

#ifndef COLORTESTSCENE_INCLUDED
#define COLORTESTSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"

class LumosGame;
class Engine;
class TestMap;
class Model;

class ColorTestScene : public Scene
{
	typedef Scene super;
public:
	ColorTestScene( LumosGame* game );
	virtual ~ColorTestScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Zoom( int style, float normal );
	virtual void Rotate( float degrees );
	virtual void HandleHotKey( int mask );

	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();

private:

	enum {
		HUMAN_MODEL,
		TEMPLE_MODEL,
		SLEEPTUBE_MODEL,
		RING_MODEL,
		PISTOL_MODEL,
		BLASTER_MODEL,
		PULSE_MODEL,
		BEAMGUN_MODEL,
		NUM_MODELS
	};

	void SetupTest();
	void DoProcedural();

	LumosGame* lumosGame;
	gamui::PushButton okay;

	Engine* engine;
	TestMap* testMap;
	Model* model[NUM_MODELS];

	enum {
		SEL_BUILDING_BASE,
		SEL_BUILDING_CONTRAST,
		SEL_BUILDING_GLOW,
		SEL_BASE,
		SEL_CONTRAST,
		NUM_PAL_SEL,
	};
	grinliz::Vector2I	paletteVec[NUM_PAL_SEL];
	gamui::Image		paletteSelector[NUM_PAL_SEL];
	gamui::TextLabel	paletteLabel[NUM_PAL_SEL];
};

#endif // COLORTESTSCENE_INCLUDED
