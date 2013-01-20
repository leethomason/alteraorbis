#include "gamemovecomponent.h"
#include "worldmap.h"
#include "../grinliz/glperformance.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chit.h"

using namespace grinliz;

void GameMoveComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}


void GameMoveComponent::ApplyBlocks( Vector2F* pos, bool* forceApplied, bool* isStuck )
{
	GRINLIZ_PERFTRACK;
	RenderComponent* render = parentChit->GetRenderComponent();
	GLASSERT( render );
	if ( !render ) return;

	Vector2F newPos = *pos;
	float rotation = 0;
	float radius = render->RadiusOfBase();

	WorldMap::BlockResult result = map->ApplyBlockEffect( *pos, radius, &newPos );
	if ( forceApplied ) *forceApplied = ( result == WorldMap::FORCE_APPLIED );
	if ( isStuck )		*isStuck = ( result == WorldMap::STUCK );

	if ( forceApplied || isStuck ) {
		int x = (int)newPos.x;
		int y = (int)newPos.y;

		if ( !map->IsPassable( x, y )) {
			Vector2I out = map->FindPassable( x, y );
			newPos.x = (float)out.x + 0.5f;
			newPos.y = (float)out.y + 0.5f;
		}
	}
	
	*pos = newPos;
}

