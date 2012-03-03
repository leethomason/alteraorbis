#ifndef XENOENGINE_ATLAS
#define XENOENGINE_ATLAS

#include "SDL.h"
#include <vector>
#include "../grinliz/glrectangle.h"

class Atlas
{
public:
	Atlas()		{}
	~Atlas()	{}

	void Add( const char* assetName, SDL_Surface* surface );
	SDL_Surface* Generate();

	bool Map( const char* assetName, const grinliz::Rectangle2F& in, grinliz::Rectangle2F* out );

private:
	std::vector< SDL_Surface* > surfaces;
};


#endif // XENOENGINE_ATLAS