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

#ifndef LUMOS_WORLD_MAP_INCLUDED
#define LUMOS_WORLD_MAP_INCLUDED

#include "gamelimits.h"
#include "worldgrid.h"
#include "sectorport.h"

#include "../engine/map.h"
#include "../engine/rendertarget.h"
#include "../engine/shadermanager.h"

#include "../micropather/micropather.h"

#include "../grinliz/glrectangle.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glmemorypool.h"
#include "../grinliz/glbitarray.h"

#include "../tinyxml2/tinyxml2.h"

class Texture;
class WorldInfo;
class Model;
class ModelResource;
class ChitBag;
class SectorData;
class DamageDesc;
class NewsHistory;
class GameItem;

/*
	Remembering Y is up and we are in the xz plane:

	+--------x
	|
	|
	|
	|
	z

	1. The (0,0) world origin is in the upper left.
	2. This is annoying, but it's important to keep sane.
	3. If the mark is used it should display, save, and load in the upper left
*/

/*
	The world map has layers. (May add more in the future.)
	Basic layer: water or land.
	For land, is it passable or not?
	In the future, flowing water, magma, etc. would be great.

	Rendering is currently broken into strips. (Since the map
	doesn't change after the Init...() is called.) Could be
	further optimized to use regions.
*/
class WorldMap : public Map, 
	             public micropather::Graph,
				 public IDeviceLossHandler
{
public:
	WorldMap( int width, int height );
	~WorldMap();

	virtual WorldMap* ToWorldMap() { return this; }

	// Call to turn on rock rendering and make map aware of "InUse"
	void AttachEngine( Engine* engine, IMapGridUse* iMapInUse  );
	void AttachHistory(NewsHistory* h) { newsHistory = h; }

	// Test initiliazation:
	void InitCircle();
	bool InitPNG( const char* filename, 
				  grinliz::CDynArray<grinliz::Vector2I>* blocks,
				  grinliz::CDynArray<grinliz::Vector2I>* wayPoints,
				  grinliz::CDynArray<grinliz::Vector2I>* features );
	// Init from WorldGen data:
	void MapInit( const U8* land, const U16* path );

	void SavePNG( const char* path );
	void Save( const char* filename );
	void Load( const char* filename );

	// Set the rock to h.
	//		h= 1 to 3 rock
	//		h= 0 no rock
	//		h=-1 sets to nominal value.
	//		h=-2 sets to initial value, used when loading
	void SetRock( int x, int y, int h, bool magma, int rockType );
	void SetMagma( int x, int y, bool magma ) {
		int index = INDEX( x, y );
		WorldGrid wg = grid[index];
		SetRock( x, y, wg.RockHeight(), magma, wg.RockType() );
	}
	void SetPave( int x, int y, int pave ) {
		int index = INDEX(x,y);
		const WorldGrid& wg = grid[index];
		if ( wg.Land() == WorldGrid::LAND && wg.Height() == 0 ) {
			grid[index].SetPave(pave);
		}
	}
	void SetPorch( int x, int y, int id ) {
		int index = INDEX( x, y );
		grid[index].SetPorch( id );
	}
	void SetPlant(int x, int y, int typeBase1, int stage);
	void SetWorldGridHP(int x, int y, int hp) {
		grid[INDEX(x, y)].SetHP(hp);
	}

	const WorldGrid& GetWorldGrid(int x, int y) { return grid[INDEX(x, y)]; }
	const WorldGrid& GetWorldGrid(const grinliz::Vector2I& p) { return grid[INDEX(p.x, p.y)]; }

	grinliz::Vector2I FindPassable( int x, int y );

	void ResetPather( int x, int y );
	void UpdateBlock( int x, int y );	// tell the map to check for blocks. will call ResetPather() if needed
	bool IsPassable( int x, int y ) const;
	
	bool IsLand( int x, int y )		{ 
		int i = INDEX(x,y);
		return grid[i].IsLand(); 
	}
	int ContainsWaterfall(const grinliz::Rectangle2I& b) const;
	
	// Call the pather; return true if successful.
	bool CalcPath(	const grinliz::Vector2F& start, 
					const grinliz::Vector2F& end, 
					grinliz::CDynArray< grinliz::Vector2F > *path,
					float* totalCost,
					bool showDebugging = false );

	// Calculate a path assuming that the position at 'end' is blocked,
	// but we can get to a neighbor.
	bool CalcPathBeside(	const grinliz::Vector2F& start, 
							const grinliz::Vector2F& end, 
							grinliz::Vector2F* bestEnd,
							float* totalCost );

	// Uses the very fast straight line pather.
	bool HasStraightPath( const grinliz::Vector2F& start, 
						  const grinliz::Vector2F& end );

	// Returns the nearest pathable port to 'pos'. Returns (0,0) on failure.
	SectorPort NearestPort( const grinliz::Vector2F& pos );
	SectorPort RandomPort( grinliz::Random* random );
	// Debugging. Makes the 'RandomPort' not random.
	void SetRandomPort( SectorPort sp ) { randomPortDebug = sp; }

	enum BlockResult {
		NO_EFFECT,
		FORCE_APPLIED,
		STUCK
	};

	// Calculate the effect of walls on 'pos'. Note that
	// there can be multiple walls, and this takes multiple calls.
	BlockResult CalcBlockEffect(	const grinliz::Vector2F& pos,
									float radius,
									grinliz::Vector2F* force );
	// Call CalcBlockEffect and return the result.
	BlockResult ApplyBlockEffect(	const grinliz::Vector2F inPos, 
									float radius, 
									grinliz::Vector2F* outPos );

	// ---- Map ---- //
	// Texture positions
	enum {
		ROCK,
		POOL,
		MAGMA,
		ICE
	};
	virtual void Submit( GPUState* shader, bool emissiveOnly );
	virtual void PrepVoxels(const SpaceTree*, Model** root, const grinliz::Plane* planes6);
	virtual void DrawVoxels( GPUState* shader, const grinliz::Matrix4* xform );
	virtual void PrepGrid( const SpaceTree* spaceTree );
	virtual void Draw3D(  const grinliz::Color3F& colorMult, StencilMode, bool useSaturation );
	virtual grinliz::Vector3I IntersectVoxel(	const grinliz::Vector3F& origin,
												const grinliz::Vector3F& dir,
												float length,				
												grinliz::Vector3F* at ); 

	// ---- Device Loss --- //
	virtual void DeviceLoss();

	// Brings water & waterfalls current
	void DoTick( U32 delta, ChitBag* chitBag );
	void VoxelHit(const grinliz::Vector2I& voxel, const DamageDesc& dd)
	{
		grinliz::Vector3I v = { voxel.x, 0, voxel.y };
		VoxelHit(v, dd);
	}
	void VoxelHit(const grinliz::Vector3I& voxel, const DamageDesc& dd);

	// ---- MicroPather ---- //
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd );
	virtual void  AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent );
	virtual void  PrintStateInfo( void* state );

	// --- Debugging -- //
	void ShowAdjacentRegions( float x, float y );
	void ShowRegionPath( float x0, float y0, float x1, float y1 );
	bool IsShowingRegionOverlay() const							{ return debugRegionOverlay.Area() > 1; }
	void ShowRegionOverlay( const grinliz::Rectangle2I& over )	{ debugRegionOverlay = over; }
	void PatherCacheHitMiss( const grinliz::Vector2I& sector, micropather::CacheData* data );
	int CalcNumRegions();

	// --- MetaData --- //
	const WorldInfo& GetWorldInfo()			{ return *worldInfo; }
	WorldInfo* GetWorldInfoMutable()		{ return worldInfo; }
	const SectorData* GetSectorData() const;
	const SectorData& GetSector( int mapx, int mapy ) const;
	const SectorData& GetSector( const grinliz::Vector2I& sector ) const;
	void SetSectorName(const grinliz::Vector2I& sector, const grinliz::IString& name);

	// Find random land on the largest continent
	grinliz::Color4U8 Pixel( int x, int y )	{ 
		return grid[y*width+x].ToColor();
	}

	/* A 16x16 zone, needs 3 bits to describe the depth. From the depth
	   can infer the origin, etc.
		d=0 16
		d=1 8
		d=2 4
		d=3 2
		d=4 1
	*/
	enum {
		ZONE_SIZE	= 16,		// adjust size of bit fields to be big enough to represent this
		ZONE_SHIFT  = 4,
		ZONE_SIZE2  = ZONE_SIZE*ZONE_SIZE,
		NUM_ZONES	= MAX_MAP_SIZE/ZONE_SIZE,
	    NUM_ZONES2	= NUM_ZONES*NUM_ZONES,
		MAX_VOXEL_QUADS  = 16000,		// actually uses quads, so the vertex=4*MAX_VOXEL_QUADS
	};

	virtual void CreateTexture( Texture* t );

	int plantCount[NUM_PLANT_TYPES][MAX_PLANT_STAGES];

	int CountPlants() const {
		int c = 0;
		for (int i = 0; i<NUM_PLANT_TYPES; ++i)
			for (int j = 0; j<MAX_PLANT_STAGES; ++j)
				c += plantCount[i][j];
		return c;
	}

