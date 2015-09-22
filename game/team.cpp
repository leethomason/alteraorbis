#include "team.h"
#include "visitorweb.h"
#include "lumoschitbag.h"
#include "../script/corescript.h"
#include "../grinliz/glutil.h"
#include "../xegame/istringconst.h"
#include "../xegame/chit.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/chitcontext.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

Team* Team::instance = 0;

Team::Team(const gamedb::Reader* db)
{
	GLASSERT(!instance);
	instance = this;
	database = db;
	idPool = 1;	// id=0 is rogue.
}

Team::~Team()
{
	GLASSERT(instance == this);
	instance = 0;
}

void Team::DoTick(int delta)
{
	for (int i = 0; i < treaties.Size(); ++i) {
		treaties[i].peaceTimer -= delta;
		treaties[i].warTimer -= delta;
		if (treaties[i].peaceTimer < 0) treaties[i].peaceTimer = 0;
		if (treaties[i].warTimer < 0) treaties[i].warTimer = 0;

		if (treaties[i].warTimer == 0 && treaties[i].peaceTimer == 0) {
			treaties.SwapRemove(i);
			--i;
		}
	}
}

void Team::SymmetricTK::Serialize(XStream* xs)
{
	XarcOpen(xs, "SymmetricTK");
	XARC_SER(xs, t0);
	XARC_SER(xs, t1);
	XARC_SER(xs, warTimer);
	XARC_SER(xs, peaceTimer);
	XarcClose(xs);
}

void Team::Control::Serialize(XStream* xs)
{
	XarcOpen(xs, "Control");
	XARC_SER(xs, super);
	XARC_SER(xs, sub);
	XarcClose(xs);
}


void Team::Serialize(XStream* xs)
{
	XarcOpen(xs,"Team");
	XARC_SER(xs, idPool);
	XARC_SER_CARRAY(xs, treaties);
	XARC_SER_CARRAY(xs, control);

	XarcOpen(xs, "attitude");
	if (xs->Saving()) {
		int size = hashTable.Size();
		XARC_SER_KEY(xs, "size", size);
		for (int i = 0; i < size; ++i) {
			XarcOpen(xs, "teamkey");
			const TeamKey& tk = hashTable.GetKey(i);
			int t0 = tk.T0();
			int t1 = tk.T1();
			int a = hashTable.GetValue(i);
			XARC_SER_KEY(xs, "t0", t0);
			XARC_SER_KEY(xs, "t1", t1);
			XARC_SER_KEY(xs, "a", a);
			XarcClose(xs);
		}
	}
	else {
		hashTable.Clear();
		int size = 0;
		XARC_SER_KEY(xs, "size", size);
		for (int i = 0; i < size; ++i) {
			XarcOpen(xs, "teamkey");
			int t0=0, t1=0, a=0;
			XARC_SER_KEY(xs, "t0", t0);
			XARC_SER_KEY(xs, "t1", t1);
			XARC_SER_KEY(xs, "a", a);
			TeamKey tk(t0, t1);
			hashTable.Add(tk, a);
			XarcClose(xs);
		}
	}
	XarcClose(xs);	// attitude
	XarcClose(xs);	// team
}

grinliz::IString Team::TeamName(int team)
{
	IString name;
	CStr<64> str;
	int group = 0, id = 0;
	SplitID(team, &group, &id);

	switch (group) {
		case TEAM_NEUTRAL:			name = StringPool::Intern("neutral");		break;
		case TEAM_CHAOS:			name = StringPool::Intern("chaos");			break;
		case TEAM_RAT:				name = ISC::trilobyte;						break;
		case TEAM_GREEN_MANTIS:		name = ISC::mantis;							break;
		case TEAM_RED_MANTIS:		name = ISC::redMantis;						break;

		case TEAM_TROLL:
		// Since Trolls can't build anything,
		// any troll core is by definition
		// Truulga. (At least at this point.)
		name = ISC::Truulga;
		break;

		case TEAM_HOUSE:
		case TEAM_GOB:
		case TEAM_KAMAKIRI:
		if (id) {
			int superTeam = this->SuperTeam(team);
			if (superTeam == team) {
				name = LumosChitBag::StaticNameGen(database, "domainNames", id);
			}
			else { 
				IString super = LumosChitBag::StaticNameGen(database, "domainNames", Team::ID(superTeam));
				IString sub   = LumosChitBag::StaticNameGen(database, "domainNames", id);
				str.Format("%s.%s", sub.safe_str(), super.safe_str());
				name = StringPool::Intern(str.c_str());
			}
		}
		else {
			if (group == TEAM_HOUSE) str = "House";
			else if (group == TEAM_GOB) str = "Gobmen";
			else if (group == TEAM_KAMAKIRI) str = "Kamakiri";
			name = StringPool::Intern(str.c_str());
		}
		break;

		case TEAM_VISITOR:
		name = StringPool::Intern("Visitor");
		break;

		case DEITY_MOTHER_CORE:
		name = ISC::MotherCore;
		break;

		case DEITY_Q:
		name = ISC::QCore;
		break;

		case DEITY_R1K:
		name = ISC::R1k;
		break;

		case DEITY_KALA:
		name = ISC::Kala;
		break;

		case DEITY_TRUULGA:
		name = ISC::Truulga;
		break;

		default:
		GLOUTPUT(("Invalid team: %d\n", team));
		GLASSERT(0);
		break;
	}

	return name;
}


