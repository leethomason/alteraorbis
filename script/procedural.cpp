#include "procedural.h"
#include "../grinliz/glvector.h"
#include "../game/gameitem.h"
#include "../xegame/istringconst.h"

using namespace grinliz;


/*static*/ grinliz::IString ItemGen::ProcIDToName( int id )
{
	static const char* procNames[PROC_COUNT] = {
		"main",
		"guard",
		"triad",
		"blade"
	};
	GLASSERT( id >= 0 && id < PROC_COUNT );
	return StringPool::Intern( procNames[id], true );
}


HumanGen::HumanGen( bool female, U32 seed, int team, bool electric )
{
	this->female = female;
	this->seed   = seed;
	this->team   = team;
	this->electric = electric;
}


void HumanGen::GetSkinColor( int index0, int index1, float fade, Color4F* color )
{
	static const int NUM_SKIN_COLORS = 4;
	static const Vector2I c[NUM_SKIN_COLORS] = {
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		{ PAL_TANGERINE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_TANGERINE }
	};

	index0 = abs(index0) % NUM_SKIN_COLORS;
	index1 = abs(index1) % NUM_SKIN_COLORS;

	const Game::Palette* palette = Game::GetMainPalette();
		
	*color = Lerp( palette->Get4F( c[index0].x, c[index0].y ),
				   palette->Get4F( c[index1].x, c[index1].y ),
				   fade );
}


void HumanGen::GetGlassesColor( int index0, int index1, float fade, Color4F* color )
{
	static const int NUM_GLASSES_COLORS = 6;
	static const Vector2I c[NUM_GLASSES_COLORS] = {
		{ PAL_BLUE*2, PAL_ZERO },
		{ PAL_BLUE*2, PAL_RED },
		{ PAL_BLUE*2, PAL_GREEN },
		{ PAL_ZERO*2+1, PAL_BLUE },
		{ PAL_RED*2+1, PAL_BLUE },
		{ PAL_RED*2+1, PAL_PURPLE }
	};

	index0 = abs(index0) % NUM_GLASSES_COLORS;
	index1 = abs(index1) % NUM_GLASSES_COLORS; 

	const Game::Palette* palette = Game::GetMainPalette();

	*color = Lerp( palette->Get4F( c[index0].x, c[index0].y ),
				   palette->Get4F( c[index1].x, c[index1].y ),
				   fade );
}


void HumanGen::GetHairColor( int index0, Color4F* color )
{
	static const int N_FEMALE = 9;
	static const int N_MALE   = 7;
	
	static const Vector2I cFemale[N_FEMALE] = {
		// natural
		{ PAL_GREEN*2,		PAL_RED },
		{ PAL_RED*2+0,		PAL_TANGERINE },
		{ PAL_PURPLE*2,		PAL_TANGERINE },
		{ PAL_RED*2+1,		PAL_GREEN },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		// intense
		{ PAL_RED*2+1,		PAL_RED },
		{ PAL_ZERO*2+1,		PAL_TANGERINE },
		{ PAL_ZERO*2+1,		PAL_BLUE },
		{ PAL_GRAY*2,		PAL_GRAY }
	};

	static const Vector2I cMale[N_MALE] = {
		// natural
		{ PAL_GREEN*2, PAL_RED },
		{ PAL_RED*2+0, PAL_TANGERINE },
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_RED*2+1, PAL_GREEN },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		// intense
		{ PAL_ZERO*2+1, PAL_TANGERINE },
		{ PAL_GRAY*2,   PAL_GRAY }
	};

	const Game::Palette* palette = Game::GetMainPalette();
	if ( female ) {
		index0 = abs(index0) % N_FEMALE;
		*color = palette->Get4F( cFemale[index0].x, cFemale[index0].y );
	}
	else {
		index0 = abs(index0) % N_MALE;
		*color = palette->Get4F( cMale[index0].x, cMale[index0].y );
	}
}


void HumanGen::GetSuitColor( grinliz::Vector4F* c )
{
	Random random( seed );
	int y = random.Rand( PAL_COUNT );
	int x0 = y + random.Rand( PAL_COUNT - y );
	int x1 = y + random.Rand( PAL_COUNT - y );
	float fraction = random.Uniform();

	const Game::Palette* palette = Game::GetMainPalette();
	Vector4F c0 = palette->GetV4F( x0*2, y );
	Vector4F c1 = palette->GetV4F( x1*2, y );

	*c = Lerp( c0, c1, fraction );
	c->w = 0.7f;
}


