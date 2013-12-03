#include "buildscript.h"
#include "itemscript.h"

#include "../grinliz/glstringutil.h"

using namespace grinliz;

/* static */
BuildData BuildScript::buildData[NUM_OPTIONS] = {
	// Utility
	{	"None",		"",			0, 0, 0 },
	{	"Clear",	"",			0, 0, 0 },
	{	"Rotate",	"",			0, 0, 0 },
	{	"Pave",		"pave",		0, 0, 0 },
	{	"Ice",		"ice",		0, 0, 0 },
	// Tech 0
	{	"News",		"kiosk.n",	1, 0, 0 },
	{	"Media",	"kiosk.m",	1, 0, 0 },
	{	"Commerce",	"kiosk.c",	1, 0, 0 },
	{	"Social",	"kiosk.s",	1, 0, 0 },
	{	"Factory",	"factory",	1, 0, 0 },
	{	"Power",	"power",	1, 0, 0 },
	// Tech 1
	{	"Vault",	"vault",	2, 0, 0 },
	{	"SleepTube","bed",		2, 0, 0 },
	{	"Market",	"market",	2, 0, 0 },
	{	"Bar",		"bar",		2, 0, 0 },
};


const BuildData* BuildScript::GetDataFromStructure( const grinliz::IString& structure )
{
	for( int i=0; i<NUM_OPTIONS; ++i ) {
		if ( structure == buildData[i].cStructure ) {
			return &GetData( i );
		}
	}
	return 0;
}


const BuildData& BuildScript::GetData( int i )
{
	GLASSERT( i >= 0 && i < NUM_OPTIONS );
	if ( buildData[i].size == 0 ) {

		buildData[i].name		= StringPool::Intern( buildData[i].cName, true );
		buildData[i].structure	= StringPool::Intern( buildData[i].cStructure, true );

		bool has = ItemDefDB::Instance()->Has( buildData[i].cStructure );

		// Generate the label.
		CStr<64> str = buildData[i].cName;
		if ( *buildData[i].cName && has ) 
		{
			const GameItem& gi = ItemDefDB::Instance()->Get( buildData[i].cStructure );
			int cost = 0;
			gi.keyValues.GetInt( "cost", &cost );
			str.Format( "%s\nAu %d", buildData[i].cName, cost );
		}
		buildData[i].label = StringPool::Intern( str.c_str() ) ;
		
		buildData[i].size = 1;
		if ( has ) {
			const GameItem& gi = ItemDefDB::Instance()->Get( buildData[i].cStructure );
			gi.keyValues.GetInt( "size", &buildData[i].size );
			gi.keyValues.GetInt( "cost", &buildData[i].cost );

			buildData[i].needs.SetZero();
			
			for( int i=0; i<ai::Needs::NUM_NEEDS; ++i ) {
				CStr<32> str;
				str.Format( "need.%s", ai::Needs::Name(i) );

				float need=0;
				gi.keyValues.GetFloat( str.c_str(), &need );
				buildData[i].needs.Set( i, need );
			}

		}
	}
	return buildData[i];
}