int Team::GetTeam( const grinliz::IString& itemName )
{
	if (itemName == ISC::trilobyte) {
		return TEAM_RAT;
	}
	else if ( itemName == ISC::mantis ) {
		return TEAM_GREEN_MANTIS;
	}
	else if ( itemName == ISC::redMantis ) {
		return TEAM_RED_MANTIS;
	}
	else if ( itemName == ISC::troll ) {
		return TEAM_TROLL;
	}
	else if (itemName == ISC::gobman) {
		return TEAM_GOB;
	}
	else if (itemName == ISC::kamakiri) {
		return TEAM_KAMAKIRI;
	}
	else if (itemName == ISC::MotherCore) {
		return DEITY_MOTHER_CORE;
	}
	else if (itemName == ISC::QCore) {
		return DEITY_Q;
	}
	else if (itemName == ISC::R1k) {
		return DEITY_R1K;
	}
	else if (itemName == ISC::Kala) {
		return DEITY_KALA;
	}
	else if (itemName == ISC::Truulga) {
		return DEITY_TRUULGA;
	}
	else if (    itemName == ISC::cyclops
		      || itemName == ISC::fireCyclops
		      || itemName == ISC::shockCyclops )
	{
		return TEAM_CHAOS;
	}
	else if (    itemName == ISC::human) 
	{
		return TEAM_HOUSE;
	}
	GLASSERT(0);
	return TEAM_NEUTRAL;
}


ERelate Team::BaseRelationship( int _t0, int _t1 )
{
	int t0 = 0, t1 = 0;
	int g0 = 0, g1  =0 ;
	SplitID(_t0, &t0, &g0);
	SplitID(_t1, &t1, &g1);

	// t0 <= t1 to keep the logic simple.
	if ( t0 > t1 ) Swap( &t0, &t1 );

	// Neutral is just neutral. Else Chaos units
	// keep attacking neutral cores. Very annoying.
	if (t0 == TEAM_NEUTRAL || t1 == TEAM_NEUTRAL)
		return ERelate::NEUTRAL;

	// CHAOS hates all - even each other.
	if ( t0 == TEAM_CHAOS || t1 == TEAM_CHAOS)
		return ERelate::ENEMY;

	// Deity exception:
	if (t1 == DEITY_TRUULGA)
		return BaseRelationship(t0, TEAM_TROLL);

	// Other deities neutral:
	if (t1 >= DEITY_MOTHER_CORE)
		return ERelate::NEUTRAL;

	GLASSERT(t0 >= TEAM_RAT && t0 < NUM_TEAMS);
	GLASSERT(t1 >= TEAM_RAT && t1 < NUM_TEAMS);

	static const int F = int(ERelate::FRIEND);
	static const int E = int(ERelate::ENEMY);
	static const int N = int(ERelate::NEUTRAL);
	static const int OFFSET = TEAM_RAT;
	static const int NUM = NUM_MOB_TEAMS - OFFSET;

	static const int relate[NUM][NUM] = {
		{ F, E, E, E, E, E, E, E },		// rat
		{ 0, F, E, N, E, E, F, N },		// green
		{ 0, 0, F, N, E, E, E, E },		// red
		{ 0, 0, 0, F, E, N, N, N },		// troll 
		{ 0, 0, 0, 0, F, N, E, F },		// house
		{ 0, 0, 0, 0, 0, F, E, F },		// gobmen
		{ 0, 0, 0, 0, 0, 0, F, N },		// kamakiri
		{ 0, 0, 0, 0, 0, 0, 0, F }		// visitor
	};
	GLASSERT(t0 - OFFSET >= 0 && t0 - OFFSET < NUM);
	GLASSERT(t1 - OFFSET >= 0 && t1 - OFFSET < NUM);
	GLASSERT(t1 >= t0);

	// Special handling for left/right battle scene matchups:
	if (   t0 == t1 
		&& ((g0 == TEAM_ID_LEFT && g1 == TEAM_ID_RIGHT) || (g0 == TEAM_ID_RIGHT && g1 == TEAM_ID_LEFT))) 
	{
		return ERelate::ENEMY;
	}
	return ERelate(relate[t0-OFFSET][t1-OFFSET]);
}


