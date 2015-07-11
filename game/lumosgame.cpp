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

#include "lumosgame.h"
#include "gamelimits.h"
#include "worldinfo.h"
#include "layout.h"
#include "../markov/markov.h"

#include "../scenes/titlescene.h"
#include "../scenes/dialogscene.h"
#include "../scenes/rendertestscene.h"
#include "../scenes/colortestscene.h"
#include "../scenes/particlescene.h"
#include "../scenes/navtestscene.h"
#include "../scenes/navtest2scene.h"
#include "../scenes/battletestscene.h"
#include "../scenes/animationscene.h"
#include "../scenes/livepreviewscene.h"
#include "../scenes/worldgenscene.h"
#include "../scenes/gamescene.h"
#include "../scenes/characterscene.h"
#include "../scenes/mapscene.h"
#include "../scenes/forgescene.h"
#include "../scenes/censusscene.h"
#include "../scenes/soundscene.h"
#include "../scenes/creditsscene.h"
#include "../scenes/fluidtestscene.h"

using namespace grinliz;
using namespace gamui;

LumosGame::LumosGame(  int width, int height, int rotation ) 
	: Game( width, height, rotation, 600 )
{
	InitButtonLooks();
	CoreScript::Init();

	PushScene( SCENE_TITLE, 0 );
	PushPopScene();
}


LumosGame::~LumosGame()
{
	CoreScript::Free();
	TextureManager::Instance()->TextureCreatorInvalid( this );
}


void LumosGame::InitButtonLooks()
{
//	TextureManager* tm = TextureManager::Instance();

	static const char* UP[2]   = { "buttonUp", "tabUp" };
	static const char* DOWN[2] = { "buttonDown", "tabDown" };

	for( int i=0; i<1; ++i ) {
		RenderAtom blueUp = LumosGame::CalcUIIconAtom( UP[i], true );
		RenderAtom blueUpD = blueUp;
		blueUpD.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

		RenderAtom blueDown = LumosGame::CalcUIIconAtom( DOWN[i], true );
		RenderAtom blueDownD = blueDown;
		blueDownD.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;

		buttonLookArr[i].Init( blueUp, blueUpD, blueDown, blueDownD );
	}
}


Scene* LumosGame::CreateScene( int id, SceneData* data )
{
	Scene* scene = 0;

	switch ( id ) {
	case SCENE_TITLE:		scene = new TitleScene( this );				break;
	case SCENE_DIALOG:		scene = new DialogScene( this );			break;
	case SCENE_RENDERTEST:	scene = new RenderTestScene( this, (const RenderTestSceneData*)data );		break;
	case SCENE_COLORTEST:	scene = new ColorTestScene(this);			break;
	case SCENE_PARTICLE:	scene = new ParticleScene( this );			break;
	case SCENE_NAVTEST:		scene = new NavTestScene( this );			break;
	case SCENE_NAVTEST2:	scene = new NavTest2Scene( this, (const NavTest2SceneData*)data );			break;
	case SCENE_BATTLETEST:	scene = new BattleTestScene( this );		break;
	case SCENE_ANIMATION:	scene = new AnimationScene( this );			break;
	case SCENE_LIVEPREVIEW:	scene = new LivePreviewScene( this, (const LivePreviewSceneData*)data );	break;
	case SCENE_WORLDGEN:	scene = new WorldGenScene( this );			break;
	case SCENE_GAME:		scene = new GameScene( this );				break;
	case SCENE_CHARACTER:	scene = new CharacterScene( this, (CharacterSceneData*)data );				break;
	case SCENE_MAP:			scene = new MapScene( this, (MapSceneData*)data );							break;
	case SCENE_FORGE:		scene = new ForgeScene( this, (ForgeSceneData*)data );						break;
	case SCENE_CENSUS:		scene = new CensusScene( this, (CensusSceneData*)data );					break;
	case SCENE_SOUND:		scene = new SoundScene(this);				break;
	case SCENE_CREDITS:		scene = new CreditsScene(this);				break;
	case SCENE_FLUID:		scene = new FluidTestScene(this);			break;

	default:
		GLASSERT( 0 );
		break;
	}
	return scene;
}


