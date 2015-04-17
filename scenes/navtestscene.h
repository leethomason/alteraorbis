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

#ifndef NAVTESTSCENE_INCLUDED
#define NAVTESTSCENE_INCLUDED

#include "../xegame/scene.h"
#include "../xegame/chitcontext.h"
#include "../xegame/chitbag.h"
#include "../grinliz/glrandom.h"
#include "../grinliz/glbitarray.h"
#include "../engine/map.h"

class LumosGame;
class Engine;
class WorldMap;

class NavTestScene : public Scene, 
					 public IMapGridBlocked
{
public:
	NavTestScene( LumosGame* game );
	~NavTestScene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );
	void Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world);
	void MoveCamera(float dx, float dy);

	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );

	virtual void DrawDebugText();
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world );
	virtual bool MapGridBlocked( int x, int y );

private:
	gamui::PushButton	okay,
						block,
						block20;
	gamui::TextLabel	textLabel;
	gamui::ToggleButton showAdjacent, 
		                showZonePath,
						showOverlay,
						toggleBlock;

	ChitContext		context;
	grinliz::Random random;
	grinliz::Vector3F tapMark;
	grinliz::Ray debugRay;

	enum { NUM_CHITS = 5 };
	enum { SIZE = 32 };
	Chit* chit[NUM_CHITS];

	grinliz::BitArray< SIZE, SIZE, 1 > blocks;
};


#endif // NAVTESTSCENE_INCLUDED