ERelate Team::GetRelationship(Chit* chit0, Chit* chit1)
{
	const GameItem* item0 = chit0->GetItem();
	const GameItem* item1 = chit1->GetItem();

	if (item0 && item1) {
		const int team0 = item0->Team();
		const int team1 = item1->Team();

		ERelate r = GetRelationship(team0, team1);
		// Check symmetry:
		GLASSERT(r == GetRelationship(team1, team0));
		return r;
	}
	return ERelate::NEUTRAL;
}


ERelate Team::GetRelationship(int t0, int t1)
{
	if (t0 == t1) {
		return t0 == TEAM_CHAOS ? ERelate::ENEMY : ERelate::FRIEND;
	}

	TeamKey tk0(t0, t1);
	TeamKey tk1(t1, t0);
	int d0 = 0, d1 = 0;

	if (!hashTable.Query(tk0, &d0)) {
		d0 = RelationshipToAttitude(BaseRelationship(t0, t1));
	}
	if (!hashTable.Query(tk1, &d1)) {
		d1 = RelationshipToAttitude(BaseRelationship(t0, t1));
	}
	// Combined relationship is the worse one.
	int d = Min(d0, d1);
	ERelate r = AttitudeToRelationship(d);
	return r;
}


int Team::Attitude(CoreScript* center, CoreScript* eval)
{
	int t0 = center->ParentChit()->Team();
	int t1 = eval->ParentChit()->Team();

	TeamKey tk(t0, t1);
	int d0 = 0;
	if (hashTable.Query(tk, &d0)) {
		return d0;
	}
	ERelate relate = BaseRelationship(t0, t1);
	int d = RelationshipToAttitude(relate);
	return d;
}

int Team::CalcAttitude(CoreScript* center, CoreScript* eval, const Web* web)
{
	GLASSERT(eval != center);

	// Positive: more friendly
	// Negative: more enemy

	// Species 
	int centerTeam = center->ParentChit()->Team();
	int evalTeam = eval->ParentChit()->Team();
	if (Team::IsDeity(centerTeam)) return 0;

	const bool ENVIES_WEALTH = (centerTeam == TEAM_GOB);
	const bool ENVIES_TECH = (centerTeam == TEAM_KAMAKIRI);
	const bool WARLIKE = (centerTeam == TEAM_KAMAKIRI);

	ERelate relate = BaseRelationship(centerTeam, evalTeam);
	int d = 0;

	switch (relate) {
		case ERelate::FRIEND:	d = d + 2;	break;
		case ERelate::ENEMY:	d = d - 1;	break;
		case ERelate::NEUTRAL:				break;
	}

	// Compete for Visitors
	Vector2I sector = ToSector(center->ParentChit()->Position());
	// There should be a webNode where we are, of course,
	// but that depends on bringing the web cache current.
	const MinSpanTree::Node* webNode = web->FindNode(sector);
	if (!webNode) {
		// Web needs to be updated. Return NEUTRAL for now.
		return 0;
	}
	float visitorStr = webNode->strength;

	// An an alternate world where 'eval' is gone...what happens?
	Web altWeb;
	Vector2I altSector = ToSector(eval->ParentChit()->Position());
	altWeb.Calc(&altSector);
	const MinSpanTree::Node* altWebNode = altWeb.FindNode(sector);
	float altVisitorStr = altWebNode->strength;

	if (altVisitorStr > visitorStr) {
		d--;		// better off if they are gone...
	}
	else if (altVisitorStr < visitorStr) {
		d += WARLIKE ? 1 : 2;		// better off with them around...
	}

	// Techiness, envy of Kamakiri
	if (ENVIES_TECH && eval->GetTech() > center->GetTech()) {
		d--;
	}

	// Wealth, envy of the Gobmen
	if (ENVIES_WEALTH && ((eval->CoreWealth() * 2 / 3) > center->CoreWealth())) {
		d--;
	}

	// Treaties:
	SymmetricTK stk(centerTeam, evalTeam);
	int idx = treaties.Find(stk);
	if (idx >= 0) {
		if (treaties[idx].warTimer) {
			d -= treaties[idx].warTimer * 10 / TREATY_TIME;
		}
		else if (treaties[idx].peaceTimer) {
			d += treaties[idx].peaceTimer * 10 / TREATY_TIME;
		}
	}

	// Too much of one group.
	const Census& census = center->ParentChit()->Context()->chitBag->census;	// FIXME clearly a path out of scope
	int numCores = census.NumCoresOfTeam(Team::Group(evalTeam));
	if (numCores >= TYPICAL_AI_DOMAINS / 2)
		d--;
	if (numCores >= TYPICAL_AI_DOMAINS * 3 / 4)
		d--;

	// Control!
	if ((evalTeam == SuperTeam(centerTeam)) || (centerTeam == SuperTeam(evalTeam))) {
		d = Max(d, 0);	// at least neutral.
	}
	else if (SuperTeam(evalTeam) == SuperTeam(centerTeam)) {
		d += 2;	// tend to be friendlier.
	}

	TeamKey tk(centerTeam, evalTeam);
	hashTable.Add(tk, d);
	return d;
}


