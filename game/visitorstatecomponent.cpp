#include "visitorstatecomponent.h"
#include "../grinliz/glvector.h"
#include "lumosgame.h"
#include "worldmap.h"
#include "aicomponent.h"
#include "../script/procedural.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"

using namespace grinliz;
using namespace gamui;

static const float SIZE = 0.2f;
static const Vector2F OFFSET = { -0.5f, -0.5f };

VisitorStateComponent::VisitorStateComponent( WorldMap* _map ) : worldMap( _map )
{
	for( int i=0; i<VisitorData::NUM_VISITS; ++i ) {
		RenderAtom nullAtom;
		wants[i].Init( &worldMap->overlay0, nullAtom, true );
	}


	RenderAtom gray = LumosGame::CalcPaletteAtom( PAL_GRAY*2, 0 );
	RenderAtom green = LumosGame::CalcPaletteAtom( PAL_GREEN*2, 0 );
	bar.Init( &worldMap->overlay0, 10, green, gray );
}


VisitorStateComponent::~VisitorStateComponent()
{

}


void VisitorStateComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	this->EndSerialize( xs );
}


void VisitorStateComponent::OnAdd( Chit* chit )
{
	super::OnAdd( chit );
	needsInit = true;

	for( int i=0; i<VisitorData::NUM_VISITS; ++i ) {
		worldMap->overlay0.Add( &wants[i] );
	}
	worldMap->overlay0.Add( &bar );
}


void VisitorStateComponent::OnRemove()
{
	for( int i=0; i<VisitorData::NUM_VISITS; ++i ) {
		worldMap->overlay0.Remove( &wants[i] );
	}
	worldMap->overlay0.Remove( &bar );
	super::OnRemove();
}


int VisitorStateComponent::DoTick( U32 delta, U32 since )
{
	AIComponent* ai = parentChit->GetAIComponent();
	if ( ai ) {
		int index = ai->VisitorIndex();
		if ( index >= 0 ) {
			const VisitorData* vd = Visitors::Get( index );

			if ( needsInit ) {
				static const char* ICON[VisitorData::NUM_KIOSK_TYPES] = {
					"news",
					"media",
					"commerce",
					"social"
				};
				for( int i=0; i<VisitorData::NUM_VISITS; ++i ) {
					int need = vd->wants[i];
					RenderAtom atom = LumosGame::CalcIconAtom( ICON[need] );
					wants[i].SetAtom( atom );
				}
				needsInit = false;
			}

			for( int i=0; i<VisitorData::NUM_VISITS; ++i ) {
				wants[i].SetVisible( i >= vd->nWants );
			}
			if ( vd->kioskTime ) {
				float t = (float)vd->kioskTime / (float)VisitorData::KIOSK_TIME;
				bar.SetVisible( true );
				bar.SetRange( t );
			}
			else {
				bar.SetVisible( false );
			}
		}
	}
	return VERY_LONG_TICK;
}


void VisitorStateComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	Vector2F pos2 = chit->GetSpatialComponent()->GetPosition2D() + OFFSET;

	if ( msg.ID() == ChitMsg::SPATIAL_CHANGED ) {
		for( int i=0; i<VisitorData::NUM_VISITS; ++i ) {
			wants[i].SetPos( pos2.x + (float)i*0.25f, pos2.y );
			wants[i].SetSize( SIZE, SIZE );
		}
		bar.SetPos( pos2.x, pos2.y + 1.0f - SIZE );
		bar.SetSize( 1.0f, SIZE );
	}
	else {
		super::OnChitMsg( chit, msg );
	}
}



