#include "consolewidget.h"

using namespace grinliz;


ConsoleWidget::ConsoleWidget()
{
	GLString empty;
	for( int i=0; i<NUM_LINES; ++i ) {
		lines.Push( empty );
		age.Push( 0 );
	}
}


void ConsoleWidget::Init( gamui::Gamui* g )
{
	text.Init( g );
	text.SetText( "Console Line 1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n" );
}


void ConsoleWidget::SetBounds( float w, float h )
{
	text.SetBounds( w, h );
}


void ConsoleWidget::Push( const grinliz::GLString &str )
{
	for( int i=1; i<lines.Size(); ++i ) {
		lines[i-1] = lines[i];
		age[i-1]   = age[i];
	}
	lines[lines.Size()-1] = str;
	age[age.Size()-1] = 0;

	strBuffer = "";
	for( int i=0; i<lines.Size(); ++i ) {
		strBuffer.append( lines[i] );
		strBuffer.append( "\n" );
	}
}


void ConsoleWidget::DoTick( U32 delta )
{
	int count = 0;
	for( int i=0; i<lines.Size(); ++i ) {
		age[i] += delta;
		if ( age[i] > AGE_TIME ) {
			++count;
		}
	}
	GLString empty;
	for( int i=0; i<count; ++i ) {
		Push( empty );
	}
}
