#include "moneywidget.h"
#include "../game/lumosgame.h"
#include "../game/wallet.h"

using namespace gamui;
using namespace grinliz;

MoneyWidget::MoneyWidget()
{
}


void MoneyWidget::Init( gamui::Gamui* gamui )
{
	float size = gamui->GetTextHeight() * 2.0f;

	for( int i=0; i<COUNT; ++i ) {
		textLabel[i].Init( gamui );

		static const char* NAMES[COUNT] = { "au", "crystalGreen", "crystalRed", "crystalBlue", "crystalViolet" };
		RenderAtom atom = LumosGame::CalcUIIconAtom( NAMES[i], true );
		image[i].Init( gamui, atom, false );
		image[i].SetSize( size, size );

		textLabel[i].SetText( "0" );
	}
}


void MoneyWidget::SetPos( float x, float y )
{
	float w = image[0].Width();

	for( int i=0; i<COUNT; ++i ) {
		image[i].SetPos( x + w * float(i) * 1.0f, y );
		textLabel[i].SetCenterPos( image[i].CenterX(), image[i].CenterY() );
	}
}


void MoneyWidget::SetVisible( bool vis )
{
	for( int i=0; i<COUNT; ++i ) {
		image[i].SetVisible( vis );
		textLabel[i].SetVisible( vis );
	}
}


void MoneyWidget::Set( const Wallet& wallet )
{
	CStr<16> str;
	str.Format( "%d", wallet.Gold() );
	textLabel[0].SetText( str.c_str() );

	for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
		str.Format( "%d", wallet.Crystal(i) );
		textLabel[i+1].SetText( str.c_str() );
	}

	for( int i=0; i<COUNT; ++i ) {
		textLabel[i].SetCenterPos( image[i].CenterX(), image[i].CenterY() );
	}
}

