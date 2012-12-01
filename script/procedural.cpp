#include "procedural.h"
#include "../grinliz/glvector.h"

using namespace grinliz;

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
		{ PAL_ZERO*2+1, PAL_BLUE },
		{ PAL_RED*2+1, PAL_BLUE }
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
