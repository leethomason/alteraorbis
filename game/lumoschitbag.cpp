#include "../grinliz/glarrayutil.h"

#include "lumoschitbag.h"
#include "gameitem.h"
#include "gamelimits.h"
#include "census.h"
#include "pathmovecomponent.h"
#include "aicomponent.h"
#include "debugstatecomponent.h"
#include "healthcomponent.h"
#include "mapspatialcomponent.h"
#include "reservebank.h"
#include "worldmap.h"
#include "worldgrid.h"
#include "worldinfo.h"
#include "gridmovecomponent.h"
#include "team.h"
#include "visitorstatecomponent.h"
#include "lumosmath.h"
#include "lumosgame.h"	// FIXME: namegen should be in script
#include "circuitsim.h"

#include "../ai/domainai.h"

#include "../scenes/characterscene.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/componentfactory.h"
#include "../xegame/chitcontext.h"

#include "../engine/model.h"
#include "../engine/engine.h"
#include "../engine/loosequadtree.h"
#include "../engine/particle.h"

#include "../audio/xenoaudio.h"

#include "../script/battlemechanics.h"
#include "../script/itemscript.h"
#include "../script/corescript.h"
#include "../script/plantscript.h"
#include "../script/procedural.h"
#include "../script/countdownscript.h"
#include "../script/corescript.h"
#include "../script/buildscript.h"
#include "../markov/markov.h"

#include "../xarchive/glstreamer.h"


//#define DEBUG_EXPLOSION

using namespace grinliz;

LumosChitBag::LumosChitBag(const ChitContext& c, Sim* s) : ChitBag(c), sceneID(-1), sceneData(0), sim(s)
{
	memset( mapSpatialHash, 0, sizeof(MapSpatialComponent*)*NUM_SECTORS*NUM_SECTORS);
	//memset(deityID, 0, sizeof(deityID[0])*NUM_DEITY);
	homeTeam = 0;
}


LumosChitBag::~LumosChitBag()
{
	// Call the parent function so that chits aren't 
	// aren't using deleted memory.
	DeleteAll();
	delete sceneData;
}


void LumosChitBag::Serialize( XStream* xs )
{
	super::Serialize( xs );

	XarcOpen( xs, "LumosChitBag" );
	XARC_SER( xs, homeTeam );
	XARC_SER_CARRAY(xs, namePool);
	XarcClose( xs );
}


void LumosChitBag::AddToBuildingHash( MapSpatialComponent* chit, int x, int y )
{
	if (x == 0 && y == 0) return; // sentinel
	Vector2I sector = ToSector(x, y);
	GLASSERT( chit->nextBuilding == 0 );

	int index = sector.y * NUM_SECTORS + sector.x;
	chit->nextBuilding = mapSpatialHash[index];
	mapSpatialHash[index] = chit;
}


void LumosChitBag::RemoveFromBuildingHash( MapSpatialComponent* chit, int x, int y )
{
	if (x == 0 && y == 0) return; // sentinel

	Vector2I sector = ToSector(x, y);
	int index = sector.y * NUM_SECTORS + sector.x;
	GLASSERT( mapSpatialHash[index] );

	MapSpatialComponent* prev = 0;
	for( MapSpatialComponent* it = mapSpatialHash[index]; it; prev = it, it = it->nextBuilding ) {
		if ( it == chit ) {
			if ( prev ) {
				prev->nextBuilding = it->nextBuilding;
			}
			else {
				mapSpatialHash[index] = it->nextBuilding;
			}
			it->nextBuilding = 0;
			return;
		}
	}
	GLASSERT( 0 );
}


void LumosChitBag::BuildingCounts(const Vector2I& sector, int* counts, int n)
{
	BuildScript buildScript;

	for( MapSpatialComponent* it = mapSpatialHash[sector.y*NUM_SECTORS+sector.x]; it; it = it->nextBuilding ) {
		Chit* chit = it->ParentChit();
		GLASSERT( chit );

		const GameItem* item = chit->GetItem();
		if (!item)
			continue;
		const IString& name = item->IName();

		int id = 0;
		if (!name.empty()) {
			buildScript.GetDataFromStructure(name, &id);
			if (id < n) {
				counts[id] += 1;
			}
		}
	}
}

Chit* LumosChitBag::FindBuildingCC(const grinliz::IString& name,		// particular building, or emtpy to match all
									const grinliz::Vector2I& sector,	// sector to query
									const grinliz::Vector2F* pos,		// optional IN: used for evaluating NEAREST, etc.
									LumosChitBag::EFindMode flags,
									CChitArray* arr,					// optional; the matches that fit
									IChitAccept* filter)				// optional; run this filter first
{
	chitArr.Clear();
	Chit* chit = FindBuilding(name, sector, pos, flags, &chitArr, filter);
	arr->Clear();
	for (int i = 0; i < chitArr.Size(); ++i) {
		if (arr->HasCap())
			arr->Push(chitArr[i]);
	}
	return chit;
}