void HumanGen::GetColors( grinliz::Vector4F* v )
{
	Color4F c[3];
	GetColors( c );
	for( int i=0; i<3; ++i ) {
		v[i].Set( c[i].r, c[i].g, c[i].b, c[i].a );
	}
}

	
void HumanGen::GetColors( grinliz::Color4F* c )
{
	Random random( seed );
	random.Rand();

	GetSkinColor(	random.Rand(), 
					random.Rand(),
					random.Uniform(), 
					c+SKIN );
	GetHairColor( random.Rand(), c+HAIR );
	GetGlassesColor( random.Rand(),
					 random.Rand(),
					 random.Uniform(),
					 c+GLASSES );

	if ( electric ) {
		c[SKIN] = c[SKIN] + c[HAIR]*0.5f + c[GLASSES]*0.5f;
		c[HAIR] = c[HAIR] + c[GLASSES]*0.5f;
	}
}


void HumanGen::AssignFace( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Vector4F vcol[3];
	GetColors( vcol );

	// Get the texture to get the the metadata
	Texture* texture = 0;
	if ( female ) {
		texture = TextureManager::Instance()->GetTexture( "humanFemaleFaceAtlas" );
	}
	else {
		texture = TextureManager::Instance()->GetTexture( "humanMaleFaceAtlas" );
	}
	GLASSERT( texture );

	info->texture = texture;
	texture->GetTableEntry( random.Rand( texture->NumTableEntries() ), &info->te );

	info->color.SetCol( 0, vcol[0] );
	info->color.SetCol( 1, vcol[1] );
	info->color.SetCol( 2, vcol[2] );
}


void HumanGen::AssignSuit( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Vector4F vcol[3];
	GetColors( vcol );
	GetSuitColor( &vcol[2] );

	// Get the texture to get the the metadata
	Texture* texture = 0;
	if ( female ) {
		texture = TextureManager::Instance()->GetTexture( "femsuit0" );
	}
	else {
		texture = TextureManager::Instance()->GetTexture( "suit0" );
	}
	GLASSERT( texture );

	info->texture = texture;
	//texture->GetTableEntry( random.Rand( texture->NumTableEntries() ), &info->te );

	vcol[0].w = 0;
	vcol[1].w = 0;
	vcol[2].w = 0.7f;	// suit glow

	info->color.SetCol( 0, vcol[0] );
	info->color.SetCol( 1, vcol[1] );
	info->color.SetCol( 2, vcol[2] );
}


void WeaponGen::GetColors( int i, bool fire, bool shock, grinliz::Color4F* array )
{
	// red for fire
	// green for shock/corruption? 
	static const Vector2I c[NUM_COLORS*3] = {
		{ PAL_GREEN*2, PAL_ZERO },		{ PAL_GREEN*2, PAL_ZERO },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2, PAL_ZERO },		{ PAL_GREEN*2, PAL_RED },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2+1, PAL_BLUE },	{ PAL_GREEN*2+1, PAL_BLUE },	{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2+1, PAL_BLUE },	{ PAL_GREEN*2+1, PAL_PURPLE },	{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2+1, PAL_PURPLE },	{ PAL_GREEN*2+1, PAL_PURPLE },	{ PAL_GREEN*2, PAL_GRAY },
		{ PAL_BLUE*2, PAL_RED },		{ PAL_BLUE*2, PAL_RED },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_PURPLE*2, PAL_RED },		{ PAL_BLUE*2, PAL_RED },		{ PAL_PURPLE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_PURPLE },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_BLUE },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_TANGERINE },	{ PAL_BLUE*2+1, PAL_GRAY },
	};

	i = abs(i) % NUM_COLORS;

	Vector2I effect = c[i*3+2];
	if ( fire && shock ) {
		effect.Set( 1, PAL_TANGERINE );
	}
	else if ( fire ) {
		effect.Set( 1, PAL_RED );
	}
	else if ( shock ) {
		effect.Set( 1, PAL_GREEN );
	}

	const Game::Palette* palette = Game::GetMainPalette();

	Color4F base		= palette->Get4F( c[i*3+0].x, c[i*3+0].y );
	array[BASE]			= base;
	Color4F contrast	= palette->Get4F( c[i*3+1].x, c[i*3+1].y );
	static const float LERP = 0.65f;
	array[CONTRAST].Set(	Lerp( base.r, contrast.r, LERP ),
							Lerp( base.g, contrast.g, LERP ),
							Lerp( base.b, contrast.b, LERP ),
							Lerp( base.a, contrast.a, LERP ) );
							
	array[EFFECT]		= palette->Get4F( effect.x, effect.y );

	array[BASE].a		= 0;
	array[CONTRAST].a	= 0;
	array[EFFECT].a		= (fire || shock) ? 0.7f : 0.0f;
}


