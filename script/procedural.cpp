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


void FaceGen::GetSkinColor( int index0, int index1, float fade, Color4F* color )
{
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


void FaceGen::GetGlassesColor( int index0, int index1, float fade, Color4F* color )
{
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


void FaceGen::GetHairColor( int index0, Color4F* color )
{
	static const Vector2I c[NUM_HAIR_COLORS] = {
		// natural
		{ PAL_GREEN*2, PAL_RED },
		{ PAL_RED*2+0, PAL_TANGERINE },
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_RED*2+1, PAL_GREEN },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
//		{ PAL_TANGERINE*2+1, PAL_GRAY },
		// intense
		{ PAL_RED*2+1, PAL_RED },
		{ PAL_ZERO*2+1, PAL_TANGERINE },
		{ PAL_ZERO*2+1, PAL_BLUE },
		{ PAL_GRAY*2, PAL_GRAY }
	};

	const Game::Palette* palette = Game::GetMainPalette();
	index0 = abs(index0) % NUM_HAIR_COLORS;
	*color = palette->Get4F( c[index0].x, c[index0].y );
}


void FaceGen::GetColors( U32 seed, grinliz::Vector4F* v )
{
	Color4F c[3];
	GetColors( seed, c );
	for( int i=0; i<3; ++i ) {
		v[i].Set( c[i].r, c[i].g, c[i].b, c[i].a );
	}
}

	
void FaceGen::GetColors( U32 seed, grinliz::Color4F* c )
{
	Random random( seed );
	random.Rand();

	GetSkinColor(	random.Rand( NUM_SKIN_COLORS ), 
					random.Rand( NUM_SKIN_COLORS ),
					random.Uniform(), 
					c+SKIN );
	GetHairColor( random.Rand( NUM_HAIR_COLORS ), c+HAIR );
	GetGlassesColor( random.Rand( NUM_GLASSES_COLORS ),
					 random.Rand( NUM_GLASSES_COLORS ),
					 random.Uniform(),
					 c+GLASSES );
}


void FaceGen::Render( int seed, ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Vector4F vcol[3];
	GetColors( random.Rand(), vcol );

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

	array[BASE]		= palette->Get4F( c[i*3+0].x, c[i*3+0].y );
	array[CONTRAST] = palette->Get4F( c[i*3+1].x, c[i*3+1].y );
	array[EFFECT]	= palette->Get4F( effect.x, effect.y );

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