Chit* LumosChitBag::FindBuilding(	const grinliz::IString&  name, 
									const grinliz::Vector2I& sector, 
									const grinliz::Vector2F* pos, 
									LumosChitBag::EFindMode flags,
									CDynArray<Chit*>* arr,
									IChitAccept* filter )
{
	CDynArray<Chit*>& match = arr ? *arr : findMatch;	// sleazy reference trick to point to either passed in or local.
	match.Clear();
	findWeight.Clear();

	for( MapSpatialComponent* it = mapSpatialHash[sector.y*NUM_SECTORS+sector.x]; it; it = it->nextBuilding ) {
		Chit* chit = it->ParentChit();
		GLASSERT( chit );
		if ( filter && !filter->Accept( chit )) {				// if a filter, check it.
			continue;
		}

		const GameItem* item = chit->GetItem();

		if ( item && ( name.empty() || item->IName() == name )) {	// name, if empty, matches everything
			match.Push( chit );
		}
	}

	// If we found nothing, or we don't care about the position, return early.
	// Else deal with choice / sorting / etc. below.
	if ( match.Empty() )
		return 0;
	if ( !pos )
		return match[0];

	// NEAREST scans and finds the closest one.
	// RANDOM_NEAR chooses one at random, but weighted by the (inverse) of the distance
	if ( flags == EFindMode::NEAREST ) {
		float closest = ( ToWorld2F(match[0]->Position()) - *pos ).LengthSquared();
		int   ci = 0;
		for( int i=1; i<match.Size(); ++i ) {
			float len2 = ( ToWorld2F(match[i]->Position()) - *pos ).LengthSquared();
			if ( len2 < closest ) {
				closest = len2;
				ci = i;
			}
		}
		return match[ci];
	}
	if ( flags == EFindMode::RANDOM_NEAR ) {
		for( int i=0; i<match.Size(); ++i ) {
			float len = ( ToWorld2F(match[i]->Position()) - *pos ).Length();
			if (len < 1) len = 1;
			findWeight.Push( 1.0f/len );
		}
		int index = random.Select( findWeight.Mem(), findWeight.Size() );
		return match[index];
	}

	// Bad flag? Something didn't return?
	GLASSERT( 0 );
	return 0;
}


Chit* LumosChitBag::NewBuilding(const Vector2I& pos, const char* name, int team)
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();

	const GameItem& rootItem = ItemDefDB::Instance()->Get(name);
	GameItem* item = rootItem.Clone();

	// Hack...how to do this better??
	if (item->IResourceName() == "pyramid0") {
		CStr<32> str;
		str.Format("pyramid%d", random.Rand(3));
		item->SetResource(str.c_str());
	}
	if (item->IResourceName() == ISC::kiosk) {
		switch (random.Rand(4)) {
			case 0: item->SetResource("kiosk.n");	break;
			case 1: item->SetResource("kiosk.m");	break;
			case 2: item->SetResource("kiosk.s");	break;
			default:item->SetResource("kiosk.c");	break;
		}
	}

	int size = 1;
	rootItem.keyValues.Get(ISC::size, &size);
	int porch = 0;
	rootItem.keyValues.Get(ISC::porch, &porch);
	const int circuit = 0;

	// Should be pre-cleared. But recover from weird situations by clearing.
	// Note that water is a real problem.
	Rectangle2I r;
	r.Set(pos.x, pos.y, pos.x + size - 1, pos.y + size - 1);
	for (Rectangle2IIterator it(r); !it.Done(); it.Next()) {
		const WorldGrid& wg = context->worldMap->GetWorldGrid(it.Pos());
		GLASSERT(wg.IsLand());
		(void)wg;
		context->worldMap->SetRock(it.Pos().x, it.Pos().y, 0, false, 0);
		context->worldMap->SetPlant(it.Pos().x, it.Pos().y, 0, 0);
	}

	MapSpatialComponent* msc = new MapSpatialComponent();
	msc->SetBuilding(size, porch != 0, circuit);
	msc->SetBlocks((rootItem.flags & GameItem::PATH_NON_BLOCKING) ? false : true);
	chit->Add(msc);
	MapSpatialComponent::SetMapPosition(chit, pos.x, pos.y);

	chit->Add(new RenderComponent(item->ResourceName()));
	chit->Add(new HealthComponent());
	AddItem(item, chit, context->engine, team, 0);

	IString script = rootItem.keyValues.GetIString("script");
	if (!script.empty()) {
		Component* s = ComponentFactory::Factory(script.c_str(), &chitContext);
		GLASSERT(s);
		chit->Add(s);
	}

	IString proc = rootItem.keyValues.GetIString("procedural");
	if (!proc.empty()) {
		ProcRenderInfo info;
		AssignProcedural(chit->GetItem(), &info);
		chit->GetRenderComponent()->SetProcedural(0, info);
	}

	IString consumes = rootItem.keyValues.GetIString(ISC::zone);
	if (!consumes.empty()) {
		Component* s = ComponentFactory::Factory("EvalBuildingScript", &chitContext);
		GLASSERT(s);
		chit->Add(s);
	}

	IString nameGen = rootItem.keyValues.GetIString( "nameGen");
	if ( !nameGen.empty() ) {
		IString p = Context()->chitBag->NameGen(nameGen.c_str(), chit->random.Rand());
		chit->GetItem()->SetProperName( p );
	}

