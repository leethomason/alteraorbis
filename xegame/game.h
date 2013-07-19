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

#ifndef UFOATTACK_GAME_INCLUDED
#define UFOATTACK_GAME_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glperformance.h"
#include "../grinliz/glcontainer.h"

#include "../engine/surface.h"
#include "../engine/texture.h"
#include "../engine/model.h"
#include "../engine/uirendering.h"
#include "../engine/screenport.h"

#include "../tinyxml2/tinyxml2.h"
#include "../shared/gamedbreader.h"
#include "../gamui/gamui.h"

#include "cgame.h"

#include <limits.h>

class ParticleSystem;
class Scene;
class SceneData;
class ISceneResult;
class ItemDef;
class TiXmlDocument;
class Stats;
class Unit;
class Research;

enum SavePathMode {
	SAVEPATH_READ,
	SAVEPATH_WRITE
};


/*
*/

class Game : public ITextureCreator, public grinliz::IPerformancePrinter 
{
public:
	Game( int width, int height, int rotation, int uiHeight, const char* savepath );
	virtual ~Game();

	void DeviceLoss();
	void Resize( int width, int height, int rotation );
	void DoTick( U32 msec );

	void Tap( int action, int x, int y, int mod );
	void Zoom( int style, float distance );
	void Rotate( float degreesFromStart );
	void CancelInput();

	virtual Scene* CreateScene( int id, SceneData* data ) = 0;
	
	// debugging / testing / mapmaker
	void MouseMove( int x, int y );
	void HandleHotKey( int mask );

	void RotateSelection( int delta );
	void DeleteAtSelection();
	void DeltaCurrentMapItem( int d );

	void PushScene( int sceneID, SceneData* data );
	void PopScene( int result = INT_MAX );
	void PopAllAndLoad( int slot );

	enum { MAX_SCENES = 1000 };
	bool IsScenePushed() const		{ return sceneQueued.sceneID != MAX_SCENES; }

	U32 CurrentTime() const	{ return currentTime; }
	U32 DeltaTime() const	{ return currentTime-previousTime; }

	void SetDebugLevel( int level )		{ debugLevel = (level%4); }
	int GetDebugLevel() const			{ return debugLevel; }
	void SetPerfLevel( int level )		{ perfLevel = (level%2); }
	int GetPerfLevel() const			{ return perfLevel; }
	int GetTapMod() const				{ return tapMod; }
	bool DebugUIEnabled() const			{ return debugUIEnabled; }

	const char* GamePath( const char* type, int slot, const char* extension ) const;
	bool HasFile( const char* file ) const;
	void DeleteFile( const char* file );

	bool PopSound( int* database, int* offset, int* size );

	const gamedb::Reader* GetDatabase()	{ return database0; }
	const Screenport& GetScreenport() const { return screenport; }
	Screenport* GetScreenportMutable() { return &screenport; }

	enum {
		ATOM_TEXT, ATOM_TEXT_D,
		ATOM_COUNT
	};
	const gamui::RenderAtom& GetRenderAtom( int id );
	gamui::RenderAtom CreateRenderAtom( int uiRendering, const char* assetName, float x0=0, float y0=0, float x1=1, float y1=1 );
	
	// For creating some required textures:
	virtual void CreateTexture( Texture* t );

	struct Palette
	{
		const char* name;
		int dx;
		int dy;
		grinliz::CArray< grinliz::Color4U8, 128 > colors;

		grinliz::Color4U8 Get4U8( int x, int y ) const {
			GLASSERT( x >= 0 && x < dx && y >= 0 && y < dy );
			int i = y*dx + x;
			return colors[i];
		}

		grinliz::Color4F Get4F( int x, int y ) const {
			GLASSERT( x >= 0 && x < dx && y >= 0 && y < dy );
			int i = y*dx + x;
			return grinliz::Convert_4U8_4F( colors[i] );
		}

		grinliz::Vector4F GetV4F( int x, int y ) const {
			grinliz::Color4F c = Get4F( x, y );
			grinliz::Vector4F v = { c.r, c.g, c.b, c.a };
			return v;
		}
	};
	const Palette* GetPalette( const char* name=0 ) const;

	// This is a hack; but if there isn't a static method, lots
	// of code needs a game* that only needs a palette. Should
	// get moved out of game to its own thing.
	static const Palette* GetMainPalette()	{ return mainPalette; }

	virtual void PrintPerf( int depth, const grinliz::PerfData& data );

protected:
	void PushPopScene();

private:
	static const Palette* mainPalette;

	// Color palettes
	grinliz::CDynArray< Palette > palettes;

	Screenport screenport;
	Surface surface;	// general purpose memory buffer for handling images


	bool scenePopQueued;

	void LoadTextures();
	void LoadModels();
	void LoadModel( const char* name );
	void LoadAtoms();
	void LoadPalettes();

	int tapMod;
	int currentFrame;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int debugLevel;
	int perfLevel;
	int perfFrameCount;
	bool suppressText;
	bool renderUI;
	bool debugUIEnabled;
	int perfY;

	ModelLoader* modelLoader;
	gamedb::Reader* database0;		// the basic, complete database

	struct SceneNode {
		Scene*			scene;
		int				sceneID;
		SceneData*		data;
		int				result;

		SceneNode() : scene( 0 ), sceneID( MAX_SCENES ), data( 0 ), result( INT_MIN )	{}

		void Free();
		void SendResult();
	};
	void CreateSceneLower( const SceneNode& in, SceneNode* node );
	SceneNode sceneQueued;
	CStack<SceneNode> sceneStack;

	U32 currentTime;
	U32 previousTime;
	bool isDragging;

	int rotTestStart;
	int rotTestCount;
	grinliz::GLString savePath;

	gamui::RenderAtom renderAtoms[ATOM_COUNT];
};




#endif
