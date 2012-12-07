#include "procedural.h"
#include "../grinliz/glvector.h"
#include "../game/gameitem.h"

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


void FaceGen::GetSkinColor( int index0, int index1, float fade, Color4F* color, Color4F* highlight )
{
	static const Vector2I c[NUM_SKIN_COLORS] = {
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		{ PAL_TANGERINE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_TANGERINE }
	};

	index0 = abs(index0) % NUM_SKIN_COLORS;
	index1 = abs(index1) % NUM_SKIN_COLORS;

	*color = Lerp( palette->Get4F( c[index0].x, c[index0].y ),
				   palette->Get4F( c[index1].x, c[index1].y ),
				   fade );
	*highlight = palette->Get4F( PAL_GRAY*2+1, PAL_GRAY );
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
		{ PAL_TANGERINE*2+1, PAL_GRAY },
		// intense
		{ PAL_RED*2+1, PAL_RED },
		{ PAL_ZERO*2+1, PAL_TANGERINE },
		{ PAL_ZERO*2+1, PAL_BLUE },
		{ PAL_GRAY*2, PAL_GRAY }
	};

	index0 = abs(index0) % NUM_HAIR_COLORS;
	*color = palette->Get4F( c[index0].x, c[index0].y );
}


void FaceGen::GetColors( U32 seed, grinliz::Color4F* c )
{
	Random random( seed );
	random.Rand();

	GetSkinColor(	random.Rand( NUM_SKIN_COLORS ), 
					random.Rand( NUM_SKIN_COLORS ),
					random.Uniform(), 
					c+SKIN, c+HIGHLIGHT );
	GetHairColor( random.Rand( NUM_HAIR_COLORS ), c+HAIR );
	GetGlassesColor( random.Rand( NUM_GLASSES_COLORS ),
					 random.Rand( NUM_GLASSES_COLORS ),
					 random.Uniform(),
					 c+GLASSES );
}


void FaceGen::Render( const GameItem& item, ProcRenderInfo* info )
{
	U32 seed = item.stats.Hash();
	Random random( seed );
	random.Rand();

	GetColors( random.Rand(), info->color );

	info->vOffset[0] = (float)(FACE_ROWS - random.Rand(FACE_ROWS)) / (float)FACE_ROWS;
	info->vOffset[1] = (float)(EYE_ROWS - random.Rand(EYE_ROWS)) / (float)EYE_ROWS;
	
	// 50% chance of having glasses.
	const int glassesRows = female ? FEMALE_GLASSES_ROWS : MALE_GLASSES_ROWS;

	if ( random.Bit() )
		info->vOffset[2] = 1.0f;
	else
		info->vOffset[2] = (float)(glassesRows - random.Rand(glassesRows)) / (float)glassesRows;

	info->vOffset[3] = (float)(HAIR_ROWS - random.Rand(HAIR_ROWS)) / (float)HAIR_ROWS;
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

	array[BASE]		= palette->Get4F( c[i*3+0].x, c[i*3+0].y );
	array[CONTRAST] = palette->Get4F( c[i*3+1].x, c[i*3+1].y );
	array[EFFECT]	= palette->Get4F( effect.x, effect.y );
	array[GLOW]		= palette->Get4F( c[i*3+2].x, c[i*3+2].y );

	array[BASE].a		= 0;
	array[CONTRAST].a	= 0;
	array[EFFECT].a		= (fire || shock) ? 0.7f : 0.0f;
	array[GLOW].a		= 0.7f;
}


void WeaponGen::Render( const GameItem& item, ProcRenderInfo* info )
{
	U32 seed = item.stats.Hash();
	Random random( seed );
	random.Rand();

	GetColors(	random.Rand(), 
				(item.flags & GameItem::EFFECT_FIRE) != 0, 
				(item.flags & GameItem::EFFECT_SHOCK) != 0, 
				info->color );

	info->vOffset[0] = (float)( NUM_ROWS - random.Rand(NUM_ROWS)) / (float)(NUM_ROWS);
	info->vOffset[1] = (float)( NUM_ROWS - random.Rand(NUM_ROWS)) / (float)(NUM_ROWS);
	info->vOffset[2] = (float)( NUM_ROWS - random.Rand(NUM_ROWS)) / (float)(NUM_ROWS);
	info->vOffset[3] = (float)( NUM_ROWS - random.Rand(NUM_ROWS)) / (float)(NUM_ROWS);	

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
	if ( name == "roundsToGlow" ) {
		return PROCEDURAL_ROUNDS_TO_GLOW;
	}
	else if ( name == "ring" ) {
		return PROCEDURAL_RING;
	}
	return PROCEDURAL_NONE;
}


grinliz::IString ItemGen::ToName( int id )
{
	switch( id ) {
	case PROCEDURAL_ROUNDS_TO_GLOW:	return StringPool::Intern( "roundsToGlow", true );
	case PROCEDURAL_RING:			return StringPool::Intern( "ring", true );
	default:
		break;
	}
	return IString();
}


/* static */ int ItemGen::RenderItem( const Game::Palette* palette, const GameItem& item, ProcRenderInfo* info )
{
	int result = NONE;
	switch ( item.procedural ) {
		case PROCEDURAL_ROUNDS_TO_GLOW:
		{
			info->color[0].Set( 1, 1, 1, item.RoundsFraction() );
			result = COLOR_XFORM;
		}
		break;

		case PROCEDURAL_RING:
		{
			WeaponGen gen( palette );
			gen.Render( item, info );
			result = PROC4;
		}
		break;

		default:
			GLASSERT( 0 );
			break;
	}
	return result;
}