#if 0	// debugging
	SectorPort sp;
	sp.sector.Set( pos.x/SECTOR_SIZE, pos.y/SECTOR_SIZE );
	sp.port = 1;
	worldMap->SetRandomPort( sp );
#endif

	context->engine->particleSystem->EmitPD( ISC::constructiondone, ToWorld3F( pos ), V3F_UP, 0 );

	if (XenoAudio::Instance()) {
		Vector3F pos3 = ToWorld3F(pos);
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &pos3);
	}

	return chit;
}


Chit* LumosChitBag::NewLawnOrnament(const Vector2I& pos, const char* name, int team)
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();

	GameItem* rootItem = ItemDefDB::Instance()->Get(name).Clone();

	// Hack...how to do this better??
	if (rootItem->IResourceName() == "ruins1.0") {
		CStr<32> str;
		str.Format("ruins1.%d", random.Rand(2));
		rootItem->SetResource(str.c_str());
	}

	int size = 1;
	rootItem->keyValues.Get(ISC::size, &size);

	MapSpatialComponent* msc = new MapSpatialComponent();
	msc->SetBuilding(size, false, 0);
	msc->SetBlocks((rootItem->flags & GameItem::PATH_NON_BLOCKING) ? false : true);
	chit->Add(msc);
	MapSpatialComponent::SetMapPosition(chit, pos.x, pos.y);


	chit->Add(new RenderComponent(rootItem->ResourceName()));
	chit->Add(new HealthComponent());
	AddItem(rootItem, chit, context->engine, team, 0);

	IString proc = rootItem->keyValues.GetIString("procedural");
	if (!proc.empty()) {
		ProcRenderInfo info;
		AssignProcedural(chit->GetItem(), &info);
		chit->GetRenderComponent()->SetProcedural(0, info);
	}
	
	context->engine->particleSystem->EmitPD(ISC::constructiondone, ToWorld3F(pos), V3F_UP, 0);

	if (XenoAudio::Instance()) {
		Vector3F pos3 = ToWorld3F(pos);
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &pos3);
	}

	return chit;
}


Chit* LumosChitBag::NewMonsterChit(const Vector3F& pos, const char* name, int team)
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();

	AddItem( name, chit, context->engine, team, 0 );

	chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
	chit->Add( new PathMoveComponent());
	chit->Add( new AIComponent());

	chit->SetPosition( pos );

	chit->Add( new HealthComponent());

	IString mob = chit->GetItem()->keyValues.GetIString(ISC::mob);
	if (ReserveBank::Instance()) {
		ReserveBank::Instance()->WithdrawMonster(chit->GetWallet(), mob == ISC::greater);
	}
	if (mob == ISC::greater) {
		// Mark this item as important with a destroyMsg:
		chit->GetItem()->SetSignificant(GetNewsHistory(), ToWorld2F(pos), NewsEvent::GREATER_MOB_CREATED, NewsEvent::GREATER_MOB_KILLED, 0);
	}
	chit->GetItem()->GetTraitsMutable()->Roll(chit->ID());

	if (XenoAudio::Instance()) {
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &pos);
	}
	return chit;
}


Chit* LumosChitBag::NewDenizen( const grinliz::Vector2I& pos, int team )
{
	const ChitContext* context = Context();
	IString itemName;

	switch (Team::Group(team)) {
		case TEAM_HOUSE:	itemName = (random.Bit()) ? ISC::humanFemale : ISC::humanMale;	break;
		case TEAM_GOB:		itemName = ISC::gobman;											break;
		case TEAM_KAMAKIRI:	itemName = ISC::kamakiri;										break;
		default: GLASSERT(0); break;
	}

	Chit* chit = NewChit();
	const GameItem& root = ItemDefDB::Instance()->Get(itemName.safe_str());

	chit->Add( new RenderComponent(root.ResourceName()));
	chit->Add( new PathMoveComponent());

	const char* altName = 0;
	if (Team::Group(team) == TEAM_HOUSE) {
		altName = "human";
	}
	AddItem(root.Name(), chit, context->engine, team, 0, 0, altName);

	ReserveBank::Instance()->WithdrawDenizen(chit->GetWallet());
	chit->GetItem()->GetTraitsMutable()->Roll( random.Rand() );
	chit->GetItem()->GetPersonalityMutable()->Roll( random.Rand(), &chit->GetItem()->Traits() );

	IString nameGen = chit->GetItem()->keyValues.GetIString( "nameGen" );
	if ( !nameGen.empty() ) {
		LumosChitBag* chitBag = chit->Context()->chitBag;
		if ( chitBag ) {
			chit->GetItem()->SetProperName(chitBag->NameGen(nameGen.c_str(), chit->ID()));
		}
	}

	AIComponent* ai = new AIComponent();
	chit->Add( ai );

	chit->Add( new HealthComponent());
	chit->SetPosition( (float)pos.x+0.5f, 0, (float)pos.y+0.5f );

	chit->GetItem()->SetSignificant(GetNewsHistory(), ToWorld2F(pos), NewsEvent::DENIZEN_CREATED, NewsEvent::DENIZEN_KILLED, 0);

	if (XenoAudio::Instance()) {
		Vector3F pos3 = ToWorld3F(pos);
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &pos3);
	}

	return chit;
}