bool Team::War(CoreScript* c0, CoreScript* c1, bool commit, const Web* web)
{
	if (c0 && c1 && (c0 != c1) 
		&& c0->InUse() && c1->InUse() 
		&& !Team::IsDeity(c0->ParentChit()->Team()) 
		&& !Team::IsDeity(c1->ParentChit()->Team())) 
	{
		ERelate relate = GetRelationship(c0->ParentChit(), c1->ParentChit());
		if (relate != ERelate::ENEMY) {
			SymmetricTK stk(c0->ParentChit()->Team(), c1->ParentChit()->Team());
			int idx = treaties.Find(stk);
			if (idx < 0) {
				// no treaty in place, of either kind.
				if (commit) {
					stk.warTimer = TREATY_TIME;
					treaties.Push(stk);
				}
				CalcAttitude(c0, c1, web);
				CalcAttitude(c1, c0, web);
				return true;
			}
		}
	}
	return false;
}


int Team::Peace(CoreScript* c0, CoreScript* c1, bool commit, const Web* web)
{
	if (c0 && c1 && (c0 != c1) 
		&& c0->InUse() && c1->InUse() 
		&& !Team::IsDeity(c0->ParentChit()->Team()) 
		&& !Team::IsDeity(c1->ParentChit()->Team())) 
	{
		ERelate relate = GetRelationship(c0->ParentChit(), c1->ParentChit());
		if (relate == ERelate::ENEMY) {
			SymmetricTK stk(c0->ParentChit()->Team(), c1->ParentChit()->Team());
			int idx = treaties.Find(stk);
			if (idx < 0) {
				// no treaty in place, of either kind.
				if (commit) {
					stk.peaceTimer = TREATY_TIME;
					treaties.Push(stk);
				}
				CalcAttitude(c0, c1, web);
				CalcAttitude(c1, c0, web);

				int a0 = Team::Instance()->Attitude(c0, c1);
				int a1 = Team::Instance()->Attitude(c1, c0);
				int a = Min(a0, a1);
				int cost = -a * 50;
				cost = Min(cost, 50);
				return cost;
			}
		}
	}
	return 0;
}


bool Team::AddSubteam(int super, int sub)
{
	GLASSERT(Team::ID(super));
	GLASSERT(Team::ID(sub));
	GLASSERT(Team::IsDenizen(super));
	GLASSERT(Team::IsDenizen(sub));

	// Removes all existing treaties:
	SymmetricTK stk(super, sub);
	treaties.Filter(stk, [](const SymmetricTK& stk, const SymmetricTK& item) {
		return stk != item;
	});

	// Run the array; make sure that 'sub' isn't a super.
	// Do nothing if exists, etc.
	for (const Control& c : control) {
		if (c.super == sub) {
			// already a sub-team.
			return false;
		}
		if (c.super == super && c.sub == sub) {
			// redundant
			return true;
		}
	}
	Control c = { super, sub };
	control.Push(c);
	return true;
}


void Team::CoreDestroyed(int team)
{
	if (!Team::IsDenizen(team)) {
		return;
	}
	GLASSERT(Team::ID(team));

	// Filter out all the existing control structures for the deleting core.
	control.Filter(team, [](int team, const Control& c) {
		return c.sub != team && c.super != team;
	});
}


int Team::SuperTeam(int team) const
{
	if (team == 0) return 0;
	for (const Control& c : control) {
		if (c.sub == team) {
			return c.super;
		}
	}
	return team;
}


bool Team::IsController(int team, grinliz::CDynArray<int>* subTeams) const
{
	if (subTeams) subTeams->Clear();
	bool rc = false;
	for (int i = 0; i < control.Size(); ++i) {
		if (control[i].super == team) {
			rc = true;
			if (subTeams) {
				subTeams->Push(control[i].sub);
			}
		}
	}
	return rc;
}


bool Team::IsControlled(int team, int* super) const
{
	for (const Control& c : control) {
		if (c.sub == team) {
			if (super) *super = c.super;
			return true;
		}
	}
	return false;
}