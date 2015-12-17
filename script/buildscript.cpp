#include "buildscript.h"
#include "itemscript.h"
#include "../game/circuitsim.h"
#include "../game/lumosgame.h"

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
	{ "GuardPost", "guardpost",		1, "Gathering point for guards. Notifies denizens of monster approach.", "Requires a Temple." },
	{ "Academy", "academy", 		1, "Trains denizens.", "" },
	// Agronomy
	{ "Solar Farm", "farm",			2, "Harvests fruit from plants." },
	{ "Distillery", "distillery",	2, "Converts fruit to elixir.", "Requires a Solar Farm." },
	{ "Tavern", "bar",				2, "Distributes elixir.", "Requires a Distillery." },
	// Economy
	{ "Market", "market",			3, "Allows Denizens and Avatar to buy and sell weapons and shields." },
	{ "Forge", "factory",			3, "Allows Denizens and Avatar to make weapons and shields.", "Requires a Market" },
	{ "Exchange", "exchange",		3, "Allows Denizens and Avatar convert between Au and Crystal.", "Requires a Market" },
	{ "Vault", "vault",				3, "Stores items for the Core and Avatar.", "Requires a Market" },
	// Visitor
	{ "Kiosk", "kiosk",				4, "Attracts Visitors, generates tech.", "Requires a Temple" },
	// Circuits
	{ "Switch On", "switchOn",		5, "Manually turns on devices." },
	{ "Switch Off", "switchOff",	5, "Manually turns off devices." },
	{ "Detector", "detector",		5, "Sensor to detect enemies" },
	{ "Turret", "turret",			5, "Fires weapon when charge applied." },
	{ "Gate", "gate",				5, "Creates silica when triggered." },
	{ "TimedGate", "timedGate",		5, "Creates temporary silica when triggered." },

	// Additional:
	{ "TrollStatue", "trollStatue", 6 },
	{ "TrollBrazier", "trollBrazier", 6 },
	{ "KamakirianStatue", "kamaStatue", 6 },
	{ "GobmanStatue", "gobmanStatue", 6 },
};


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

			if (gi.keyValues.GetIString(ISC::zone) == ISC::industrial)
				buildData[i].zone = BuildData::ZONE_INDUSTRIAL;
			else if (gi.keyValues.GetIString(ISC::zone) == ISC::natural)
				buildData[i].zone = BuildData::ZONE_NATURAL;

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


Rectangle2I BuildData::Bounds(int size, const Vector2I& pos)
{
	Rectangle2I r;
	r.min = pos;
	r.max.x = r.min.x + size - 1;
	r.max.y = r.min.y + size - 1;
	return r;
}


Rectangle2I BuildData::PorchBounds(int size, const Vector2I& pos, int r)
{
	Rectangle2I b = Bounds(size, pos);
	Rectangle2I v = b;

	switch (r) {
		case 0:		v.min.y = v.max.y = b.max.y + 1;	break;
		case 1:		v.min.x = v.max.x = b.max.x + 1;	break;
		case 2:		v.min.y = v.max.y = b.min.y - 1;	break;
		case 3:		v.min.x = v.max.x = b.min.x - 1;	break;
		default:	GLASSERT(0);	break;
	}

	return v;
}


int BuildData::DrawBounds(const grinliz::Rectangle2I& bounds, const grinliz::Rectangle2I& porchBounds,
						  gamui::Image* image)
{
	image[0].SetPos((float)bounds.min.x, (float)bounds.min.y);
	image[0].SetSize((float)bounds.Width(), (float)bounds.Height());
	image[0].SetVisible(true);
	image[0].SetAtom(LumosGame::CalcIconAtom("build"));

	int i = 1;
	if (!porchBounds.min.IsZero()) {
		for (Rectangle2IIterator it(porchBounds); !it.Done(); it.Next()) {
			image[i].SetPos(float(it.Pos().x), float(it.Pos().y));
			image[i].SetSize(1, 1);
			image[i].SetVisible(true);
			image[i].SetAtom(LumosGame::CalcIconAtom("porch"));
			i++;
		}
	}
	return i;
}


void BuildData::LabelWithCount(int count, grinliz::CStr<64>* str) const
{
	if (count) {
		str->Format("%s (%d)", label.safe_str(), count);
	}
	else {
		*str = label.safe_str();
	}
}

