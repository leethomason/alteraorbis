#include "buildscript.h"
#include "itemscript.h"
#include "../game/circuitsim.h"

#include "../grinliz/glstringutil.h"

using namespace grinliz;

/* static */
BuildData BuildScript::buildData[NUM_TOTAL_OPTIONS] = {
	{ "", "", 0, 0, 0 },
	// Utility
	{ "Cancel", "",		0, "Cancels build orders in place." },
	{ "Clear", "",		0, "Clear rock, plants, and buildings." },
	{ "Rotate", "",		0, "Rotate a building and position its porch." },
	{ "Pave", "pave",	0, "Create path paving that blocks plant growth." },
	{ "Silica", "ice",	0, "Build synthetic rock." },
	// Denizen
	{ "SleepTube", "bed",			1, "Increases population." },
	{ "Temple", "temple",			1, "Increases Tech cap, increases population cap, attracts monsters.", "Requires a SleepTube." },
	{ "GuardPost", "guardpost",		1, "Gathering point for guards. Summons denizens if monster approach.", "Requires a Temple." },
	// Agronomy
	{ "Solar Farm", "farm",			2, "Harvests fruit from plants." },
	{ "Distillery", "distillery",	2, "Converts fruit to elixir.", "Requires a Solar Farm." },
	{ "Bar", "bar",					2, "Distributes elixir.", "Requires a Distillery." },
	// Economy
	{ "Market", "market",			3, "Allows Denizens and Avatar to buy and sell weapons and shields." },
	{ "Forge", "factory",			3, "Allows Denizens and Avatar to make weapons and shields.", "Requires a Market" },
	{ "Exchange", "exchange",		3, "Allows Denizens and Avatar convert between Au and Crystal.", "Requires a Market" },
	{ "Vault", "vault",				3, "Stores items for the Core and Avatar.", "Requires a Market" },
	// Visitor
	{ "News", "kiosk.n",			4, "Attracts Visitors searching for news.", "Requires a Temple" },
	{ "Media", "kiosk.m",			4, "Attracts Visitors searching for media.", "Requires a Temple" },
	{ "Commerce", "kiosk.c",		4, "Attracts Visitors searching for commerce.", "Requires a Temple" },
	{ "Social", "kiosk.s",			4, "Attracts Visitors searching for social.", "Requires a Temple" },
	// Circuits
	{ "Circuit Fab", "circuitFab",	5, "Builds circuits, traps, and mechanisms.", "Requires a Forge" },
	{ "Battery", "power",			5, "Creates a charge when triggered.", "Requires a Circuit Fab" },
	{ "Turret", "turret",			5, "Fires weapon when charge applied.", "Requires a Circuit Fab" },
	{ "Switch", "",					5, "Creates a spark when triggered.", "Requires a Circuit Fab", CIRCUIT_SWITCH },
	{ "Bend", "",					5, "Routes electrons.", "Requires a Circuit Fab", CIRCUIT_BEND },
	{ "Split", "",					5, "Splits electrons.", "Requires a Circuit Fab", CIRCUIT_FORK_2 },
	{ "Silica\nGate", "",			5, "Uses a charge to create or destroy Silica.", "Requires a Circuit Fab", CIRCUIT_ICE },
	{ "Stop", "",					5, "Absorbs a charge or spark.", "Requires a Circuit Fab", CIRCUIT_STOP },
	{ "Detector", "",				5, "Sparks for enemies of any mass.", "Requires a Circuit Fab", CIRCUIT_DETECT_ENEMY },
	{ "Transistor", "",				5, "Digital toggle switch.", "Requires a Circuit Fab", CIRCUIT_TRANSISTOR_A },

	// Additional:
	{ "TrollStatue", "trollStatue", 6 },
};

	//{ "Zapper", "",					5, "Converts charge to a zapping attack.", "Requires a Circuit Fab", CIRCUIT_POWER_UP },


const BuildData* BuildScript::GetDataFromStructure( const grinliz::IString& structure, int *id )
{
	for( int i=0; i<NUM_TOTAL_OPTIONS; ++i ) {
		if ( structure == buildData[i].cStructure ) {
			if (id) *id = i;
			return &GetData( i );
		}
	}
	return 0;
}


const BuildData& BuildScript::GetData( int i )
{
	GLASSERT( i >= 0 && i < NUM_TOTAL_OPTIONS );
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

			if (gi.keyValues.GetIString(ISC::zoneCreate) == ISC::industrial)
				buildData[i].zoneCreate = BuildData::ZONE_INDUSTRIAL;
			else if (gi.keyValues.GetIString(ISC::zoneCreate) == ISC::natural)
				buildData[i].zoneCreate = BuildData::ZONE_NATURAL;

			if (gi.keyValues.GetIString(ISC::zoneConsume) == ISC::industrial)
				buildData[i].zoneConsume = BuildData::ZONE_INDUSTRIAL;
			else if (gi.keyValues.GetIString(ISC::zoneConsume) == ISC::natural)
				buildData[i].zoneConsume = BuildData::ZONE_NATURAL;

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

			int porch = 0;
			gi.keyValues.Get(ISC::porch, &porch);
			buildData[i].porch = porch != 0;
		}
	}
	return buildData[i];
}


