#ifndef XENOENGINE_GAME_LIMITS_INCLUDED
#define XENOENGINE_GAME_LIMITS_INCLUDED

// Cross-everything constant. How big can the base
// of a Chit be? Impacts pathing, collision, map,
// etc. Can't be >= 0.5f since the chit could get
// stuck in hallways.
static const float MAX_BASE_RADIUS = 0.4f;

// What is the maximum map size? Used to allocate
// the spatial cache.
static const int MAX_MAX_SIZE = 1024;

enum {
	COMPONENT_MSG_NONE,
	
	// Chits

	// Componets
	SPATIAL_MSG_CHANGED,		// the position or rotation has changed

	GAME_COMPONENT_MSG_START = 0x100
};

#endif // XENOENGINE_GAME_LIMITS_INCLUDED