Chit* LumosChitBag::NewWorkerChit( const Vector3F& pos, int team )
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "worker" );

	chit->Add( new RenderComponent( rootItem.ResourceName() ));
	chit->Add( new PathMoveComponent());
	chit->Add( new AIComponent());
	chit->SetPosition( pos );
	AddItem( rootItem.Name(), chit, context->engine, team, 0 );
	chit->Add( new HealthComponent());
	//chit->Add( new DebugStateComponent());

	if (XenoAudio::Instance()) {
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &pos);
	}
	return chit;
}


Chit* LumosChitBag::NewVisitor( int visitorIndex, const Web& web)
{
	const ChitContext* context = Context();
	if (web.Empty()) return 0;

	Vector2I startSector = { NUM_SECTORS / 2, NUM_SECTORS / 2 };
	CoreScript* cs = CoreScript::GetCore(startSector);
	if (!cs) return 0;	// cores get deleted, web is cached, etc.
	Vector3F pos = cs->ParentChit()->Position();

	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "visitor" );

	chit->Add( new RenderComponent( rootItem.ResourceName() ));

	AIComponent* ai = new AIComponent();
	chit->Add( ai );
	GLASSERT( visitorIndex >= 0 && visitorIndex < Visitors::NUM_VISITORS );
	ai->SetVisitorIndex( visitorIndex );
	Visitors::Instance()->visitorData[visitorIndex].Connect();	// initialize.

	// Visitors start at world center, with gridMove, and go from there.
	chit->SetPosition( pos );

	PathMoveComponent* pmc = new PathMoveComponent();
	chit->Add(pmc);

	AddItem( rootItem.Name(), chit, context->engine, TEAM_VISITOR, 0 );
	chit->Add( new HealthComponent());
	//chit->Add( new VisitorStateComponent());
	return chit;
}


#if 0
Chit* LumosChitBag::QueryRemovable( const grinliz::Vector2I& pos2i )
{
	Vector2F pos2 = { (float)pos2i.x+0.5f, (float)pos2i.y+0.5f };

	CChitArray		array;
	RemovableFilter removableFilter;

	Chit* core = 0;
	CoreScript* cs = CoreScript::GetCore(ToSector(pos2i));
	if (cs) core = cs->ParentChit();


	// Big enough to snag buildings:
	QuerySpatialHash(&array, pos2, 0.8f, core, &removableFilter);

	Chit* found = 0;
	// 1x1 buildings.
	for( int i=0; i<array.Size(); ++i ) {
		// We are casting a wide net to get buildings.
		if (array[i]->Bounds().Contains(pos2i)) {
			found = array[i];
			break;
		}
	}
	return found;
}
#endif


Chit* LumosChitBag::QueryBuilding( const IString& name, const grinliz::Rectangle2I& bounds, CChitArray* arr )
{
	GLASSERT( MAX_BUILDING_SIZE == 2 );	// else adjust logic
	Vector2I sector = ToSector( bounds.min );

	for( MapSpatialComponent* it = mapSpatialHash[SectorIndex(sector)]; it; it = it->nextBuilding ) {
		if ( it->Bounds().Intersect( bounds )) {
			Chit* chit = it->ParentChit();
			if (name.empty() || (chit->GetItem() && chit->GetItem()->IName() == name)) {
				if (!arr) {
					return chit;
				}
				if (arr->HasCap()) {
					arr->Push(chit);
				}
			}
		}
	}
	if (arr && !arr->Empty()) {
		return (*arr)[0];
	}
	return 0;
}


Chit* LumosChitBag::QueryPorch( const grinliz::Vector2I& pos )
{
	Vector2I sector = ToSector(pos);
	for( MapSpatialComponent* it = mapSpatialHash[SectorIndex(sector)]; it; it = it->nextBuilding ) {
		if ( it->PorchPos().Contains( pos )) {
			return it->ParentChit();
		}
	}
	return 0;
}


Chit* LumosChitBag::NewGoldChit( const grinliz::Vector3F& pos, Wallet* src )
{
	GLASSERT(src);
	if (src->Gold() == 0) return 0;

	const ChitContext* context = Context();

	Vector2F v2 = { pos.x, pos.z };

	ItemNameFilter goldFilter( ISC::gold);
	this->QuerySpatialHash( &chitList, v2, 1.0f, 0, &goldFilter );
	Chit* chit = 0;

	// Evil bug this: adding gold to a wallet just before
	// deletion. I'm a little concerned where else
	// this could be a problem. Would be nice to make
	// deletion immediate.
	for (int i = 0; i < chitList.Size(); ++i) {
		Chit* c = chitList[i];
		if (c && !IsQueuedForDelete(c) && c->GetWallet() && !c->GetWallet()->Closed()) {
			chit = chitList[i];
			break;
		}
	}

	if ( !chit ) {
		chit = this->NewChit();
		AddItem( "gold", chit, context->engine, 0, 0 );
		chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
		chit->SetPosition( pos );
		chit->Add(new GameMoveComponent());
	}
	chit->GetWallet()->Deposit(src, src->Gold());
	return chit;
}


