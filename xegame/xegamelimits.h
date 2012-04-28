#ifndef XENOENGINE_GAME_LIMITS_INCLUDED
#define XENOENGINE_GAME_LIMITS_INCLUDED

// Cross-everything constant. How big can the base
// of a Chit be? Impacts pathing, collision, map,
// etc. Can't be >= 0.5f since the chit could get
// stuck in hallways.
static const float MAX_BASE_RADIUS = 0.4f;

#endif // XENOENGINE_GAME_LIMITS_INCLUDED
