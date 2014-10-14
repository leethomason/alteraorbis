#include "itemdescwidget.h"
#include "../game/gameitem.h"
#include "../game/news.h"
#include "../grinliz/glstringutil.h"
#include "../script/battlemechanics.h"
#include "../xegame/chitbag.h"

using namespace gamui;
using namespace grinliz;

void ItemDescWidget::Init( gamui::Gamui* gamui )
{
	text.Init( gamui );
}


void ItemDescWidget::SetPos( float x, float y )
{
	float x1 = x + layout.Width()*2.0f + layout.SpacingX()*2.0f;

	text.SetPos( x, y );
	text.SetTab( layout.Width()*2.0f + layout.SpacingX()*2.0f );
	text.SetBounds( layout.Width()*4.0f + layout.SpacingX()*4.0f, 0 );
}


void ItemDescWidget::SetVisible( bool v )
{
	text.SetVisible( v );
}


void ItemDescWidget::SetInfo( const GameItem* item, const GameItem* user, bool showPersonality, ChitBag* chitBag )
{
	textBuffer.Clear();
	CStr<64> str;

	/*			Ranged		Melee	Shield
		STR		Dam			Dam		Capacity
		WILL	EffRange	D/TU	Reload
		CHR		Clip/Reload	x		x
		INT		Ranged D/TU x		x
		DEX		Melee D/TU	x		x
	*/

	int i = 0;

	const RangedWeapon* rangedWeapon = item->ToRangedWeapon();
	const MeleeWeapon* meleeWeapon = item->ToMeleeWeapon();
	const Shield* shield = item->ToShield();

	if (rangedWeapon) {
		str.Format("Ranged Damage\t%.1f\n", rangedWeapon->Damage());
		textBuffer += str.c_str();
		
		float radAt1 = BattleMechanics::ComputeRadAt1( user, rangedWeapon, false, false );
		str.Format( "Effective Range\t%.1f\n", BattleMechanics::EffectiveRange( radAt1 ));
		textBuffer += str.c_str();

		str.Format( "Fire Rate\t%.1f\n", 1.0f / (0.001f * (float)rangedWeapon->CooldownTime()));
		textBuffer += str.c_str();

		str.Format( "Clip/Reload\t%d / %.1f\n", rangedWeapon->ClipCap(), 0.001f * (float)rangedWeapon->ReloadTime() );
		textBuffer += str.c_str();

		str.Format( "Ranged D/S\t%.1f\n", BattleMechanics::RangedDPTU( rangedWeapon, false ));
		textBuffer += str.c_str();
	}

	if (meleeWeapon) {
		DamageDesc dd;
		BattleMechanics::CalcMeleeDamage( user, meleeWeapon, &dd );

		str.Format( "Melee Damage\t%.1f\n", dd.damage );
		textBuffer += str.c_str();

		str.Format( "Melee D/S\t%.1f\n", BattleMechanics::MeleeDPTU( user, meleeWeapon ));
		textBuffer += str.c_str();

		float boost = meleeWeapon->ShieldBoost();
		if ( boost > 1.0f ) {
			str.Format( "Shield Boost\t%02d%%\n", int(100.0f * boost) - 100 );
			textBuffer += str.c_str();
		}
	}
	if ( shield ) {
		str.Format( "Capacity\t%d\n", (int)shield->Capacity() );
		textBuffer += str.c_str();

		str.Format("Reload\t%.1f\n", 0.001f * (float)shield->ShieldRechargeTime());
		textBuffer += str.c_str();
	}

	if ( !(meleeWeapon || rangedWeapon || shield)) {
		str.Format( "Strength\t%d\n", item->Traits().Strength() );
		textBuffer += str.c_str();

		str.Format( "Will\t%d\n", item->Traits().Will() );
		textBuffer += str.c_str();

		str.Format( "Charisma\t%d\n", item->Traits().Charisma() );
		textBuffer += str.c_str();

		str.Format( "Intelligence\t%d\n", item->Traits().Intelligence() );
		textBuffer += str.c_str();

		str.Format( "Dexterity\t%d\n", item->Traits().Dexterity() );
		textBuffer += str.c_str();
	}

	textBuffer += '\n';

	if (!shortForm) {
		if (showPersonality && item->GetPersonality().HasPersonality()) {
			GLString pdesc;
			item->GetPersonality().Description(&pdesc);
			textBuffer += pdesc.c_str();
			textBuffer += "\n\n";
		}

		MicroDBIterator it(item->historyDB);
		for (; !it.Done(); it.Next()) {

			const char* key = it.Key();
			const char* prefix = "";

			if (StrEqualUntil(key, "Kills", ':')) {
				prefix = MOB_KILLS;
				key += 5;
			}

			if (it.Type() == MicroDB::TYPE_INT) {
				str.Format("%s%s\t%d\n", prefix, key, it.IntValue());
				textBuffer += str.c_str();
			}
		}

		textBuffer += '\n';

		NewsHistory* history = chitBag ? chitBag->GetNewsHistory() : 0;

		if (history && chitBag) {
			GLString s;

			int num = 0;
			const NewsEvent** events = history->Find(item->ID(), true, &num, 0);
			for (int i = 0; i < num; ++i) {
				events[i]->Console(&s, chitBag, item->ID());
				if (!s.empty()) {
					textBuffer += s.c_str();
					textBuffer += '\n';
				}
			}
		}
	}
	text.SetText( textBuffer.c_str() );
}


