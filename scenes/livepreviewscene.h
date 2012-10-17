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

#ifndef LIVE_PREVIEW_SCENE_INCLUDED
#define LIVE_PREVIEW_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../engine/texture.h"

class LumosGame;
class Engine;
class Model;

class LivePreviewScene : public Scene	//, public ITextureCreator
{
public:
	LivePreviewScene( LumosGame* game );
	virtual ~LivePreviewScene();

	virtual void Resize();

	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void LivePreviewScene::Draw3D( U32 deltaTime );

private:
	void CreateTexture( Texture* t );

	gamui::PushButton okay;

	Engine*		engine;
	Model*		model;

};

#endif // LIVE_PREVIEW_SCENE_INCLUDED