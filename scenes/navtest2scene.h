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

#ifndef NAVTEST2_SCENE_INCLUDED
#define NAVTEST2_SCENE_INCLUDED

#include "../grinliz/glrandom.h"
#include "../xegame/scene.h"
#include "../game/lumoschitbag.h"

class LumosGame;
class Engine;
class WorldMap;

class NavTest2SceneData : public SceneData
{
public:
	NavTest2SceneData( const char* _worldFilename, int _nChits, int _nPerCreation=1 ) : worldFilename( _worldFilename ), nChits( _nChits ), nPerCreation( _nPerCreation ) {}

	const char* worldFilename;
	int nChits;
	int nPerCreation;
};


class NavTest2Scene : public Scene
{
public:
	NavTest2Scene( LumosGame* game, const NavTest2SceneData* );
	~NavTest2Scene();

	virtual void DoTick( U32 deltaTime );

	virtual void Resize();
	void Zoom( int style, float delta );
	void Rotate( float degrees );
	void Pan(int action, const grinliz::Vector2F& view, const grinliz::Ray& world);
	void MoveCamera(float dx, float dy);

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world );
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void Draw3D( U32 deltaTime );
	virtual void DrawDebugText();
	virtual void MouseMove( const grinliz::Vector2F& view, const grinliz::Ray& world ) { debugRay = world; }

private:
	void LoadMap();
	void CreateChit( const grinliz::Vector2I& p );
	void DoCheck();

	gamui::PushButton okay;
	gamui::ToggleButton regionButton;

	ChitContext context;
	const NavTest2SceneData* data;

	grinliz::Random random;
	grinliz::CDynArray<grinliz::Vector2I> waypoints;
	grinliz::CDynArray<int> chits;
	grinliz::Ray debugRay;
	
	int creationTick;
};



#endif // NAVTEST2_SCENE_INCLUDED
