#include "personality.h"
#include "../grinliz/glrandom.h"
#include "../xarchive/glstreamer.h"
#include "gameitem.h"

using namespace grinliz;

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


void Personality::Description( grinliz::GLString* str )
{
	CArray< IString, NUM_TRAITS > descArr;

	if ( trait[INT_PHYS] <= LOW )
		descArr.Push( StringPool::Intern( "intellectual" ));
	else if ( trait[INT_PHYS] >= HIGH )
		descArr.Push( StringPool::Intern( "athlete" ));

	if ( trait[INTRO_EXTRO] <= LOW )
		descArr.Push( StringPool::Intern( "introvert" ));
	else if ( trait[INTRO_EXTRO] >= HIGH )
		descArr.Push( StringPool::Intern( "extrovert" ));

	if ( trait[PLANNED_IMPULSIVE] <= LOW )
		descArr.Push( StringPool::Intern( "planner" ));
	else if ( trait[PLANNED_IMPULSIVE] >= HIGH )
		descArr.Push( StringPool::Intern( "impulsive nature" ));

	if ( trait[NEUROTIC_STABLE] <= LOW )
		descArr.Push( StringPool::Intern( "neurotic" ));
	else if ( trait[NEUROTIC_STABLE] >= HIGH )
		descArr.Push( StringPool::Intern( "stable mind" ));

	if ( descArr.Size() == 0 ) {
		str->Format( "A balanced and centered personality." );
	}
	else if ( descArr.Size() == 1 ) {
		str->Format( "A %s with an otherwise balanced personality.", descArr[0].c_str() );
	}
	else if ( descArr.Size() == 2 ) {
		str->Format( "A %s and %s.", descArr[0].c_str(), descArr[1].c_str() );
	}
	else if ( descArr.Size() == 3 ) {
		str->Format( "A %s, %s, and %s.", descArr[0].c_str(), descArr[1].c_str(), descArr[2].c_str() );
	}
	else {
		str->Format( "A %s, %s, %s, and %s.", descArr[0].c_str(), descArr[1].c_str(), descArr[2].c_str(), descArr[3].c_str() );
	}

	descArr.Clear();
	if ( Botany() == LIKES && descArr.HasCap() )	descArr.Push( StringPool::Intern( "Botany" ));
	if ( Fighting() == LIKES && descArr.HasCap() )	descArr.Push( StringPool::Intern( "Fighting" ));
	if ( Guarding() == LIKES && descArr.HasCap() )	descArr.Push( StringPool::Intern( "Guarding" ));
	if ( Crafting() == LIKES && descArr.HasCap() )	descArr.Push( StringPool::Intern( "Crafting" ));
	if ( Spiritual() == LIKES && descArr.HasCap() )	descArr.Push( StringPool::Intern( "Spirituality" ));

	if ( descArr.Size() ) {
		str->append( " " );
		if ( descArr.Size() == 1 ) {
			str->AppendFormat( "Likes %s.", descArr[0].c_str() );
		}
		else if ( descArr.Size() == 2 ) {
			str->AppendFormat( "Likes %s and %s.", descArr[0].c_str(), descArr[1].c_str() );
		}
		else if ( descArr.Size() == 3 ) {
			str->AppendFormat( "Likes %s, %s, and %s.", descArr[0].c_str(), descArr[1].c_str(), descArr[2].c_str() );
		}
		else {
			str->AppendFormat( "Likes %s, %s, %s, and %s.", descArr[0].c_str(), descArr[1].c_str(), descArr[2].c_str(), descArr[3].c_str() );
		}
	}
}
