#include "gamemovecomponent.h"
#include "worldmap.h"
#include "../Shiny/include/Shiny.h"
#include "../xegame/rendercomponent.h"
#include "../xegame/chit.h"
#include "../xegame/chitbag.h"

using namespace grinliz;

void GameMoveComponent::OnChitMsg( Chit* chit, const ChitMsg& msg )
{
	super::OnChitMsg( chit, msg );
}


void GameMoveComponent::ApplyBlocks( Vector2F* pos, bool* forceApplied )
{
	//GRINLIZ_PERFTRACK;
	PROFILE_FUNC();

	RenderComponent* render = parentChit->GetRenderComponent();
	GLASSERT( render );
	if ( !render ) return;

	Vector2F newPos = *pos;
	float rotation = 0;
	float radius = render->RadiusOfBase();

	const ChitContext* context = Context();

	WorldMap::BlockResult result = context->worldMap->ApplyBlockEffect( *pos, radius, &newPos );
	if ( forceApplied ) *forceApplied = ( result == WorldMap::FORCE_APPLIED );
	bool isStuck = ( result == WorldMap::STUCK );

	if ( forceApplied || isStuck ) {
		int x = (int)newPos.x;
		int y = (int)newPos.y;

		if ( !context->worldMap->IsPassable( x, y )) {
  			Vector2I out = context->worldMap->FindPassable( x, y );
			newPos.x = (float)out.x + 0.5f;
			newPos.y = (float)out.y + 0.5f;
		}
	}
	
	*pos = newPos;
}

