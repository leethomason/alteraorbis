#include "titlescene.h"
#include "../game/lumosgame.h"

TitleScene::TitleScene( LumosGame* game ) : Scene( game )
{
	label.Init( &gamui2D );
	label.SetText( "Hello Lumos" );
}


void TitleScene::Resize()
{
	label.SetPos( 10, 10 );
}