void LumosGame::CreateTexture( Texture* t )
{
	Game::CreateTexture( t );
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


RenderAtom LumosGame::CalcUIIconAtom( const char* name, bool enabled, float* ratio )
{
	const Texture* texture = TextureManager::Instance()->GetTexture( "uiIcon" );
	if ( !name || !*name ) {
		name = "default";
	}

	Texture::TableEntry te;
	RenderAtom atom;

	if (texture->HasTableEntry(name)) {
		texture->GetTableEntry(name, &te);
		RenderAtom a((const void*)(UIRenderer::RENDERSTATE_UI_DECO),
			texture,
			te.uv.x, te.uv.y, te.uv.z, te.uv.w);
		atom = a;

		if (ratio) {
			*ratio = (float(texture->Width()) * (atom.tx1 - atom.tx0)) / (float(texture->Height()) * (atom.ty1 - atom.ty0));
		}

		if (!enabled) {
			atom.renderState = (const void*)UIRenderer::RENDERSTATE_UI_DECO_DISABLED;
		}
	}
	else {
		//GLASSERT(false);	// safe, but expected to work. 
							//Actually this gets called when we are looking at things that don't have images...
							//will pretty easily see the bug in-game.
	}
	return atom;
}


RenderAtom LumosGame::CalcDeityAtom(int team)
{
	switch (team) {
		case DEITY_MOTHER_CORE:		return CalcUIIconAtom("motherCore");
		case DEITY_Q:				return CalcUIIconAtom("qcore");
		case DEITY_R1K:				return CalcUIIconAtom("r1kcore");
		case DEITY_TRUULGA:			return CalcUIIconAtom("truulgacore");
		default: GLASSERT(0); break;
	}
	return RenderAtom();
}


RenderAtom LumosGame::CalcPaletteAtom( int x, int y )
{
	const Game::Palette* p = Game::GetMainPalette();

	float u = ((float)x+0.5f)/(float)p->dx;
	float v = 1.0f - ((float)y+0.5f)/(float)p->dy;

	//Vector2I c = { 0, 0 };
	Texture* texture = TextureManager::Instance()->GetTexture( "palette" );

	// FIXME: should be normal_opaque?
	RenderAtom atom( (const void*)(UIRenderer::RENDERSTATE_UI_NORMAL), (const void*)texture, u, v, u, v );
	return atom;
}


RenderAtom LumosGame::CalcIconAtom( const char* asset )
{
	Texture* texture = TextureManager::Instance()->GetTexture( "icon" );
	Texture::TableEntry te;
	texture->GetTableEntry( asset, &te );
	GLASSERT( !te.name.empty() );

	RenderAtom atom( (const void*)(UIRenderer::RENDERSTATE_UI_NORMAL), (const void*)texture, te.uv.x, te.uv.y, te.uv.z, te.uv.w );
	return atom;
}





void LumosGame::CopyFile(const char* src, const char* target)
{
	GLString srcPath;
	GetSystemPath(GAME_APP_DIR, src, &srcPath);

	GLString targetPath;
	GetSystemPath(GAME_SAVE_DIR, target, &targetPath);

	FILE* fp = fopen(srcPath.c_str(), "rb");
	GLASSERT(fp);
	if (fp) {
		FILE* tp = fopen(targetPath.c_str(), "wb");
		GLASSERT(tp);

		if (fp && tp) {
			CDynArray<U8> buf;
			fseek(fp, 0, SEEK_END);
			size_t size = ftell(fp);
			buf.PushArr(size);

			fseek(fp, 0, SEEK_SET);
			size_t didRead = fread(&buf[0], 1, size, fp);
			GLASSERT(didRead == 1);
			if (didRead == 1) {
				fwrite(buf.Mem(), 1, size, tp);
			}

			fclose(tp);
		}
		fclose(fp);
	}
}


void LumosGame::ItemToButton( const GameItem* item, gamui::Button* button )
{
	CStr<64> text;

	// Set the text to the proper name, if we have it.
	// Then an icon for what it is, and a check
	// mark if the object is in use.
	int value = item->GetValue();
	const char* name = item->ProperName() ? item->ProperName() : item->Name();
	if ( value ) {
		text.Format( "%s\n%d", name, value );
	}
	else {
		text.Format( "%s\n ", name );
	}
	button->SetText( text.c_str() );

	IString decoName = item->keyValues.GetIString( "uiIcon" );
	RenderAtom atom  = LumosGame::CalcUIIconAtom( decoName.c_str(), true );
	atom.renderState = (const void*) UIRenderer::RENDERSTATE_UI_DISABLED;
	RenderAtom atomD = LumosGame::CalcUIIconAtom( decoName.c_str(), false );

	button->SetDeco( atom, atomD );
}


void LumosGame::Save()
{
	for (SceneNode* node = sceneStack.BeginTop(); node; node = sceneStack.Next()) {
		if (node->sceneID == SCENE_GAME) {
			((GameScene*)(node->scene))->Save();
			break;
		}
	}
}
