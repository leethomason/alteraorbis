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

#include "../scenes/characterscene.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/spatialcomponent.h"
#include "../xegame/istringconst.h"
#include "../xegame/componentfactory.h"

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
	memset(deityID, 0, sizeof(deityID[0])*NUM_DEITY);
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
	XARC_SER_ARR(xs, deityID, NUM_DEITY);
	XARC_SER_CARRAY(xs, namePool);
	XarcClose( xs );
}


#if 0
void LumosChitBag::AddToSpatialHash( Chit* chit, int x, int y )
{
	//MoBFilter filter : This seems like a good idea, accept that the chit can
	// of course change its components, has an update sequence,
	// and may or may not match criteria. 
	Vector2I sector = { 0, 0 };
	if ( worldMap->UsingSectors() ) {
		sector.Set( x / SECTOR_SIZE, y / SECTOR_SIZE );
	}
	chitsInSector[sector.y*NUM_SECTORS+sector.x].Push( chit );
	super::AddToSpatialHash( chit, x, y );
}
#endif


void LumosChitBag::AddToBuildingHash( MapSpatialComponent* chit, int x, int y )
{
	int sx = x / SECTOR_SIZE;
	int sy = y / SECTOR_SIZE;
	GLASSERT( sx >= 0 && sx < NUM_SECTORS );
	GLASSERT( sy >= 0 && sy < NUM_SECTORS );
	GLASSERT( chit->nextBuilding == 0 );
	
	chit->nextBuilding = mapSpatialHash[sy*NUM_SECTORS+sx];
	mapSpatialHash[sy*NUM_SECTORS+sx] = chit;
}


void LumosChitBag::RemoveFromBuildingHash( MapSpatialComponent* chit, int x, int y )
{
	int sx = x / SECTOR_SIZE;
	int sy = y / SECTOR_SIZE;
	GLASSERT( sx >= 0 && sx < NUM_SECTORS );
	GLASSERT( sy >= 0 && sy < NUM_SECTORS );
	GLASSERT( mapSpatialHash[sy*NUM_SECTORS+sx] );

	MapSpatialComponent* prev = 0;
	for( MapSpatialComponent* it = mapSpatialHash[sy*NUM_SECTORS+sx]; it; prev = it, it = it->nextBuilding ) {
		if ( it == chit ) {
			if ( prev ) {
				prev->nextBuilding = it->nextBuilding;
			}
			else {
				mapSpatialHash[sy*NUM_SECTORS+sx] = it->nextBuilding;
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
									int flags,
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
									int flags,
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
	if ( flags == NEAREST ) {
		float closest = ( match[0]->GetSpatialComponent()->GetPosition2D() - *pos ).LengthSquared();
		int   ci = 0;
		for( int i=1; i<match.Size(); ++i ) {
			float len2 = ( match[i]->GetSpatialComponent()->GetPosition2D() - *pos ).LengthSquared();
			if ( len2 < closest ) {
				closest = len2;
				ci = i;
			}
		}
		return match[ci];
	}
	if ( flags == RANDOM_NEAR ) {
		for( int i=0; i<match.Size(); ++i ) {
			float len = ( match[i]->GetSpatialComponent()->GetPosition2D() - *pos ).Length();
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

	int cx = 1;
	rootItem.keyValues.Get(ISC::size, &cx);
	int porch = 0;
	rootItem.keyValues.Get(ISC::porch, &porch);
	int circuit = CircuitSim::NameToID(rootItem.keyValues.GetIString(ISC::circuit));

	// FIXME: remove this check, because fluid can trigger?
	GLASSERT(context->worldMap->IsPassable(pos.x, pos.y));

	MapSpatialComponent* msc = new MapSpatialComponent();
	msc->SetMapPosition(pos.x, pos.y, cx, cx);
	msc->SetMode(GRID_BLOCKED);
	msc->SetBuilding(porch != 0, circuit);
	chit->Add(msc);

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
		LumosGame* game = Context()->game;
		IString p = Context()->chitBag->NameGen(nameGen.c_str(), chit->random.Rand(), 0, 0);
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
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &ToWorld3F(pos));
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

	int cx = 1;
	rootItem->keyValues.Get(ISC::size, &cx);

	MapSpatialComponent* msc = new MapSpatialComponent();
	msc->SetMapPosition(pos.x, pos.y, cx, cx);
	msc->SetMode(GRID_BLOCKED);

	chit->Add(msc);
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
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &ToWorld3F(pos));
	}

	return chit;
}


Chit* LumosChitBag::GetDeity(int id)
{
	GLASSERT(id >= 0 && id < NUM_DEITY);
	Chit* chit = GetChit(deityID[id]);
	
	if (!chit) {
		const ChitContext* context = Context();
		chit = NewChit();
		deityID[id] = chit->ID();

		const char* NAME[NUM_DEITY] = { "MotherCore", "QCore", "R1kCore", "Truulga", "BeastCore", "ShogScrift" };
		AddItem("deity", chit, context->engine, 0, 0);
		chit->GetItem()->SetProperName(NAME[id]);
	}

	if (!chit->GetSpatialComponent()) {
		// Have a spatial component but not a render component.
		// Used to set the "focus" of the deity, and mark
		// the location of a home core. (Truulga's base, for 
		// example.)
		chit->Add(new SpatialComponent());
	}

	return chit;
}


Chit* LumosChitBag::NewMonsterChit(const Vector3F& pos, const char* name, int team)
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();

	chit->Add( new SpatialComponent());
	AddItem( name, chit, context->engine, team, 0 );

	chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
	chit->Add( new PathMoveComponent());
	chit->Add( new AIComponent());

	chit->GetSpatialComponent()->SetPosition( pos );

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

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent(root.ResourceName()));
	chit->Add( new PathMoveComponent());

	AddItem( root.Name(), chit, context->engine, team, 0 );
	ReserveBank::Instance()->WithdrawDenizen(chit->GetWallet());
	chit->GetItem()->GetTraitsMutable()->Roll( random.Rand() );
	chit->GetItem()->GetPersonalityMutable()->Roll( random.Rand(), &chit->GetItem()->Traits() );

	IString nameGen = chit->GetItem()->keyValues.GetIString( "nameGen" );
	if ( !nameGen.empty() ) {
		LumosChitBag* chitBag = chit->Context()->chitBag;
		if ( chitBag ) {
			chit->GetItem()->SetProperName(chitBag->NameGen(nameGen.c_str(), chit->ID(), 4, 8));
		}
	}

	AIComponent* ai = new AIComponent();
	chit->Add( ai );

	chit->Add( new HealthComponent());
	chit->GetSpatialComponent()->SetPosYRot( (float)pos.x+0.5f, 0, (float)pos.y+0.5f, 0 );

	Vector2I sector = ToSector( pos );
	chit->GetItem()->SetSignificant(GetNewsHistory(), ToWorld2F(pos), NewsEvent::DENIZEN_CREATED, NewsEvent::DENIZEN_KILLED, 0);

	if (XenoAudio::Instance()) {
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &ToWorld3F(pos));
	}

	return chit;
}


