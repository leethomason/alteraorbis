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

	void Tap( int action, int x, int y );
	void Zoom( int style, float distance );
	void Rotate( float degreesFromStart );
	void CancelInput();

	// 0 is always the title screen.
	// query for the sub-scene id
	virtual int LoadSceneID() = 0;
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

	FILE* GameSavePath( SavePathMode mode, int slot ) const;
	bool HasSaveFile( int slot ) const;
	void DeleteSaveFile( int slot );
	void SavePathTimeStamp(int slot, grinliz::GLString* stamp );
	int LoadSlot() const				{ return loadSlot; }

	void Load( const tinyxml2::XMLDocument& doc );
	void Save( int slot, bool saveGeo, bool saveTac );

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
	};
	const Palette* GetPalette( const char* name=0 ) const;

	virtual void PrintPerf( int depth, const grinliz::PerfData& data );

protected:
	void PushPopScene();
	static const Palette* mainPalette;

private:

	// Color palettes
	grinliz::CDynArray< Palette > palettes;

	Screenport screenport;
	Surface surface;	// general purpose memory buffer for handling images


	bool scenePopQueued;
	int loadSlot;

	void Init();
	void LoadTextures();
	void LoadModels();
	void LoadModel( const char* name );
	void LoadAtoms();
	void LoadPalettes();

	int currentFrame;
	U32 markFrameTime;
	U32 frameCountsSinceMark;
	float framesPerSecond;
	int debugLevel;
	int perfLevel;
	int perfFrameCount;
	bool suppressText;
	bool renderUI;
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
