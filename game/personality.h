#ifndef PERSONALITY_INCLUDED
#define PERSONALITY_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"

class XStream;
class GameTrait;


/*
	The Personality scale is a variant of the Big-5 personality
	traits. 4 are easier to get your head around, and they are
	changed to be more useful for the game.

	In this case, there isn't meaning to a lower or higher
	score. 10 or 11 is "in between" and scores above or below
	favor behavior on that spectrum.

	Intellectual -	Physical
	Introvert	-	Extrovert
	Planned		-	Impulsive
	Neurotic	-	Stable
*/
class Personality
{
public:
	Personality() { Init(); }
	void Init();
	// Traits effect personality; if passed in, will influence roll.
	void Roll( U32 seed, const GameTrait* );

	void Serialize( XStream* xs );

	bool HasPersonality() const {
		for( int i=0; i<NUM_TRAITS; ++i ) {
			if ( trait[i] != 10 ) return true;
		}
		return false;
	}

	enum {
		LIKES		= 1,
		INDIFFERENT = 0,
		DISLIKES	= -1
	};

	int Botany() const		{ return Sum( 
								Grade( trait[INT_PHYS], -1 ),
								Grade( trait[INTRO_EXTRO], -1 )); }
	int Fighting() const	{ return Grade( trait[INT_PHYS], 1 ); }
	int Guarding() const	{ return Sum(
								Grade( trait[INT_PHYS], 1 ),
								Grade( trait[PLANNED_IMPULSIVE], -1 )); }
	int Crafting() const	{ return Sum(
								Grade( trait[INT_PHYS], -1 ),
								Grade( trait[PLANNED_IMPULSIVE], -1 )); }
	int Spiritual() const	{ return Sum( 
								Grade( trait[INT_PHYS], -1 ),
								Grade( trait[NEUROTIC_STABLE], -1 )); }

	void Description( grinliz::GLString* str ) const;

private:
	int Grade( int v, int dir ) const { 
		if ( v >= HIGH ) return LIKES * dir;
		if ( v <= LOW )  return DISLIKES * dir;
		return INDIFFERENT;
	}
	int Sum( int v0, int v1 ) const { return grinliz::Clamp( (v0+v1)/2, -1, 1 ); }

	enum {
		INT_PHYS,
		INTRO_EXTRO,
		PLANNED_IMPULSIVE,
		NEUROTIC_STABLE,
		NUM_TRAITS,

		LOW  = 8,
		HIGH = 13,
	};
	int trait[NUM_TRAITS];
};


#endif //  PERSONALITY_INCLUDED