Chit* LumosChitBag::NewCrystalChit( const grinliz::Vector3F& _pos, Wallet* src, bool fuzz )
{
	Vector3F pos = _pos;
	if ( fuzz ) {
		pos.x = floorf(_pos.x) + random.Uniform();
		pos.z = floorf(_pos.z) + random.Uniform();
	}

	int crystal = -1;
	for (int i = 0; i < NUM_CRYSTAL_TYPES; ++i) {
		if (src->Crystal(i)) {
			crystal = i;
			break;
		}
	}

	if (crystal == -1) return 0;	// done

	const char* name = 0;
	switch ( crystal ) {
	case CRYSTAL_GREEN:		name="crystal_green";	break;
	case CRYSTAL_RED:		name="crystal_red";		break;
	case CRYSTAL_BLUE:		name="crystal_blue";	break;
	case CRYSTAL_VIOLET:	name="crystal_violet";	break;
	}

	const ChitContext* context = Context();

	Chit* chit = this->NewChit();
	AddItem( name, chit, context->engine, 0, 0 );
	chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
	chit->Add(new GameMoveComponent());

	chit->SetPosition( pos );
	
	int c[NUM_CRYSTAL_TYPES] = { 0 };
	c[crystal] = 1;

	chit->GetWallet()->Deposit(src, 0, c);

	return chit;
}


void LumosChitBag::NewWalletChits( const grinliz::Vector3F& pos, Wallet* wallet )
{
	NewGoldChit(pos, wallet);
	for( int i=0; i<NUM_CRYSTAL_TYPES; ++i ) {
		while (NewCrystalChit(pos, wallet, true)) {}
	}
}


Chit* LumosChitBag::NewWildFruit(const grinliz::Vector2I& pos)
{
	Vector3F pos3 = ToWorld3F(pos);
	pos3.x = floorf(pos3.x) + random.Uniform();
	pos3.z = floorf(pos3.z) + random.Uniform();

	const GameItem& root = ItemDefDB::Instance()->Get("fruit");
	GameItem* item = root.Clone();
	item->SetProperName(ISC::wildFruit);

	Chit* chit = this->NewChit();
	chit->Add(new ItemComponent(item));
	chit->Add(new RenderComponent(item->ResourceName()));
	chit->Add(new GameMoveComponent());
	chit->Add(new HealthComponent());

	chit->SetPosition(pos3);
	return chit;
}


Chit* LumosChitBag::NewItemChit( const grinliz::Vector3F& _pos, GameItem* orphanItem, bool fuzz, bool onGround, int selfDestructTimer )
{
	GLASSERT( !orphanItem->Intrinsic() );
	GLASSERT( !orphanItem->IResourceName().empty() );

	Vector3F pos = _pos;
	if ( fuzz ) {
		pos.x = floorf(pos.x) + random.Uniform();
		pos.z = floorf(pos.z) + random.Uniform();
	}

	Chit* chit = this->NewChit();
	chit->Add( new ItemComponent( orphanItem ));
	chit->Add( new RenderComponent( orphanItem->ResourceName() ));
	chit->Add(new GameMoveComponent());

	if ( onGround ) {
		const ModelResource* res = chit->GetRenderComponent()->MainResource();
		Rectangle3F aabb = res->AABB();
		pos.y = -aabb.min.y;

		if ( aabb.SizeY() < 0.1f ) {
			pos.y += 0.1f;
		}
	}
	chit->SetPosition( pos );
	chit->Add( new HealthComponent());

	if (!selfDestructTimer && orphanItem->keyValues.Has(ISC::selfDestruct)) {
		orphanItem->keyValues.Get("selfDestruct", &selfDestructTimer);
		selfDestructTimer *= 1000;
	}
	if ( selfDestructTimer ) {
		chit->Add( new CountDownScript( selfDestructTimer ));
	}
	return chit;
}


