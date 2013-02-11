#include "worldinfo.h"

#include "../grinliz/glrandom.h"

using namespace tinyxml2;
using namespace grinliz;

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


void WorldInfo::Save( gamedb::WItem* parent )
{
	gamedb::WItem* info = parent->FetchChild( "WorldInfo" );
	gamedb::WItem* features = info->FetchChild( "WorldFeatures" );
	for( int i=0; i<featureArr.Size(); ++i ) {
		featureArr[i].Serialize( DBItem(features), i );
		//featureArr[i].Save( features );
	}
}


void WorldInfo::Load( const tinyxml2::XMLElement& parent )
{
	Clear();
	const XMLElement* worldInfo = parent.FirstChildElement( "WorldInfo" );
	GLASSERT( worldInfo );
	if ( worldInfo ) {
		const XMLElement* worldFeatures = worldInfo->FirstChildElement( "WorldFeatures" );
		GLASSERT( worldFeatures );
		if ( worldFeatures ) {
			for( const XMLElement* feature=worldFeatures->FirstChildElement( "WorldFeature" );
				 feature;
				 feature = feature->NextSiblingElement( "WorldFeature" ) ) 
			{
				WorldFeature* wf = featureArr.PushArr(1);
				wf->Load( *feature );
			}
		}
	}
}

