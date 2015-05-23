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
#if 0
#ifndef NOISETEST_SCENE_INCLUDED
#define NOISETEST_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../grinliz/glrandom.h"
#include "../engine/texture.h"

class LumosGame;


class NoiseTestScene : public Scene, public ITextureCreator
{
public:
	NoiseTestScene( LumosGame* game );
	virtual ~NoiseTestScene() {}

	virtual void Resize();

	virtual bool Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		return ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );

	void CreateTexture( Texture* t );

private:
	gamui::PushButton okay;
	gamui::Image noiseImage;

	enum { SIZE = 128 };
	U16 buffer[SIZE*SIZE];
	grinliz::Random random;
};

#endif // NOISETEST_SCENE_INCLUDED
#endif
