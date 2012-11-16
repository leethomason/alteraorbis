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


