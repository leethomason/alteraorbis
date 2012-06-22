#ifndef ANIMATION_BUILDER_INCLUDED
#define ANIMATION_BUILDER_INCLUDED

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../tinyxml2/tinyxml2.h"
#include "../shared/gamedbwriter.h"

void ProcessAnimation( const tinyxml2::XMLElement* element,
					   gamedb::WItem* witem );

#endif // ANIMATION_BUILDER_INCLUDED