void LumosChitBag::HandleBolt( const Bolt& bolt, const ModelVoxel& mv )
{
	const ChitContext* context = Context();
	Chit* chitShooter = GetChit( bolt.chitID );	// may be null
	int shooterTeam = -1;
	if ( chitShooter && chitShooter->GetItemComponent() ) {
		shooterTeam = chitShooter->GetItemComponent()->GetItem()->Team();
	}
	int explosive = bolt.effect & GameItem::EFFECT_EXPLOSIVE;
 
	if ( !explosive ) {
		if ( mv.Hit() ) {
			Chit* chitHit = mv.ModelHit() ? mv.model->userData : 0;
			DamageDesc dd( bolt.damage, bolt.effect );

			if ( chitHit ) {
				GLASSERT( GetChit( chitHit->ID() ) == chitHit );
				if ( chitHit->GetItemComponent() &&
					 chitHit->GetItemComponent()->GetItem()->Team() == shooterTeam ) 
				{
					// do nothing. don't shoot own team.
				}
				else {
		
					ChitDamageInfo info( dd );
					info.originID = bolt.chitID;
					info.awardXP  = true;
					info.isMelee  = false;
					info.isExplosion = false;
					info.originOfImpact = mv.at;

					ChitMsg msg( ChitMsg::CHIT_DAMAGE, 0, &info );
					chitHit->SendMessage( msg, 0 );
				}
			}
			else if ( mv.VoxelHit() ) {
				context->worldMap->VoxelHit( mv.voxel, dd );
			}
		}
	}
	else {
		// How it used to work. Now only uses radius:
			// Here don't worry about the chit hit. Just ray cast to see
			// who is in range of the explosion and takes damage.
		
			// Back up the origin of the bolt just a bit, so it doesn't keep
			// intersecting the model it hit. Then do ray checks around to 
			// see what gets hit by the explosion.

		float rewind = Min( 0.1f, 0.5f*bolt.len );
		GLASSERT( Equal( bolt.dir.Length(), 1.f, 0.001f ));
		Vector3F origin = mv.at - bolt.dir * rewind;

		DamageDesc dd( bolt.damage, bolt.effect );
		BattleMechanics::GenerateExplosion( dd, origin, bolt.chitID, context->engine, this, context->worldMap );
	}
}


GameItem* LumosChitBag::AddItem(const char* name, Chit* chit, Engine* engine, int team, int level, const char* altRes, const char* altName)
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get(name, &itemDefArr);
	GLASSERT(itemDefArr.Size() > 0);

	GameItem* item = itemDefArr[0]->Clone();
	if (altName) {
		item->SetName(altName);
	}
	if (altRes) {
		item->SetResource(altRes);
	}
	item->Roll(team, item->Traits().Get());
	item->GetTraitsMutable()->SetExpFromLevel( level );

	if ( !chit->GetItemComponent() ) {
		ItemComponent* ic = new ItemComponent( item );
		chit->Add( ic );
		for( int i=1; i<itemDefArr.Size(); ++i ) {
			GameItem* gi = itemDefArr[i]->Clone();
			ic->AddToInventory(gi);
		}
	}
	else {
		GLASSERT( itemDefArr.Size() == 1 );
		chit->GetItemComponent()->AddToInventory(item);
	}
	return item;
}


GameItem* LumosChitBag::AddItem(GameItem* item, Chit* chit, Engine* engine, int team, int level)
{
	item->Roll(team, item->Traits().Get());
	item->GetTraitsMutable()->SetExpFromLevel( level );

	if ( !chit->GetItemComponent() ) {
		ItemComponent* ic = new ItemComponent( item );
		chit->Add( ic );
	}
	else {
		chit->GetItemComponent()->AddToInventory(item);
	}
	return item;
}


// Only MapSpatialComponents block the grid, and there
// is a linked list for every sector.
bool LumosChitBag::MapGridBlocked(int x, int y)
{
	Vector2I sector = ToSector(x, y);
	MapSpatialComponent* mscIt = mapSpatialHash[sector.y*NUM_SECTORS + sector.x];
	for (; mscIt; mscIt = mscIt->nextBuilding) {
		if (mscIt->Blocks() && mscIt->Bounds().Contains(x, y)) {
			return true;
		}
	}
	return false;
}


CoreScript* LumosChitBag::GetHomeCore()	const
{ 
	if (homeTeam) {
		return CoreScript::GetCoreFromTeam(homeTeam);
	}
	return 0;
}


Chit* LumosChitBag::GetAvatar() const
{
	CoreScript* cs = GetHomeCore();
	if (cs) {
		Chit* prime = cs->PrimeCitizen();
		return prime;	// The prime citizen of the home domain is the Avatar.
	}
	return 0;
}



Vector2I LumosChitBag::GetHomeSector()	const
{ 
	grinliz::Vector2I v = { 0, 0 };
	CoreScript* home = GetHomeCore();
	if (home && home->ParentChit() ) {
		v = ToSector(home->ParentChit()->Position());
	}
	return v;
}


CameraComponent* LumosChitBag::GetCamera() const
{
	Chit* chit = GetNamedChit(ISC::Camera);
	if (chit) {
		GLASSERT(chit);
		CameraComponent* cc = (CameraComponent*) chit->GetComponent("CameraComponent");
		GLASSERT(cc);
		return cc;
	}
	return 0;
}


bool ItemFlagFilter::Accept( Chit* chit )
{
	GameItem* item = chit->GetItem();
	if ( !item ) return false;

	if ( ( (item->flags & required) == required ) &&
		 ( (item->flags & excluded) == 0 ))
	{
		return true;
	}
	return false;
}


ItemNameFilter::ItemNameFilter()
{
	names = 0;
	count  = 0;
}



ItemNameFilter::ItemNameFilter( const grinliz::IString& istring)
{
	names = &istring;
	count  = 1;
}


ItemNameFilter::ItemNameFilter( const grinliz::IString* arr, int n)
{
	names = arr;
	count  = n;
}


