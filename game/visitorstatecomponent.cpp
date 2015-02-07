#include "visitorstatecomponent.h"
#include "../grinliz/glvector.h"
#include "lumosgame.h"
#include "worldmap.h"
#include "aicomponent.h"
#include "../script/procedural.h"
#include "../xegame/chit.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/chitbag.h"

using namespace grinliz;
using namespace gamui;

/*
static const float SIZE = 0.2f;
static const Vector2F OFFSET = { -0.5f, -0.5f };

VisitorStateComponent::VisitorStateComponent()
{
}


VisitorStateComponent::~VisitorStateComponent()
{

}


void VisitorStateComponent::Serialize( XStream* xs )
{
	this->BeginSerialize( xs, Name() );
	this->EndSerialize( xs );
}


void VisitorStateComponent::OnAdd( Chit* chit, bool init )
{
	super::OnAdd( chit, init );

	const ChitContext* context = Context();
	needsInit = true;

	RenderAtom nullAtom;
	want.Init( &context->worldMap->overlay0, nullAtom, true );

	RenderAtom gray = LumosGame::CalcPaletteAtom( PAL_GRAY*2, 0 );
	RenderAtom green = LumosGame::CalcPaletteAtom( PAL_GREEN*2, 0 );
	bar.Init( &context->worldMap->overlay0, green, gray );
	bar.SetVisible( false );

	context->worldMap->overlay0.Add( &want );
	context->worldMap->overlay0.Add( &bar );
}


void VisitorStateComponent::OnRemove()
{
	const ChitContext* context = Context();
	context->worldMap->overlay0.Remove( &want );
	context->worldMap->overlay0.Remove( &bar );
	super::OnRemove();
}


int VisitorStateComponent::DoTick( U32 delta )
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
				RenderAtom atom = LumosGame::CalcIconAtom( ICON[vd->want] );
				want.SetAtom( atom );
				needsInit = false;
			}

			//want[i].SetVisible( i >= vd->nwant );
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

		Vector2F pos2 = parentChit->GetSpatialComponent()->GetPosition2D() + OFFSET;
		//for (int i = 0; i < VisitorData::NUM_VISITS; ++i) {
			want.SetPos(pos2.x + 0.50f, pos2.y);
			want.SetSize(SIZE, SIZE);
		//}
		bar.SetPos(pos2.x, pos2.y + 1.0f - SIZE);
		bar.SetSize(1.0f, SIZE);
	}
	return VERY_LONG_TICK;
}


void VisitorStateComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}

*/