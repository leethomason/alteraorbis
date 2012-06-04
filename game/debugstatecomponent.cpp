#include "debugstatecomponent.h"
#include "worldmap.h"
#include "lumosgame.h"
#include "healthcomponent.h"
#include "gamelimits.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/chit.h"
#include "../xegame/xegamelimits.h"


using namespace gamui;
using namespace grinliz;

static const float SIZE_X = 0.8f;
static const float SIZE_Y = 0.2f;
static const Vector2F OFFSET = { -0.5f, -0.5f };

DebugStateComponent::DebugStateComponent( WorldMap* _map ) : map( _map )
{
	RenderAtom a1 = LumosGame::CalcPaletteAtom( 1, 3 );
	RenderAtom a2 = LumosGame::CalcPaletteAtom( 1, 1 );
	healthBar.Init( &map->overlay, 10, a1, a2 ); 
}

void DebugStateComponent::OnAdd( Chit* chit )
{
	Component::OnAdd( chit );
	map->overlay.Add( &healthBar );

	chit->AddListener( this );
	//if ( chit->GetSpatialComponent() ) {
	//	Vector2F pos = chit->GetSpatialComponent()->GetPosition2D() + OFFSET;
	//	healthBar.SetPos( pos.x, pos.y );
	//}
	healthBar.SetSize( SIZE_X, SIZE_Y );

	//HealthComponent* pHealth = GET_COMPONENT( chit, HealthComponent );
	//if ( pHealth ) {
	//	health = pHealth->Health();
	//}
	healthBar.SetRange( 0.8f );
}


void DebugStateComponent::OnRemove()
{
	map->overlay.Remove( &healthBar );
	Component::OnRemove();
}


void DebugStateComponent::OnChitMsg( Chit* chit, int id )
{
	if ( id == SPATIAL_MSG_CHANGED ) {
		Vector2F pos = chit->GetSpatialComponent()->GetPosition2D() + OFFSET;
		healthBar.SetPos( pos.x, pos.y );
	}
	else if ( id == HEALTH_MSG_CHANGED ) {
		HealthComponent* pHealth = GET_COMPONENT( chit, HealthComponent );
		healthBar.SetRange( pHealth->GetHealthFraction() );
	}
}

