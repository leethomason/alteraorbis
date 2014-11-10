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

#ifndef WORLDGEN_SCENE_INCLUDED
#define WORLDGEN_SCENE_INCLUDED

#include "../xegame/scene.h"
#include "../gamui/gamui.h"
#include "../engine/texture.h"

#include "../script/worldgen.h"
#include "../script/rockgen.h"
#include "../game/newsconsole.h"

class LumosGame;
class WorldMap;
class Sim;

class WorldGenScene : public Scene, public ITextureCreator
{
public:
	WorldGenScene( LumosGame* game );
	~WorldGenScene();

	virtual void Resize();
	virtual void Tap( int action, const grinliz::Vector2F& screen, const grinliz::Ray& world )				
	{
		ProcessTap( action, screen, world );
	}
	virtual void ItemTapped( const gamui::UIItem* item );
	virtual void DoTick( U32 step );

	void CreateTexture( Texture* t );
	virtual void DeActivate();

private:
	enum { 
		NZONE = 4,
		NUM_ZONES = NZONE*NZONE
	};

	void SetMapBright(bool bright);
	void BlendLine( int y );

	WorldGen*	worldGen;
	RockGen*	rockGen;
	WorldMap*	worldMap;
	U16*		pix16;

	struct GenState {
		void Clear() { mode=NOT_STARTED; y=0; }
		enum {
			NOT_STARTED,
			GEN_NOTES,
			WORLDGEN,
			ROCKGEN_START,
			ROCKGEN,
			SIM_START,
			SIM_TICK,
			SIM_DONE,
			DONE
		};
		int mode;
		int y;
	};
	GenState genState;
	Sim* sim;
	grinliz::GLString	simStr;
	gamui::PushButton	okay, cancel;
	gamui::Image		worldImage;
	gamui::TextLabel	headerText;				// Top label 
	gamui::TextLabel	footerText;				// bottom label
	gamui::TextLabel	statText;			// (at this time) mostly debug.
	NewsConsole			newsConsole;
};

#endif // WORLDGEN_SCENE_INCLUDED