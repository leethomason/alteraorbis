#include "worldinfo.h"

void WorldInfo::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "WorldInfo" );

	printer->OpenElement( "WorldFeatures" );
	for( int i=0; i<featureArr.Size(); ++i ) {
		featureArr[i].Save( printer );
	}
	printer->CloseElement();

	printer->CloseElement();
}
