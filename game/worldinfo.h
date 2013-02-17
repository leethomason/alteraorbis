#ifndef LUMOS_WORLDINFO_INCLUDED
#define LUMOS_WORLDINFO_INCLUDED

#include "../script/worldgen.h"
#include "../xarchive/glstreamer.h"

class WorldInfo
{
public:
	grinliz::CDynArray< WorldFeature > featureArr;	// continents & oceans
	void Serialize( XStream* );
	void Save( tinyxml2::XMLPrinter* printer );
	void Load( const tinyxml2::XMLElement& parent );

private:
	void Clear() { featureArr.Clear(); }
};

#endif // LUMOS_WORLDINFO_INCLUDED