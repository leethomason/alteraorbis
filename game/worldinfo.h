#ifndef LUMOS_WORLDINFO_INCLUDED
#define LUMOS_WORLDINFO_INCLUDED

#include "../script/worldgen.h"

class WorldInfo
{
public:
	grinliz::CDynArray< WorldFeature > featureArr;	// continents & oceans

	void Save( tinyxml2::XMLPrinter* );
	void Load( const tinyxml2::XMLElement& doc );

private:
	void Clear() { featureArr.Clear(); }
};

#endif // LUMOS_WORLDINFO_INCLUDED