private:
	int INDEX( int x, int y ) const { 
		GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height ); 
		return y*width + x; 
	}
	int INDEX( grinliz::Vector2I v ) const { return INDEX( v.x, v.y ); }
	float IndexToRotation360(int index);

	int ZDEX( int x, int y ) const { 
		GLASSERT( x >= 0 && x < width ); GLASSERT( y >= 0 && y < height );
		x /= ZONE_SIZE;
		y /= ZONE_SIZE;
		return (y*width/ZONE_SIZE) + x; 
	} 

	void ProcessZone( ChitBag* cb );
	void EmitWaterfalls( U32 delta );	// particle systems

	void Init( int w, int h );
	void FreeVBOs();

	void CalcZone( int x, int y );
	void DeleteAllRegions();

	void DrawZones();			// debugging
	void DrawTreeZones();
	void ClearDebugDrawing();	// debugging
	void DumpRegions();

	// The solver has 3 components:
	//	Vector path:	the final result, a collection of points that form connected vector
	//					line segments.
	//	Grid path:		intermediate; a path checked by a step walk between points on the grid
	//  State path:		the micropather computed region

	bool GridPath( const grinliz::Vector2F& start, const grinliz::Vector2F& end );

	grinliz::Vector2I ZoneOrigin( int x, int y ) const {
		const WorldGrid& g = grid[INDEX(x,y)];
		grinliz::Vector2I origin = g.ZoneOrigin( x, y );
		return origin;
	}

	WorldGrid* ZoneOriginG( int x, int y ) {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return &grid[INDEX(v)];
	}

	bool IsZoneOrigin( int x, int y ) const {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return v.x == x && v.y == y;
	}

	grinliz::Rectangle2F ZoneBounds( int x, int y ) const {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		const WorldGrid& g = grid[INDEX(x,y)];
		grinliz::Rectangle2F b;
		b.min.Set( (float)v.x, (float)v.y );
		int size = g.ZoneSize();
		b.max.Set( (float)(v.x + size), (float)(v.y + size) );
		return b;
	}

	grinliz::Vector2F ZoneCenter( int x, int y ) const {
		grinliz::Rectangle2F r = ZoneBounds( x, y );
		return r.Center();
	}

	void* ToState( int x, int y ) {
		grinliz::Vector2I v = ZoneOrigin( x, y );
		return (void*)(v.y*width+v.x);
	}

	WorldGrid* ToGrid( void* state, grinliz::Vector2I* vec ) {
		int v = (int)(state);
		if ( vec ) {
			int y = v / width;
			int x = v - y*width;
			vec->Set( x, y );
		}
		GLASSERT( v < width*height );
		return grid+v;
	}

	void PushQuad( int layer, int x, int y, int w, int h, grinliz::CDynArray<PTVertex>* vertex, grinliz::CDynArray<U16>* index );
	void PushVoxel( int id, float x, float y, float h, const float* walls );
	Vertex* PushVoxelQuad( int id, const grinliz::Vector3F& normal );
	void PushTree(Model** root, int x, int y, int type0Based, int stage, float hpFraction);

	int IntersectPlantAtVoxel( const grinliz::Vector3I& voxel,
		const grinliz::Vector3F& origin, const grinliz::Vector3F& dir, float length, grinliz::Vector3F* at);
	void ProcessEffect( ChitBag* chitBag );	// on slow tick

	WorldGrid*					grid;
	Engine*						engine;
	IMapGridUse*				iMapGridUse;
	int							slowTick;
	NewsHistory*				newsHistory;

	WorldInfo*					worldInfo;
	grinliz::Rectangle2I		debugRegionOverlay;
	SectorPort					randomPortDebug;

	micropather::MicroPather*	currentPather;
	micropather::MicroPather* PushPather( const grinliz::Vector2I& sector );
	void PopPather() {
		GLASSERT( currentPather );
		currentPather = 0;
	}

	MP_VECTOR< void* >						pathRegions;
	grinliz::CDynArray< grinliz::Vector2F >	debugPathVector;
	grinliz::CDynArray< grinliz::Vector2F >	pathCache;

	class CompValue {
	public:
		static U32 Hash( const grinliz::Vector2I& v)			{ return (U32)(v.y*MAX_MAP_SIZE+v.x); }
		static bool Equal( const grinliz::Vector2I& v0, const grinliz::Vector2I& v1 )	{ return v0 == v1; }
	};

	GPUVertexBuffer*				voxelVertexVBO;
	Texture*						voxelTexture;
	GPUVertexBuffer*				gridVertexVBO;
	Texture*						gridTexture;
	int								nVoxels;
	int								nGrids;
	int								nTrees;	// we don't necessarily use all the trees in the treePool

	struct PlantEffect {
		bool operator==(const PlantEffect& rhs) const { return rhs.voxel == voxel; }
		void Serialize(XStream* xs);
		grinliz::Vector2I voxel;
		bool fire;
		bool shock;
	};

	// List of interesting things that need to be processed each frame.
	grinliz::CDynArray< grinliz::Vector2I > waterfalls;
	grinliz::CDynArray< grinliz::Vector2I > magmaGrids;
	grinliz::CDynArray< PlantEffect >	plantEffect;		// plants on fire/shock

	// Memory pool of models to use for tree rendering.
	grinliz::CDynArray< Model* > treePool;

	// Temporaries to avoid allocation
	grinliz::CDynArray< grinliz::Vector2I > waterStack;
	grinliz::CDynArray< grinliz::Vector2I > poolGrids;

	grinliz::BitArray< NUM_ZONES, NUM_ZONES, 1 > zoneInit;		// pather
	grinliz::BitArray< NUM_ZONES, NUM_ZONES, 1 > voxelInit;		// rendering

	// Temporary - big one - last in class
	grinliz::CArray< Vertex, MAX_VOXEL_QUADS*4 > voxelBuffer;
};

#endif // LUMOS_WORLD_MAP_INCLUDED