bool ItemNameFilter::Accept( Chit* chit )
{
	const GameItem* item = chit->GetItem();
	if ( item ) {
		const IString& key = item->IName();
		for( int i=0; i<count; ++i ) {
			if ( key == names[i] )
				return true;
		}
	}
	return false;
}


GoldCrystalFilter::GoldCrystalFilter()
{
	arr[0] = ISC::gold;
	arr[1] = ISC::crystal_green;
	arr[2] = ISC::crystal_red;
	arr[3] = ISC::crystal_blue;
	arr[4] = ISC::crystal_violet;
	names = arr;
	count = 5;
}


bool BuildingFilter::Accept( Chit* chit ) 
{
	// Assumed to be MapSpatial with "building" flagged on.
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	return msc != 0;
}

bool BuildingWithPorchFilter::Accept(Chit* chit)
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	if (msc && msc->HasPorch()) {
		return true;
	}
	return false;
}

bool BuildingRepairFilter::Accept(Chit* chit)
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	if (msc ) { //&& msc->Building()) {
		GameItem* item = chit->GetItem();
		if (item && item->HPFraction() < 0.9f) {
			return true;
		}
	}
	return false;
}


void RelationshipFilter::CheckRelationship(Chit* _compareTo, ERelate _relationship)
{
	team = _compareTo->Team();
	relationship = _relationship;
}


void RelationshipFilter::CheckRelationship(int _team, ERelate _relationship)
{
	team = _team;
	relationship = _relationship;
}


bool RelationshipFilter::Accept( Chit* chit )
{
	if (team < 0) return true;	// not checking.
	return Team::Instance()->GetRelationship(team, chit->Team()) == relationship;
}


bool MOBKeyFilter::Accept(Chit* chit)
{
	GameItem* item = chit->GetItem();
	if (item) {
		IString mob = item->keyValues.GetIString(ISC::mob);
		if (!mob.empty()) {
			if (value.empty())
				return RelationshipFilter::Accept(chit);
			else if (value == mob)
				return RelationshipFilter::Accept(chit);
		}
	}
	return false;
}


bool MOBIshFilter::Accept(Chit* chit)
{
	// If it can move and has a team...?
	// Mostly a good metric. Doesn't account for dummy targets.
	PathMoveComponent* pmc = GET_SUB_COMPONENT(chit, MoveComponent, PathMoveComponent);
	if (pmc && chit->Team()) {
		return RelationshipFilter::Accept(chit);
	}
	GameItem* item = chit->GetItem();
	if (item && item->IName() == ISC::dummyTarget) {
		return RelationshipFilter::Accept(chit);
	}
	return false;
}


bool BattleFilter::Accept(Chit* chit)
{
	return chit->GetRenderComponent() != 0;
}


bool RemovableFilter::Accept( Chit* chit )
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	if (msc) {
		GameItem* item = chit->GetItem();
		return (item && !(item->flags & GameItem::INDESTRUCTABLE));
	}
	return false;
}


bool ChitHasMapSpatial::Accept( Chit* chit ) 
{
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	return msc != 0;
}


bool LootFilter::Accept( Chit* chit )
{
	/*
		Currently weapons and shields. Could be
		expanded to other sorts of things when
		they become available. (Keys? Decoration?)
	*/
	const GameItem* item = chit->GetItem();
	if ( item ) {
		if (    item->ToMeleeWeapon()
			|| item->ToRangedWeapon()
			|| item->ToShield() ) 
		{
			return true;
		}
	}
	return  false;
}

bool FruitElixirFilter::Accept(Chit* chit)
{
	const GameItem* item = chit->GetItem();
	if (item) {
		return item->IName() == ISC::fruit || item->IName() == ISC::elixir;
	}
	return false;
}


bool WeaponFilter::Accept(Chit* chit)
{
	const GameItem* item = chit->GetItem();
	if (item) {
		IString istr = item->IName();
		if (istr == ISC::shield
			|| istr == ISC::ring
			|| istr == ISC::pistol
			|| istr == ISC::blaster
			|| istr == ISC::pulse
			|| istr == ISC::beamgun) {
			return true;
		}
	}
	return false;
}


bool TeamFilter::Accept(Chit* chit)
{
	return chit->Team() == team;
}


Bolt* LumosChitBag::NewBolt(	const Vector3F& pos,
								Vector3F dir,
								int effectFlags,
								int chitID,
								float damage,
								float speed,
								bool trail )
{
	Bolt* bolt = ChitBag::NewBolt();

	dir.Normalize();
	bolt->head = pos + dir*0.5f;
	bolt->len = 0.5f;
	bolt->dir = dir;

	const Game::Palette* palette = Game::GetMainPalette();

	switch( effectFlags & (GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK) ) {
	case 0:													bolt->color = palette->Get4F( 1, PAL_GREEN );	break;
	case GameItem::EFFECT_FIRE:								bolt->color = palette->Get4F( 1, PAL_RED );	break;
	case GameItem::EFFECT_SHOCK:							bolt->color = palette->Get4F( 1, PAL_BLUE );	break;
	case GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK:	bolt->color = palette->Get4F( 1, PAL_PURPLE );	break;
	default:
		GLASSERT( 0 );
		break;
	}

	bolt->chitID = chitID;
	bolt->damage = damage;
	bolt->effect = effectFlags;
	bolt->particle  = trail;
	bolt->speed = speed;

	return bolt;
}