Chit* LumosChitBag::NewWorkerChit( const Vector3F& pos, int team )
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "worker" );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( rootItem.ResourceName() ));
	chit->Add( new PathMoveComponent());
	chit->Add( new AIComponent());
	chit->GetSpatialComponent()->SetPosition( pos );
	AddItem( rootItem.Name(), chit, context->engine, team, 0 );
	chit->Add( new HealthComponent());
	//chit->Add( new DebugStateComponent());

	if (XenoAudio::Instance()) {
		XenoAudio::Instance()->PlayVariation(ISC::rezWAV, random.Rand(), &pos);
	}
	return chit;
}


Chit* LumosChitBag::NewVisitor( int visitorIndex )
{
	const ChitContext* context = Context();
	Chit* chit = NewChit();
	const GameItem& rootItem = ItemDefDB::Instance()->Get( "visitor" );

	chit->Add( new SpatialComponent());
	chit->Add( new RenderComponent( rootItem.ResourceName() ));

	AIComponent* ai = new AIComponent();
	chit->Add( ai );
	GLASSERT( visitorIndex >= 0 && visitorIndex < Visitors::NUM_VISITORS );
	ai->SetVisitorIndex( visitorIndex );
	Visitors::Instance()->visitorData[visitorIndex].Connect();	// initialize.

	// Visitors start at world center, with gridMove, and go from there.
	Vector3F pos = { (float)context->worldMap->Width()*0.5f, 0.0f, (float)context->worldMap->Height()*0.5f };
	chit->GetSpatialComponent()->SetPosition( pos );

	PathMoveComponent* pmc = new PathMoveComponent();
	chit->Add(pmc);
	GridMoveComponent* gmc = new GridMoveComponent();
	chit->Add( gmc );

	SectorPort sp = context->worldMap->RandomPort( &random );
	gmc->SetDest( sp );

	AddItem( rootItem.Name(), chit, context->engine, TEAM_VISITOR, 0 );
	chit->Add( new HealthComponent());
	chit->Add( new VisitorStateComponent());
	return chit;
}


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
		if (array[i]->GetSpatialComponent()->Bounds().Contains(pos2i)) {
			found = array[i];
			break;
		}
	}
	return found;
}


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