void WeaponGen::Render( int seed, const GameItem& item, ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Color4F c[3];
	GetColors(	random.Rand(), 
				(item.flags & GameItem::EFFECT_FIRE) != 0, 
				(item.flags & GameItem::EFFECT_SHOCK) != 0, 
				c );

	for( int i=0; i<3; ++i ) {
		Vector4F v = { c[i].r, c[i].g, c[i].b, c[i].a };
		info->color.SetCol( i, v );
	}
	info->color.m44 = 0;

	const ModelResource* resource = ModelResourceManager::Instance()->GetModelResource( item.ResourceName() );
	GLASSERT( resource );
	Texture* texture = resource->atom[0].texture;
	GLASSERT( texture );

	info->texture = texture;
	texture->GetTableEntry( random.Rand( texture->NumTableEntries() ), &info->te );
	
	info->filter[PROC_RING_MAIN] = true;
	info->filter[PROC_RING_GUARD] = random.Boolean();
	info->filter[PROC_RING_TRIAD] = random.Boolean();
	info->filter[PROC_RING_BLADE] = random.Boolean();

	for( int i=0; i<4; ++i ) {
		info->filterName[i] = ProcIDToName( PROC_RING_MAIN+i );
	}
}



int ItemGen::ToID( IString name )
{
	if ( name == "ring" ) {
		return PROCEDURAL_RING;
	}
	return PROCEDURAL_NONE;
}


grinliz::IString ItemGen::ToName( int id )
{
	switch( id ) {
	case PROCEDURAL_RING:			return StringPool::Intern( "ring", true );
	default:
		break;
	}
	return IString();
}


/*static*/ bool ItemGen::ProceduralRender( int seed, const GameItem& item, ProcRenderInfo* info )
{
	if (    item.resource == IStringConst::kring 
		 || item.resource == IStringConst::klargeRing ) 
	{
		WeaponGen wg;
		wg.Render( seed, item, info );
		return true;
	}
	return false;
}


void TeamGen::Assign( int seed, ProcRenderInfo* info )
{
	static const int NUM = 4;
	static const Vector4I colors[NUM] = {
		// approved
		{ PAL_BLUE*2, PAL_GREEN,			PAL_GREEN*2, PAL_GREEN },
		{ PAL_TANGERINE*2, PAL_TANGERINE,	PAL_GREEN*2, PAL_TANGERINE },
		{ PAL_GREEN*2, PAL_TANGERINE,		PAL_TANGERINE*2, PAL_TANGERINE },
		{ PAL_RED*2, PAL_RED,				PAL_GREEN*2, PAL_RED },

		// fail
		//{ PAL_RED*2, PAL_RED,				PAL_TANGERINE*2, PAL_RED },
		//{ PAL_BLUE*2, PAL_BLUE,				PAL_PURPLE*2, PAL_BLUE }
	};

	info->texture = TextureManager::Instance()->GetTexture( "structure" );

	int index = seed % NUM;
	bool select = (seed / NUM) & 1 ? true : false; 

	const Game::Palette* palette = Game::GetMainPalette();

	Vector4F base		= palette->GetV4F( colors[index].x, colors[index].y );
	Vector4F contrast	= palette->GetV4F( colors[index].z, colors[index].w );
	Vector4F glow		= select ? base : contrast;

	// Grey colors for neutral:
	if ( seed == 0 ) {
		base		= palette->GetV4F( 0, PAL_GRAY );	// dark gray
		contrast	= palette->GetV4F( 0, PAL_GRAY );
		glow		= base;
	}

	base.w		= 0;
	contrast.w	= 0;
	glow.w		= 0.7f;

	info->color.SetCol( 0, base );
	info->color.SetCol( 1, contrast );
	info->color.SetCol( 2, glow );
	info->color.m44 = 0;
}


void AssignProcedural( const char* name,
					   bool female, U32 seed, int team, bool electric,
					   ProcRenderInfo* info )
{
	if ( !name || !*name )
		return;

	if ( StrEqual( name, "team" )) {
		TeamGen gen;
		gen.Assign( team, info );
	}
	else if ( StrEqual( name, "suit" )) {
		HumanGen gen( female, seed, team, electric );
		gen.AssignSuit( info );
	}
	else {
		GLASSERT( 0 );
	}
}