void LumosChitBag::PushScene( int id, SceneData* data )
{
	if ( sceneID >= 0 ) {
//		GLLOG(( "Dupe scene pushed.\n" ));
		delete data;
	}
	else {
		sceneID = id;
		sceneData = data;
	}
}


bool LumosChitBag::PopScene( int* id, SceneData** data )
{
	if ( sceneID >= 0 ) {
		*id = sceneID;
		*data = sceneData;
		sceneID = -1;
		sceneData = 0;
		return true;
	}
	return false;
}


IString LumosChitBag::NameGen(const char* dataset, int seed)
{
	const gamedb::Reader* database = Context()->game->GetDatabase();
	const gamedb::Item* parent = database->Root()->Child("markovName");
	GLASSERT(parent);
	const gamedb::Item* item = parent->Child(dataset);
	GLASSERT(item);
	if (!item) return IString();
	const gamedb::Item* names = item->Child("names");

	int id = 0;

	if (names) {
		IString ds = StringPool::Intern(dataset);
		bool found = false;
		for (int i = 0; i < namePool.Size(); ++i) {
			if (namePool[i].dataset == ds) {
				namePool[i].id++;
				id = namePool[i].id;
				found = true;
			}
		}
		if (!found) {
			NamePoolID entry = { ds, id };
			id = 0;
			namePool.Push(entry);
		}
	}
	return StaticNameGen(database, dataset, id);
}

IString LumosChitBag::StaticNameGen(const gamedb::Reader* database, const char* dataset, int seed)
{
	const gamedb::Item* parent = database->Root()->Child("markovName");
	GLASSERT(parent);
	const gamedb::Item* item = parent->Child(dataset);
	GLASSERT(item);
	if (!item) return IString();
	const gamedb::Item* names = item->Child("names");
	GLASSERT(names);

	int index = abs(seed) % names->NumChildren();
	const char* n = names->ChildAt(index)->GetString("name");
	return StringPool::Intern(n);
}


void LumosChitBag::NamePoolID::Serialize(XStream* xs)
{
	XarcOpen(xs, "NamePoolID");
	XARC_SER(xs, this->dataset);
	XARC_SER(xs, this->id);
	XarcClose(xs);
}


void LumosChitBag::DoTick(U32 delta)
{
	// From the CHIT_DESTROYED_START we have a list of
	// Cores that will be going away...check them here,
	// so that we replace as soon as possible.
	while (!coreCreateList.Empty()) {
		CreateCoreData data = coreCreateList.Pop();
		CoreScript* sc = CoreScript::GetCore(data.sector);
		Vector2I sector = data.sector;
		CoreScript* conqueringCore = CoreScript::GetCoreFromTeam(data.conqueringTeam);

		if (!sc) {
			// FIXME: last criteria "must be super team" isn't correct. Sub-teams
			// should be able to conquor for super teams.

			if (data.wantsTakeover
				&& data.conqueringTeam
				&& conqueringCore
				&& Team::IsDenizen(data.conqueringTeam)
				&& (Team::Instance()->SuperTeam(data.conqueringTeam) == data.conqueringTeam))
			{
				// The case where this domain is conquered. Switch to a sub-domain team ID,
				// and switch the existing team over. Intentionally limit to CChitArray items so there
				// isn't a full switch over.
				// Also, remember by the time this code is executed, the team will be Rogue.

				int teamID = Team::Instance()->GenTeam(Team::Group(data.conqueringTeam));
				Team::Instance()->AddSubteam(data.conqueringTeam, teamID);
				CoreScript* newCore = CoreScript::CreateCore(sector, teamID, Context());
				GLASSERT(CoreScript::GetCore(sector) == newCore);
				newCore->ParentChit()->Add(DomainAI::Factory(teamID));

				CChitArray arr;
				TeamFilter filter(Team::Group(data.defeatedTeam));	// use the group since this is a rogue team.
				Context()->chitBag->QuerySpatialHash(&arr, InnerSectorBounds(sector), 0, &filter);
				for (Chit* c : arr) {
					if (c->PlayerControlled()) continue;

					if (c->GetItem()->IsDenizen()) {
						c->GetItem()->SetRogue();
						newCore->AddCitizen(c);
					}
					else {
						c->GetItem()->SetTeam(teamID);
					}
				}
				// Finally, give this new core a chance. 
				// Transferm money from Conquering domain.
				int gold = conqueringCore->ParentChit()->GetWallet()->Gold() / 4;
				gold = Min(gold, 500);
				newCore->ParentChit()->GetWallet()->Deposit(conqueringCore->ParentChit()->GetWallet(), gold);
			}
			else {
				CoreScript::CreateCore(sector, TEAM_NEUTRAL, Context());
			}
		}
	}
	super::DoTick(delta);
}
