#include "itemdescwidget.h"
#include "../game/gameitem.h"
#include "../grinliz/glstringutil.h"
#include "../script/battlemechanics.h"

using namespace gamui;
using namespace grinliz;

void ItemDescWidget::Init( gamui::Gamui* gamui )
{
	for( int i=0; i<NUM_TEXT_KV; ++i ) {
		textKey[i].Init( gamui );
		textVal[i].Init( gamui );
	}
}


void ItemDescWidget::SetPos( float x, float y )
{
	float x1 = x + layout.Width()*2.0f + layout.SpacingX()*2.0f;

	for( int i=0; i<NUM_TEXT_KV; ++i ) {
		float y1 =  y + (float)i * layout.Height() * 0.5f;
		textKey[i].SetPos( x,  y1 );
		textVal[i].SetPos( x1, y1 );
	}
}


void ItemDescWidget::SetVisible( bool v )
{
	for( int i=0; i<NUM_TEXT_KV; ++i ) {
		textKey[i].SetVisible( v );
		textVal[i].SetVisible( v );
	}
}


void ItemDescWidget::SetInfo( const GameItem* item, const GameItem* user )
{
	for( int i=0; i<NUM_TEXT_KV; ++i ) {
		textKey[i].SetText( "" );
		textVal[i].SetText( "" );
	}


	CStr<64> str;

	/*			Ranged		Melee	Shield
		STR		Dam			Dam		Capacity
		WILL	EffRange	D/TU	Reload
		CHR		Clip/Reload	x		x
		INT		Ranged D/TU x		x
		DEX		Melee D/TU	x		x
	*/

	int i = 0;
	if ( item->ToRangedWeapon() ) {
		textKey[i].SetText( "Ranged Damage" );
		str.Format( "%.1f", item->traits.Damage() * item->rangedDamage );
		textVal[i++].SetText( str.c_str() );

		textKey[i].SetText( "Effective Range" );
		float radAt1 = BattleMechanics::ComputeRadAt1( user, item->ToRangedWeapon(), false, false );
		str.Format( "%.1f", BattleMechanics::EffectiveRange( radAt1 ));
		textVal[i++].SetText( str.c_str() );

		textKey[i].SetText( "Fire Rate" );
		str.Format( "%.1f", 1.0f / (0.001f * (float)item->cooldown.Threshold()));
		textVal[i++].SetText( str.c_str() );

		textKey[i].SetText( "Clip/Reload" );
		str.Format( "%d / %.1f", item->clipCap, 0.001f * (float)item->reload.Threshold() );
		textVal[i++].SetText( str.c_str() );

		textKey[i].SetText( "Ranged D/TU" );
		str.Format( "%.1f", BattleMechanics::RangedDPTU( item->ToRangedWeapon(), false ));
		textVal[i++].SetText( str.c_str() );
	}
	if ( item->ToMeleeWeapon() ) {
		textKey[i].SetText( "Melee Damage" );
		DamageDesc dd;
		BattleMechanics::CalcMeleeDamage( user, item->ToMeleeWeapon(), &dd );
		str.Format( "%.1f", dd.damage );
		textVal[i++].SetText( str.c_str() );

		textKey[i].SetText( "Melee D/TU" );
		str.Format( "%.1f", BattleMechanics::MeleeDPTU( user, item->ToMeleeWeapon() ));
		textVal[i++].SetText( str.c_str() );

		float boost = BattleMechanics::ComputeShieldBoost( item->ToMeleeWeapon() );
		if ( boost > 1.0f ) {
			textKey[i].SetText( "Shield Boost" );
			str.Format( "%.1f", boost );
			textVal[i++].SetText( str.c_str() );
		}
	}
	if ( item->ToShield() ) {
		textKey[i].SetText( "Capacity" );
		str.Format( "%d", item->clipCap );
		if ( i<NUM_TEXT_KV ) textVal[i++].SetText( str.c_str() );

		textKey[i].SetText( "Reload" );
		str.Format( "%.1f", 0.001f * (float)item->reload.Threshold() );
		if ( i<NUM_TEXT_KV ) textVal[i++].SetText( str.c_str() );
	}

	if ( !(item->ToMeleeWeapon() || item->ToShield() || item->ToRangedWeapon() )) {
		str.Format( "%d", item->traits.Strength() );
		textKey[KV_STR].SetText( "Strength" );
		textVal[KV_STR].SetText( str.c_str() );

		str.Format( "%d", item->traits.Will() );
		textKey[KV_WILL].SetText( "Will" );
		textVal[KV_WILL].SetText( str.c_str() );

		str.Format( "%d", item->traits.Charisma() );
		textKey[KV_CHR].SetText( "Charisma" );
		textVal[KV_CHR].SetText( str.c_str() );

		str.Format( "%d", item->traits.Intelligence() );
		textKey[KV_INT].SetText( "Intelligence" );
		textVal[KV_INT].SetText( str.c_str() );

		str.Format( "%d", item->traits.Dexterity() );
		textKey[KV_DEX].SetText( "Dexterity" );
		textVal[KV_DEX].SetText( str.c_str() );
		i = KV_DEX+1;
	}

	++i;	// put in space
	MicroDBIterator it( item->microdb );
	for( ; !it.Done() && i < NUM_TEXT_KV; it.Next() ) {
		
		const char* key = it.Key();
		if ( it.NumSub() == 1 && it.SubType(0) == 'd' ) {
			textKey[i].SetText( key );
			str.Format( "%d", it.Int(0) );
			textVal[i++].SetText( str.c_str() );
		}
	}
}


