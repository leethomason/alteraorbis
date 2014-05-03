#include "buildscript.h"
#include "itemscript.h"

#include "../grinliz/glstringutil.h"

using namespace grinliz;

/* static */
BuildData BuildScript::buildData[NUM_OPTIONS] = {
	{ "", "", 0, 0, 0 },
	// Utility
	{	"Cancel",	"",			0, "Cancels build orders in place." },
	{	"Clear",	"",			0, "Clear rock, plants, and buildings." },
	{	"Rotate",	"",			0, "Rotate a building and position its porch." },
	{	"Pave",		"pave",		0, "Create path paving that blocks plant growth." },
	{	"Ice",		"ice",		0, "Build synthetic rock." },
	// Visitor stuff
	{	"News",		"kiosk.n",	1, "Attracts Visitors searching for news." },
	{	"Media",	"kiosk.m",	1, "Attracts Visitors searching for media." },
	{	"Commerce",	"kiosk.c",	1, "Attracts Visitors searching for commerce." },
	{	"Social",	"kiosk.s",	1, "Attracts Visitors searching for social." },
	{	"Temple",	"power",	1, "Increases Tech cap, increases population cap, attracts monsters." },
	// Economy
	{	"Vault",	"vault",	2, "Stores items for the Core and Avatar." },
	{	"SleepTube","bed",		2, "Increases population." },
	{	"Market",	"market",	2, "Allows Denizens and Avatar to buy and sell weapons and shields." },
	{	"Bar",		"bar",		2, "Distributes elixir." },
	// Defense
	{	"GuardPost","guardpost",3, "Gathering point for guards. Summons denizens if monster approach." },
	// Industry
	{	"Solar Farm","farm",	4, "Harvests fruit from plants." },
	{	"Distillery","distillery",4, "Converts fruit to elixir." },
	{	"Forge",	"factory",	4, "Allows Denizens and Avatar to make weapons and shields." },
	{	"Exchange", "exchange", 4, "Allows Denizens and Avatar convert between Au and Crystal." },
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
			gi.keyValues.Get( ISC::cost, &cost );
			str.Format( "%s\nAu %d", buildData[i].cName, cost );
		}
		buildData[i].label = StringPool::Intern( str.c_str() ) ;
		
		buildData[i].size = 1;
		if ( has ) {
			const GameItem& gi = ItemDefDB::Instance()->Get( buildData[i].cStructure );
			gi.keyValues.Get( ISC::size, &buildData[i].size );
			gi.keyValues.Get( ISC::cost, &buildData[i].cost );

			buildData[i].needs.SetZero();
			
			for( int k=0; k<ai::Needs::NUM_NEEDS; ++k ) {
				CStr<32> str;
				str.Format( "need.%s", ai::Needs::Name(k) );

				float need=0;
				gi.keyValues.Get( str.c_str(), &need );
				if ( need > 0 ) {
					buildData[i].needs.Set( k, need );
				}
			}

			float timeF = 1.0;
			gi.keyValues.Get( "need.time", &timeF );
			buildData[i].standTime = int( timeF * 1000.0f );
		}
	}
	return buildData[i];
}


