#include "personality.h"
#include "../grinliz/glrandom.h"
#include "../xarchive/glstreamer.h"
#include "gameitem.h"

void Personality::Init()
{
	for( int i=0; i<NUM_TRAITS; ++i ) {
		trait[i] = 10;
	}
}


void Personality::Roll( U32 seed, const GameTrait* t )
{
	grinliz::Random random( seed );
	for( int i=0; i<NUM_TRAITS; ++i ) {
		trait[i] = random.Dice( 3, 6 );
	}

	if ( t ) {
		// Re-roll odd matchups.
		// Intellectual-Physical 
		if ( t->Strength() > t->Intelligence() ) {
			// More physical.
			if ( trait[INT_PHYS] < LOW )
				trait[INT_PHYS] = random.Dice( 3, 6 );
		}
		else {
			// More intellectual.
			if ( trait[INT_PHYS] > HIGH )
				trait[INT_PHYS] = random.Dice( 3, 6 );
		}

		if ( t->Charisma() > HIGH && trait[INTRO_EXTRO] < LOW ) 
			trait[INTRO_EXTRO] = random.Dice( 3, 6 );
		if ( t->Will() > HIGH && trait[NEUROTIC_STABLE] < LOW )
			trait[NEUROTIC_STABLE] = random.Dice( 3,6 );
	}
}


void Personality::Serialize( XStream* xs )
{
	XarcOpen( xs, "Personality" );
	XARC_SER_ARR( xs, trait, NUM_TRAITS );
	XarcClose( xs );
}

