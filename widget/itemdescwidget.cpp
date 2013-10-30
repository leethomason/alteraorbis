#include "itemdescwidget.h"

void ItemDescWidget::Init( gamui::Gamui* gamui )
{
	for( int i=0; i<NUM_TEXT_KV; ++i ) {
		textKey[i].Init( gamui );
		textVal[i].Init( gamui );
	}
}