Chit* LumosChitBag::QueryPorch( const grinliz::Vector2I& pos, int *type )
{
	if (type) *type = 0;

	Vector2I sector = ToSector(pos);
	for( MapSpatialComponent* it = mapSpatialHash[SectorIndex(sector)]; it; it = it->nextBuilding ) {
		if ( it->PorchPos().Contains( pos )) {
			if (type) *type = it->PorchType();
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

	ItemNameFilter goldFilter( ISC::gold, IChitAccept::MOB );
	this->QuerySpatialHash( &chitList, v2, 1.0f, 0, &goldFilter );
	Chit* chit = 0;

	// Evil bug this: adding gold to a wallet just before
	// deletion. I'm a little concerned where else
	// this could be a problem. Would be nice to make
	// deletion immediate.
	for (int i = 0; i < chitList.Size(); ++i) {
		if (chitList[i] && !IsQueuedForDelete(chitList[i])) {
			chit = chitList[i];
			break;
		}
	}

	if ( !chit ) {
		chit = this->NewChit();
		chit->Add( new SpatialComponent());
		AddItem( "gold", chit, context->engine, 0, 0 );
		chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
		chit->GetSpatialComponent()->SetPosition( pos );
		chit->Add(new GameMoveComponent());
	}
	chit->GetWallet()->Deposit(src, src->Gold());
	return chit;
}


Chit* LumosChitBag::NewCrystalChit( const grinliz::Vector3F& pos, Wallet* src, bool fuzz )
{
	Vector2F v2 = { pos.x, pos.z };
	if ( fuzz ) {
		v2.x = floorf(pos.x) + random.Uniform();
		v2.y = floorf(pos.y) + random.Uniform();
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
	chit->Add( new SpatialComponent());
	AddItem( name, chit, context->engine, 0, 0 );
	chit->Add( new RenderComponent( chit->GetItem()->ResourceName() ));
	chit->Add(new GameMoveComponent());

	chit->GetSpatialComponent()->SetPosition( pos );
	
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
	chit->Add( new SpatialComponent());
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
	chit->GetSpatialComponent()->SetPosition( pos );
	chit->Add( new HealthComponent());

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


GameItem* LumosChitBag::AddItem(const char* name, Chit* chit, Engine* engine, int team, int level, const char* altRes)
{
	ItemDefDB* itemDefDB = ItemDefDB::Instance();
	ItemDefDB::GameItemArr itemDefArr;
	itemDefDB->Get(name, &itemDefArr);
	GLASSERT(itemDefArr.Size() > 0);

	GameItem* item = itemDefArr[0]->Clone();
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


int LumosChitBag::MapGridUse( int x, int y )
{
	GLASSERT( MAX_BUILDING_SIZE == 2 );
	// An object is either at the center or influence by
	// something with 0.5 + EPS radius.

	static const float OFFSET = 0.6f;

	Rectangle2F r;
	r.min.x = (float)x + 0.5f - OFFSET;
	r.min.y = (float)y + 0.5f - OFFSET;
	r.max.x = (float)x + 0.5f + OFFSET;
	r.max.y = (float)y + 0.5f + OFFSET;

	ChitHasMapSpatial hasMapSpatial;
	QuerySpatialHash( &inUseArr, r, 0, &hasMapSpatial );
	int flags = 0;
	for( int i=0; i<inUseArr.Size(); ++i ) {
		MapSpatialComponent* ms = GET_SUB_COMPONENT( inUseArr[i], SpatialComponent, MapSpatialComponent );
		GLASSERT( ms );
		if ( ms->Bounds().Contains( x, y )) {
			flags |= ms->Mode();
		}
	}
	return flags;
}


CoreScript* LumosChitBag::GetHomeCore()	const
{ 
	if (homeTeam) {
		return CoreScript::GetCoreFromTeam(homeTeam);
	}
	return 0;
}


Vector2I LumosChitBag::GetHomeSector()	const
{ 
	grinliz::Vector2I v = { 0, 0 };
	CoreScript* home = GetHomeCore();
	if (home && home->ParentChit() && home->ParentChit()->GetSpatialComponent()) {
		v = home->ParentChit()->GetSpatialComponent()->GetSector();
	}
	return v;
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
	type = MAP | MOB;
}



ItemNameFilter::ItemNameFilter( const grinliz::IString& istring, int t )
{
	names = &istring;
	count  = 1;
	type = t;
	GLASSERT(t == MAP || t == MOB || t == (MAP | MOB));
}


ItemNameFilter::ItemNameFilter( const grinliz::IString* arr, int n, int t )
{
	names = arr;
	count  = n;
	type = t;
	GLASSERT(t == MAP || t == MOB || t == (MAP | MOB));
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
	type = MOB;
}


bool BuildingFilter::Accept( Chit* chit ) 
{
	// Assumed to be MapSpatial with "building" flagged on.
	MapSpatialComponent* msc = GET_SUB_COMPONENT( chit, SpatialComponent, MapSpatialComponent );
	return msc != 0;
}


bool BuildingRepairFilter::Accept(Chit* chit)
{
	// Assumed to be MapSpatial with "building" flagged on.
	MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
	if (msc ) { //&& msc->Building()) {
		GameItem* item = chit->GetItem();
		if (item && item->HPFraction() < 0.9f) {
			return true;
		}
	}
	return false;
}


void RelationshipFilter::CheckRelationship(Chit* _compareTo, int _relationship)
{
	team = _compareTo->Team();
	relationship = _relationship;
}


void RelationshipFilter::CheckRelationship(int _team, int _relationship)
{
	team = _team;
	relationship = _relationship;
}


bool RelationshipFilter::Accept( Chit* chit )
{
	if (team < 0) return true;	// not checking.
	return Team::GetRelationship(team, chit->Team()) == relationship;
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


void LumosChitBag::AddSummoning(const grinliz::Vector2I& sector, int reason)
{
	summoningArr.Push(sector);
}


grinliz::Vector2I LumosChitBag::HasSummoning(int reason)
{
	Vector2I v = { 0, 0 };
	if (summoningArr.Size()) {
		v = summoningArr[0];
	}
	return v;
}


void LumosChitBag::RemoveSummoning(const grinliz::Vector2I& sector)
{
	int i = 0;
	while ((i = summoningArr.Find(sector)) >= 0) {
		summoningArr.Remove(i);
	}
}


IString LumosChitBag::NameGen(const char* dataset, int seed, int min, int max)
{
	const gamedb::Reader* database = Context()->game->GetDatabase();
	const gamedb::Item* parent = database->Root()->Child("markovName");
	GLASSERT(parent);
	const gamedb::Item* item = parent->Child(dataset);
	GLASSERT(item);
	if (!item) return IString();
	const gamedb::Item* names = item->Child("names");

	if (names) {
		IString ds = StringPool::Intern(dataset);
		bool found = false;
		int id = 0;
		for (int i = 0; i < namePool.Size(); ++i) {
			if (namePool[i].dataset == ds) {
				namePool[i].id++;
				id = namePool[i].id;
				found = true;
			}
		}
		if (!found) {
			NamePoolID entry = { ds, 0 };
			id = 0;
			namePool.Push(entry);
		}
		seed = id;
	}
	return StaticNameGen(database, dataset, seed, min, max);
}


IString LumosChitBag::StaticNameGen(const gamedb::Reader* database, const char* dataset, int _seed, int min, int max)
{
	const gamedb::Item* parent = database->Root()->Child("markovName");
	GLASSERT(parent);
	const gamedb::Item* item = parent->Child(dataset);
	GLASSERT(item);
	if (!item) return IString();

	GLString nameBuffer = "";

	// Make sure the name generator is warm:
	Random random(_seed);
	random.Rand();
	random.Rand();
	int seed = random.Rand();

	const gamedb::Item* word = item->Child("words0");
	const gamedb::Item* names = item->Child("names");
	if (word) {
		// The 3-word form.
		for (int i = 0; i < 3; ++i) {
			static const char* CHILD[] = { "words0", "words1", "words2" };
			word = item->Child(CHILD[i]);
			if (word && word->NumAttributes()) {
				// attribute name and value are the same.
				const char *attr = word->AttributeName(random.Rand(word->NumAttributes()));
				if (i) {
					nameBuffer.AppendFormat(" %s", attr);
				}
				else {
					nameBuffer = attr;
				}
			}
		}
		return StringPool::Intern(nameBuffer.c_str());
	}
	else if (names) {
		int index = abs(_seed) % names->NumChildren();
		const char* n = names->ChildAt(index)->GetString("name");
		return StringPool::Intern(n);
	}
	else {
		// The triplet (letter) form.
		int size = 0;
		const void* data = database->AccessData(item, "triplets", &size);
		MarkovGenerator gen((const char*)data, size, seed);

		int len = max + 1;
		int error = 100;
		while (error--) {
			if (gen.Name(&nameBuffer, max) && (int)nameBuffer.size() >= min) {
				return StringPool::Intern(nameBuffer.c_str());
			}
		}
		gen.Name(&nameBuffer, max);
		return StringPool::Intern(nameBuffer.c_str());
	}
	return IString();
}


void LumosChitBag::NamePoolID::Serialize(XStream* xs)
{
	XarcOpen(xs, "NamePoolID");
	XARC_SER(xs, this->dataset);
	XARC_SER(xs, this->id);
	XarcClose(xs);
}
