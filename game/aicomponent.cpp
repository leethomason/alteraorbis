/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "aicomponent.h"
#include "worldmap.h"
#include "pathmovecomponent.h"
#include "gameitem.h"
#include "lumoschitbag.h"
#include "mapspatialcomponent.h"
#include "gridmovecomponent.h"
#include "workqueue.h"
#include "circuitsim.h"
#include "reservebank.h"
#include "sim.h"
#include "physicssims.h"
#include "lumosgame.h"

#include "../scenes/characterscene.h"
#include "../scenes/forgescene.h"

#include "../script/battlemechanics.h"
#include "../script/plantscript.h"
#include "../script/worldgen.h"
#include "../script/corescript.h"
#include "../script/itemscript.h"
#include "../script/buildscript.h"

#include "../engine/particle.h"

#include "../audio/xenoaudio.h"

#include "../xegame/rendercomponent.h"
#include "../xegame/itemcomponent.h"
#include "../xegame/cameracomponent.h"

#include "../Shiny/include/Shiny.h"

#include "../ai/marketai.h"
#include "../ai/domainai.h"
#include "../ai/director.h"

using namespace grinliz;
using namespace ai;

static const float	NORMAL_AWARENESS			= 10.0f;
static const float	LOOSE_AWARENESS = LONGEST_WEAPON_RANGE;
static const float	SHOOT_ANGLE_DOT				=  0.985f;	// same number, as dot product.
static const float	WANDER_RADIUS				=  5.0f;
static const int	SECTOR_HERD_ODDS			= 50;		// as in 1 in WANDER_ODDS
static const int	SECTOR_WANDER_ODDS			= 20;		// as in 1 in WANDER_ODDS
static const float	PLANT_AWARE					=  3;
static const float	GOLD_AWARE					=  5.0f;
static const float	FRUIT_AWARE					=  5.0f;
static const float  EAT_WILD_FRUIT				= 0.70f;
static const int	FORCE_COUNT_STUCK			=  8;
static const int	STAND_TIME_WHEN_WANDERING	= 1500;
static const int	RAMPAGE_THRESHOLD			= 10;		// how many times a destination must be blocked before rampage
static const int	GUARD_RANGE					= 1;
static const int	GUARD_TIME					= 10*1000;
static const double	NEED_CRITICAL				= 0.1;
static const int	REPAIR_TIME					= 4000;

const char* AIComponent::MODE_NAMES[int(AIMode::NUM_MODES)]     = { "normal", "battle" };
const char* AIComponent::ACTION_NAMES[int(AIAction::NUM_ACTIONS)] = { "none", "move", "melee", "shoot" };

Vector2I ToWG(int id) {
	Vector2I v = { 0, 0 };
	if (id < 0) {
		int i = -id;
		v.y   = i / MAX_MAP_SIZE;
		v.x   = i - v.y*MAX_MAP_SIZE;
	}
	return v;
}

inline int ToWG(const grinliz::Vector2I& v) {
	return -(v.y * MAX_MAP_SIZE + v.x);
}


template<ERelate INCLUDE0, ERelate INCLUDE1>
bool FEFilter(Chit* parentChit, int id) {
	if (!parentChit) return false;

	const ChitContext* context = parentChit->Context();
	if (id < 0) {
		Vector2I p = ToWG(id);
		const WorldGrid& wg = context->worldMap->GetWorldGrid(p);
		return wg.Plant() || wg.RockHeight();
	}
	Chit* chit = context->chitBag->GetChit(id);
	if (!chit) return false;

	float range2 = (chit->Position() - parentChit->Position()).LengthSquared();

	if ((chit != parentChit)
		&& (range2 < LOOSE_AWARENESS * LOOSE_AWARENESS)
		&& (ToSector(chit->Position()) == ToSector(parentChit->Position())))
	{
		ERelate relate = Team::Instance()->GetRelationship(chit, parentChit);
		if (relate == INCLUDE0 || relate == INCLUDE1)
			return true;
	}
	return false;
}


AIComponent::AIComponent() : feTicker( 750 ), needsTicker( 1000 )
{
	currentAction = AIAction::NO_ACTION;
	aiMode = AIMode::NORMAL_MODE;
	rampage = NO_RAMPAGE;
	focus = 0;
	wanderTime = 0;
	rethink = 0;
	fullSectorAware = false;
	visitorIndex = -1;
	destinationBlocked = 0;
	lastTargetID = 0;
	lastGrid.Zero();
}


AIComponent::~AIComponent()
{
}


void AIComponent::Serialize(XStream* xs)
{
	this->BeginSerialize(xs, Name());

	XARC_SER_ENUM(xs, AIMode, aiMode);
	XARC_SER_ENUM(xs, AIAction, currentAction);
	XARC_SER_DEF(xs, rampage, NO_RAMPAGE);
	XARC_SER(xs, lastTargetID);
	XARC_SER_DEF(xs, focus, 0);
	XARC_SER_DEF(xs, wanderTime, 0);
	XARC_SER(xs, rethink);
	XARC_SER_DEF(xs, fullSectorAware, false);
	XARC_SER_DEF(xs, visitorIndex, -1);
	XARC_SER_DEF(xs, destinationBlocked, 0);
	XARC_SER(xs, lastGrid);
	XARC_SER_VAL_CARRAY(xs, friendList2);
	XARC_SER_VAL_CARRAY(xs, enemyList2);
	feTicker.Serialize(xs, "feTicker");
	needsTicker.Serialize(xs, "needsTicker");
	needs.Serialize(xs);
	this->EndSerialize(xs);
}


bool AIComponent::Log()
{
	if (!parentChit) return false;
	CameraComponent* cc = Context()->chitBag->GetCamera();
	return Context()->game->AIDebugLog() && (cc->Tracking() == this->ParentChit()->ID());
}


void AIComponent::OnAdd(Chit* chit, bool init)
{
	super::OnAdd(chit, init);
	if (init) {
		feTicker.SetPeriod(750 + (chit->ID() & 128));
	}
	const ChitContext* context = Context();
	taskList.Init(parentChit, context);
}


void AIComponent::OnRemove()
{
	super::OnRemove();
}


bool AIComponent::LineOfSight(Chit* target, const RangedWeapon* weapon)
{
	Vector3F origin, dest;
	GLASSERT(weapon);

	RenderComponent* thisRender = parentChit->GetRenderComponent();
	if (!thisRender) return false;

	thisRender->GetMetaData(HARDPOINT_TRIGGER, &origin);
	dest = EnemyPos(target->ID(), true);

	Vector3F dir = dest - origin;
	float length = dir.Length() + 0.01f;	// a little extra just in case
	CArray<const Model*, RenderComponent::NUM_MODELS + 1> ignore, targetModels;
	thisRender->GetModelList(&ignore);

	const ChitContext* context = Context();
	// FIXME: was TEST_TRI, which is less accurate. But also fundamentally incorrect, since
	// the TEST_TRI doesn't account for bone xforms. Deep bug - but surprisingly hard to see
	// in the game. Switched to the faster TEST_HIT_AABB, but it would be nice to clean
	// up TEST_TRI and just make it fast.
	ModelVoxel mv = context->engine->IntersectModelVoxel(origin, dir, length, TEST_HIT_AABB, 0, 0, ignore.Mem());
	if (mv.model) {
		RenderComponent* targetRender = target->GetRenderComponent();
		if (targetRender) {
			targetRender->GetModelList(&targetModels);
			return targetModels.Find(mv.model) >= 0;
		}
	}
	return false;
}


bool AIComponent::LineOfSight(const grinliz::Vector2I& mapPos )
{
	RenderComponent* thisRender = ParentChit()->GetRenderComponent();
	GLASSERT(thisRender);
	if (!thisRender) return false;

	Vector3F origin;
	thisRender->CalcTrigger(&origin, 0);

	CArray<const Model*, RenderComponent::NUM_MODELS+1> ignore;
	thisRender->GetModelList( &ignore );

	Vector3F dest = { (float)mapPos.x+0.5f, 0.5f, (float)mapPos.y+0.5f };
	Vector3F dir = dest - origin;
	float length = dir.Length() + 0.01f;	// a little extra just in case

	const ChitContext* context = Context();
	ModelVoxel mv = context->engine->IntersectModelVoxel( origin, dir, length, TEST_TRI, 0, 0, ignore.Mem() );

	// A little tricky; we hit the 'mapPos' if nothing is hit (which gets to the center)
	// or if voxel at that pos is hit.
    return !mv.Hit() || ( mv.Hit() && mv.Voxel2() == mapPos );
}


void AIComponent::MakeAware(const int* enemyIDs, int n)
{
	for (int i = 0; i < n && enemyList2.HasCap(); ++i) {
		int id = enemyIDs[i];
		// Be careful to not duplicate list entries:
		if (enemyList2.Find(id) >= 0) continue;

		Chit* chit = Context()->chitBag->GetChit(id);
		if (FEFilter<ERelate::ENEMY, ERelate::NEUTRAL>(chit, 0)) {
			enemyList2.Push(id);
		}
	}
}


Vector3F AIComponent::EnemyPos(int id, bool target)
{
	Vector3F pos = { 0, 0, 0 };
	if (id > 0) {
		Chit* chit = Context()->chitBag->GetChit(id);
		if (chit) {
			if (target) {
				RenderComponent* rc = chit->GetRenderComponent();
				GLASSERT(rc);
				if (rc) {
					// The GetMetaData is expensive, because
					// of the animation xform. Cheat a bit;
					// use the base xform.
					const ModelMetaData* metaData = rc->MainResource()->GetMetaData(META_TARGET);
					pos = rc->MainModel()->XForm() * metaData->pos;
					return pos;
				}
			}
			else {
				return chit->Position();
			}
		}
	}
	else if (id < 0) {
		Vector2I v2i = ToWG(id);
		if (target)
			pos.Set((float)v2i.x + 0.5f, 0.5f, (float)v2i.y + 0.5f);
		else
			pos.Set((float)v2i.x + 0.5f, 0.0f, (float)v2i.y + 0.5f);
		return pos;
	}
	return pos;
}


void AIComponent::ProcessFriendEnemyLists(bool tick)
{
	Vector2F center = ToWorld2F(parentChit->Position());

	// Clean the lists we have.
	enemyList2.Filter(parentChit, [](Chit* parentChit, int id) {
		return FEFilter<ERelate::ENEMY, ERelate::NEUTRAL>(parentChit, id);
	});

	friendList2.Filter(parentChit, [](Chit* parentChit, int id) {
		return FEFilter<ERelate::FRIEND, ERelate::FRIEND>(parentChit, id);
	});

	// Did we lose our focused target?
	if (focus == FOCUS_TARGET) {
		if (enemyList2.Empty() || (focus != enemyList2[0])) {
			focus = FOCUS_NONE;
			if (Log()) {
				GLOUTPUT(("AI ID=%d FOCUS_TARGET lost.\n", parentChit->ID()));
			}
		}
	}

	if (tick) {
		// Compute the area for the query.
		Rectangle2I zone;
		zone.min = zone.max = ToWorld2I(parentChit->Position());
		zone.Outset(fullSectorAware ? SECTOR_SIZE : int(NORMAL_AWARENESS));

		if (Context()->worldMap->UsingSectors()) {
			zone.DoIntersection(InnerSectorBounds(ToSector(center.x, center.y)));
		}
		else {
			zone.DoIntersection(Context()->worldMap->Bounds());
		}

		friendList2.Clear();
		int saveTarget = enemyList2.Empty() ? 0 : enemyList2[0];
		enemyList2.Clear();

		MOBIshFilter mobFilter;
		ItemNameFilter coreFilter(ISC::core);
		BuildingFilter buildingFilter;

		// Order matters: prioritize mobs, then a core, then buildings.
		static const int NFILTER = 3;
		CChitArray arr[NFILTER];
		IChitAccept* filters[NFILTER] = {&mobFilter, &coreFilter, &buildingFilter};

		ChitAcceptAll all;
		Context()->chitBag->QuerySpatialHash(&chitArr, zone, parentChit, &all);

		// Don't attack buildings if there isn't a central core.
		CoreScript* cs = CoreScript::GetCore(ToSector(center));
		bool inUse = (cs && cs->InUse());

		// Add extra friends or enemies into the list.
		for (int i = 0; i < chitArr.Size(); ++i) {
			Chit* chit = chitArr[i];
			GLASSERT(chit);
			if (!chit) continue;
			const GameItem* item = chit->GetItem();
			if (!item) continue;
			if (item->flags & GameItem::AI_NOT_TARGET) continue;

			// REMEMBER: because I've made this mistake 20 times, the
			//			 path to a building will always fail, because the building
			//			 is blocking the path. So the return value of 'path' needs
			//			 to keep that in mind.

			// REMEMBER: if we try to path to something we can't get to, that
			//			 confuses the AI. Particular critical if we are chasing
			//			 enemies.

			bool path = fullSectorAware
				|| Context()->worldMap->HasStraightPath(center, ToWorld2F(chit->Position()), true);
						
			if (mobFilter.Accept(chit)) {
				if (path && FEFilter<ERelate::ENEMY, ERelate::ENEMY>(parentChit, chit->ID()))
					arr[0].PushIfCap(chit);
				else if (FEFilter<ERelate::FRIEND, ERelate::FRIEND>(parentChit, chit->ID()))
					friendList2.PushIfCap(chit->ID());
			}
			else if (path && coreFilter.Accept(chit) && FEFilter<ERelate::ENEMY, ERelate::ENEMY>(parentChit, chit->ID())) {
				arr[1].PushIfCap(chit);
			}
			else if (   inUse 
					 && buildingFilter.Accept(chit) 
					 && FEFilter<ERelate::ENEMY, ERelate::ENEMY>(parentChit, chit->ID()))
			{
				MapSpatialComponent* msc = GET_SUB_COMPONENT(chit, SpatialComponent, MapSpatialComponent);
				GLASSERT(msc);
				Rectangle2I buildingBounds = msc->Bounds();
				if (Context()->worldMap->HasStraightPathBeside(center, buildingBounds)) {
					arr[2].PushIfCap(chit);
				}
			}
		}

		Chit* saveTargetChit = Context()->chitBag->GetChit(saveTarget);
		if (saveTargetChit && !saveTargetChit->GetItem()->IsVisitor()) {
			for (int k = 0; k < NFILTER; ++k) {
				// Move the save target to the front of the appropriate array.
				int idx = arr[k].Find(saveTargetChit);
				if (idx >= 0) {
					Swap(&arr[k][0], &arr[k][idx]);
					break;
				}
				else if (filters[k]->Accept(saveTargetChit)) {
					if (arr[k].HasCap()) {
						arr[k].Insert(0, saveTargetChit);
					}
					break;
				}
			}
		}

		for (int k = 0; k < NFILTER; ++k) {
			for (int i = 0; i < arr[k].Size(); ++i) {
				enemyList2.PushIfCap(arr[k][i]->ID());
			}
		}
		if (saveTarget < 0 && enemyList2.Empty()) {
			enemyList2.Push(saveTarget);
		}
	}
}


class ChitDistanceCompare
{
public:
	ChitDistanceCompare( const Vector3F& _origin ) : origin(_origin) {}
	bool Less( Chit* v0, Chit* v1 ) const
	{
#if 0
		// This has a nasty rounding bug
		// where 2 "close" things would always evaluate "less than" and
		// the sort flips endlessly.
		return ( v0->GetPosition() - origin ).LengthSquared() <
			   ( v1->GetPosition() - origin ).LengthSquared();
#endif

		Vector3F p0 = v0->Position() - origin;
		Vector3F p1 = v1->Position() - origin;
		float len0 = p0.LengthSquared();
		float len1 = p1.LengthSquared();
		return len0 < len1;
	}

private:
	Vector3F origin;
};


void AIComponent::DoMove()
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC || !pmc || pmc->ForceCountHigh() || pmc->Stopped()) {
		currentAction = AIAction::NO_ACTION;
		if (Log()) {
			GLOUTPUT(("AI ID=%d Move stopped. PMC=%s forceCount=%d\n", parentChit->ID(), pmc ? "true" : "fals", pmc ? pmc->ForceCount() : 0));
		}
		return;
	}

	// Generally speaking, moving is done by the PathMoveComponent. When
	// not in battle, this is essentially "do nothing." If in battle mode,
	// we look for opportunity fire and such.
	if (aiMode != AIMode::BATTLE_MODE) {
		if ((rampage != NO_RAMPAGE) && RampageDone()) {
			if (Log()) {
				GLOUTPUT(("AI ID=%d Rampage Done\n", parentChit->ID()));
			}
			rampage = NO_RAMPAGE;
			aiMode = AIMode::NORMAL_MODE;
			currentAction = AIAction::NO_ACTION;
			return;
		}
		// Reloading is always a good background task.
		RangedWeapon* rangedWeapon = thisIC->GetRangedWeapon(0);
		if (rangedWeapon && rangedWeapon->CanReload()) {
			rangedWeapon->Reload(parentChit);
		}
	}
	else {
		// Battle mode! Run & Gun
		float utilityRunAndGun = 0.0f;
		Chit* targetRunAndGun = 0;

		RangedWeapon* rangedWeapon = thisIC->GetRangedWeapon(0);
		if (rangedWeapon) {
			Vector3F heading = parentChit->Heading();
			bool explosive = (rangedWeapon->flags & GameItem::EFFECT_EXPLOSIVE) != 0;

			if (rangedWeapon->CanShoot()) {
				float radAt1 = BattleMechanics::ComputeRadAt1(parentChit->GetItem(),
															  rangedWeapon,
															  true,
															  true);	// Doesn't matter to utility.

				for (int k = 0; k < enemyList2.Size(); ++k) {
					Chit* enemy = Context()->chitBag->GetChit(enemyList2[k]);
					if (!enemy) {
						continue;
					}
					Vector3F p0;
					Vector3F p1 = BattleMechanics::ComputeLeadingShot(parentChit, enemy, rangedWeapon->BoltSpeed(), &p0);
					Vector3F normal = p1 - p0;
					normal.Normalize();
					float range = (p0 - p1).Length();

					if (explosive && range < EXPLOSIVE_RANGE*3.0f) {
						// Don't run & gun explosives if too close.
						continue;
					}

					if (DotProduct(normal, heading) >= SHOOT_ANGLE_DOT) {
						// Wow - can take the shot!
						float u = BattleMechanics::ChanceToHit(range, radAt1);

						if (u > utilityRunAndGun) {
							utilityRunAndGun = u;
							targetRunAndGun = enemy;
						}
					}
				}
			}
			float utilityReload = 0.0f;
			if (rangedWeapon->CanReload()) {
				utilityReload = 1.0f - rangedWeapon->RoundsFraction();
			}
			if (utilityReload > 0 || utilityRunAndGun > 0) {
				if (Log()) {
					GLOUTPUT(("AI ID=%d Move: RunAndGun=%.2f Reload=%.2f ", parentChit->ID(), utilityRunAndGun, utilityReload));
				}
				if (utilityRunAndGun > utilityReload) {
					GLASSERT(targetRunAndGun);
					Vector3F leading = BattleMechanics::ComputeLeadingShot(parentChit, targetRunAndGun, rangedWeapon->BoltSpeed(), 0);
					BattleMechanics::Shoot(Context()->chitBag,
										   parentChit,
										   leading,
										   targetRunAndGun->GetMoveComponent() ? targetRunAndGun->GetMoveComponent()->IsMoving() : false,
										   rangedWeapon);
					if (Log()) {
						GLOUTPUT(("->RunAndGun\n"));
					}
				}
				else {
					rangedWeapon->Reload(parentChit);
					if (Log()) {
						GLOUTPUT(("->Reload\n"));
					}
				}
			}
		}
	}
}


void AIComponent::DoShoot()
{
	Vector3F leading = { 0, 0, 0 };
	bool isMoving = false;

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return;

	RangedWeapon* weapon = thisIC->GetRangedWeapon(0);
	// FIXME: real bug here, although maybe minor. serialization in 
	// ItemComponent() calls UseBestItems(), which can re-order weapons,
	// which (I think) causes the ranged weapon to be !current.
	// It will eventually reset. But annoying and may be occasionally visible.
	if (!weapon) {
		thisIC->SelectWeapon(ItemComponent::SELECT_RANGED);
		weapon = thisIC->GetRangedWeapon(0);
	}
	GLASSERT(weapon);
	if (!weapon) return;
	if (enemyList2.Empty()) return;	// no target

	int targetID = enemyList2[0];
	GLASSERT(targetID != 0);
	if (targetID == 0) return;

	if (targetID > 0) {
		Chit* targetChit = Context()->chitBag->GetChit(targetID);
		GLASSERT(targetChit);
		if (!targetChit) return;

		leading = BattleMechanics::ComputeLeadingShot(parentChit, targetChit, weapon->BoltSpeed(), 0);
		isMoving = targetChit->GetMoveComponent() ? targetChit->GetMoveComponent()->IsMoving() : false;
	}
	else {
		Vector2I p = ToWG(targetID);
		leading.Set((float)p.x + 0.5f, 0.5f, (float)p.y + 0.5f);
	}

	Vector2F leading2D = { leading.x, leading.z };
	// Rotate to target.
	Vector2F heading = parentChit->Heading2D();
	Vector2F normalToTarget = leading2D - ToWorld2F(parentChit->Position());
	normalToTarget.Normalize();
	float dot = DotProduct(heading, normalToTarget);

	if (dot >= SHOOT_ANGLE_DOT) {
		// all good.
	}
	else {
		// Rotate to target.
		PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
		if (pmc) {
			//float angle = RotationXZDegrees( normalToTarget.x, normalToTarget.y );
			pmc->QueueDest(ToWorld2F(parentChit->Position()), &normalToTarget);
		}
		return;
	}

	if (weapon) {
		if (weapon->HasRound()) {
			// Has round. May be in cooldown.
			if (weapon->CanShoot()) {
				if (Log()) {
					GLOUTPUT(("AI ID=%d shoot rounds=%d\n", parentChit->ID(), weapon->Rounds()));
				}
				BattleMechanics::Shoot(Context()->chitBag,
									   parentChit,
									   leading,
									   isMoving,
									   weapon);
			}
		}
		else {
			weapon->Reload(parentChit);
			// Out of ammo - do something else.
			currentAction = AIAction::NO_ACTION;
		}
	}
}


void AIComponent::DoMelee()
{
	if (enemyList2.Empty()) return;
	int targetID = enemyList2[0];

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return;

	MeleeWeapon* weapon = thisIC->GetMeleeWeapon();
	Chit* targetChit = Context()->chitBag->GetChit(targetID);
	Vector2I mapPos = ToWG(targetID);

	const ChitContext* context = Context();
	if ( !weapon ) {
		currentAction = AIAction::NO_ACTION;
		return;
	}
	PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );

	// Are we close enough to hit? Then swing. Else move to target.
	if ( targetChit && BattleMechanics::InMeleeZone( context->engine, parentChit, targetChit )) {
		GLASSERT( parentChit->GetRenderComponent()->AnimationReady() );
		parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
		IString sound = weapon->keyValues.GetIString(ISC::sound);
		if (!sound.empty() && XenoAudio::Instance()) {

			XenoAudio::Instance()->PlayVariation(sound, weapon->ID(), &parentChit->Position());
		}

		Vector2F pos2 = ToWorld2F(parentChit->Position());
		Vector2F heading = ToWorld2F(targetChit->Position()) - pos2;
		heading.Normalize();

		float angle = RotationXZDegrees(heading.x, heading.y);
		// The PMC seems like a good idea...BUT the PMC sends back destination
		// reached messages. Which is a good thing, but causes the logic
		// to reset. Go for the expedient solution: insta-turn for melee.
		//if ( pmc ) pmc->QueueDest( pos2, &heading );
		parentChit->SetRotation(Quaternion::MakeYRotation(angle));
		if (pmc) pmc->Stop();
	}
	else if ( targetID < 0 && BattleMechanics::InMeleeZone( context->engine, parentChit, mapPos )) {
		GLASSERT( parentChit->GetRenderComponent()->AnimationReady() );
		parentChit->GetRenderComponent()->PlayAnimation( ANIM_MELEE );
		IString sound = weapon->keyValues.GetIString(ISC::sound);
		if (!sound.empty() && XenoAudio::Instance()) {
			XenoAudio::Instance()->PlayVariation(sound, weapon->ID(), &parentChit->Position());
		}

		Vector2F pos2 = ToWorld2F(parentChit->Position());
		Vector2F heading = ToWorld2F( mapPos ) - pos2;
		heading.Normalize();

		float angle = RotationXZDegrees(heading.x, heading.y);
		// The PMC seems like a good idea...BUT the PMC sends back destination
		// reached messages. Which is a good thing, but causes the logic
		// to reset. Go for the expedient solution: insta-turn for melee.
		//if ( pmc ) pmc->QueueDest( pos2, &heading );
		parentChit->SetRotation(Quaternion::MakeYRotation(angle));
		if ( pmc ) pmc->Stop();
	}
	else {
		// Move to target.
		if ( pmc ) {
			Vector2F targetPos = ToWorld2F(EnemyPos(targetID, false));

			Vector2F pos = ToWorld2F(parentChit->Position());
			Vector2F dest = { -1, -1 };
			pmc->QueuedDest( &dest );

			float delta = ( dest-targetPos ).Length();
			float distanceToTarget = (targetPos - pos).Length();

			// If the error is greater than distance to, then re-path.
			if ( delta > distanceToTarget * 0.25f ) {
				pmc->QueueDest( targetPos );
			}
		}
		else {
			currentAction = AIAction::NO_ACTION;
			return;
		}
	}
}


VisitorData* AIComponent::GetVisitorData()
{
	if (visitorIndex >= 0)  {
		VisitorData* vd = &Visitors::Instance()->visitorData[visitorIndex];
		return vd;
	}
	return 0;
}


void AIComponent::OnChitEvent( const ChitEvent& event )
{
	super::OnChitEvent( event );
}


void AIComponent::Think()
{
	if (visitorIndex >= 0)					ThinkVisitor();
	else if (aiMode == AIMode::BATTLE_MODE)	ThinkBattle();
	else if (rampage != NO_RAMPAGE)			ThinkRampage();
	else									ThinkNormal();
}


bool AIComponent::TravelHome(bool focus)
{
	CoreScript* cs = CoreScript::GetCoreFromTeam(parentChit->Team());
	if (!cs) return false;

	Vector2F dst = ToWorld2F(cs->ParentChit()->Position());
	Vector2F src = ToWorld2F(parentChit->Position());

	SectorPort dstSP = Context()->worldMap->NearestPort(dst, &src);
	return Move(dstSP, focus);
}


bool AIComponent::Move(const SectorPort& sp, bool focused)
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
	const ChitContext* context = Context();

	if (pmc && sp.IsValid()) {
		// Read our local get-on-the-grid info. Bias to the destination.
		SectorPort local = context->worldMap->NearestPort(ToWorld2F(parentChit->Position()), 0);
		// Completely possible this chit can't actually path anywhere.
		if (local.IsValid()) {
			const SectorData& localSD = context->worldMap->GetSectorData(local.sector);
			// Local path to remote dst
			Vector2F dest2 = SectorData::PortPos(localSD.GetPortLoc(local.port), parentChit->ID());
			pmc->QueueDest(dest2, 0, &sp);
			currentAction = AIAction::MOVE;
			focus = focused ? FOCUS_MOVE : 0;
			rethink = 0;
			return true;
		}
	}
	return false;
}


void AIComponent::Pickup(Chit* item)
{
	taskList.Push(Task::MoveTask(ToWorld2F(item->Position())));
	taskList.Push(Task::PickupTask(item->ID()));
}


void AIComponent::Move( const grinliz::Vector2F& dest, bool focused, const Vector2F* normal )
{
	// A focus move removes a focused world target. This keeps the 
	// unit from obsessing over a world target, no matter where
	// it is moved. There seems to be a bug where the target
	// can be multiply entered. *sigh* Troubling architecture
	// for the enemy list.
	if (focused && enemyList2.Size() && enemyList2[0] < 0) {
		enemyList2.Filter(0, [](int, int id) {
			return id > 0;
		});
	}

	PathMoveComponent* pmc    = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
	if ( pmc ) {
		pmc->QueueDest( dest, normal );
		currentAction = AIAction::MOVE;
		focus = focused ? FOCUS_MOVE : 0;
		rethink = 0;
	}
}


bool AIComponent::TargetAdjacent(const grinliz::Vector2I& pos, bool focused)
{
	static const Vector2I DELTA[4] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
	for (int i = 0; i < 4; ++i) {
		Vector2I v = pos + DELTA[i];
		const WorldGrid& wg = Context()->worldMap->GetWorldGrid(v);
		if (wg.BlockingPlant() || wg.RockHeight()) {
			Target(v, focused);
			return true;
		}
	}
	return false;
}


void AIComponent::Target(Chit* chit, bool focused) 
{
	Target(chit->ID(), focused);
}


void AIComponent::Target(const Vector2I& pos, bool focused) 
{
	Target(ToWG(pos), focused);
}


void AIComponent::Target(int id, bool focused)
{
	if (aiMode != AIMode::BATTLE_MODE) {
		aiMode = AIMode::BATTLE_MODE;
		if (parentChit->GetRenderComponent()) {
			parentChit->GetRenderComponent()->AddDeco("attention", STD_DECO);
		}
	}
	int idx = enemyList2.Find(id);
	if (idx >= 0) {
		Swap(&enemyList2[idx], &enemyList2[0]);
	}
	else {
		enemyList2.Insert(0, id);
	}
	enemyList2.Insert(0, id);
	focus = focused ? FOCUS_TARGET : 0;
}


Chit* AIComponent::GetTarget()
{
	if (!enemyList2.Empty()) {
		return Context()->chitBag->GetChit(enemyList2[0]);
	}
	return 0;
}


WorkQueue* AIComponent::GetWorkQueue()
{
	Vector2I sector = ToSector(parentChit->Position());
	CoreScript* coreScript = CoreScript::GetCore(sector);
	if ( !coreScript )
		return 0;

	// Again, bitten: workers aren't citizens. Grr.
	if ((parentChit->GetItem()->IsWorker())
		|| coreScript->IsCitizen(parentChit->ID()))
	{
		WorkQueue* workQueue = coreScript->GetWorkQueue();
		return workQueue;
	}
	return 0;
}


void AIComponent::Rampage(int dest)
{
	rampage = dest;
	aiMode = AIMode::NORMAL_MODE;
	currentAction = AIAction::NO_ACTION;

	ChitBag::CurrentNews news = { StringPool::Intern("Rampage"), ToWorld2F(parentChit->Position()), parentChit->ID() };
	Context()->chitBag->PushCurrentNews(news);

	if (Log()) {
		GLOUTPUT(("AI ID=%d Rampage to %d\n", parentChit->ID(), dest));
	}
}


// Function to compute "should we rampage." Rampage logic is in ThinkRampage()
bool AIComponent::ThinkDoRampage()
{
	if (parentChit->PlayerControlled())
		return false;
	if (destinationBlocked < RAMPAGE_THRESHOLD)
		return false;

	Vector2I pos2i = ToWorld2I(parentChit->Position());
	Vector2I sector = ToSector(pos2i);
	const WorldGrid& wg = Context()->worldMap->GetWorldGrid(pos2i);

	const ChitContext* context = Context();

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return false;

	// Need a melee weapon to rampage. Ranged is never used..
	const MeleeWeapon* melee = thisIC->QuerySelectMelee();
	if (!melee)	return false;

	const GameItem* thisItem = parentChit->GetItem();
	if (!thisItem) return false;

	// Workers teleport. Rampaging was annoying.
	if (thisItem->IsWorker()) {
		CoreScript* cs = CoreScript::GetCore(ToSector(pos2i));
		if (cs) {
			Vector2I csPos2i = ToWorld2I(cs->ParentChit()->Position());
			if (csPos2i != pos2i) {
				SpatialComponent::Teleport(parentChit, ToWorld3F(csPos2i));
			}
			if (csPos2i == pos2i) {
				destinationBlocked = 0;
			}
		}
		// Done whether we can teleport or not.
		return true;
	}

	// Go for a rampage: remember, if the path is clear,
	// it's essentially just a random walk.
	destinationBlocked = 0;
	const SectorData& sd = context->worldMap->GetSectorData(ToSector(parentChit->Position()));

	CArray< int, 5 > targetArr;

	if (pos2i != sd.core) {
		targetArr.Push(0);	// always core.
	}
	if (!wg.IsPort()) {
		for (int i = 0; i < 4; ++i) {
			int port = 1 << i;
			if ((sd.ports & port) && !sd.GetPortLoc(port).Contains(pos2i)) {
				targetArr.Push(i + 1);	// push the port, if it has it.
			}
		}
	}
	GLASSERT(targetArr.Size());

	parentChit->random.ShuffleArray(targetArr.Mem(), targetArr.Size());
	this->Rampage(targetArr[0]);
	return true;
}


bool AIComponent::RampageDone()
{
	GLASSERT(rampage != NO_RAMPAGE);
	if (rampage == NO_RAMPAGE) return true;

	const ChitContext* context = Context();
	Vector2I pos2i = ToWorld2I(parentChit->Position());
	const SectorData& sd = context->worldMap->GetSectorData(ToSector(pos2i));
	Rectangle2I dest;

	switch (rampage) {
		case WorldGrid::PS_CORE:	dest.min = dest.max = sd.core;				break;
		case WorldGrid::PS_POS_X:	dest = sd.GetPortLoc(SectorData::POS_X);	break;
		case WorldGrid::PS_POS_Y:	dest = sd.GetPortLoc(SectorData::POS_Y);	break;
		case WorldGrid::PS_NEG_X:	dest = sd.GetPortLoc(SectorData::NEG_X);	break;
		case WorldGrid::PS_NEG_Y:	dest = sd.GetPortLoc(SectorData::NEG_Y);	break;
		default: GLASSERT(0);	break;
	}

	if (dest.Contains(pos2i)) {
		return true;
	}
	return Context()->worldMap->CalcPath(ToWorld2F(pos2i), ToWorld2F(dest.min), 0, 0, false);
}


void AIComponent::ThinkRampage()
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
	if (pmc && pmc->IsMoving())
		return;
	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return;

	// Where are we, and where to next?
	const ChitContext* context = Context();
	Vector2I pos2i = ToWorld2I(parentChit->Position());
	const WorldGrid& wg0 = context->worldMap->GetWorldGrid(pos2i.x, pos2i.y);
	Vector2I next = pos2i + wg0.Path(rampage);
	const WorldGrid& wg1 = context->worldMap->GetWorldGrid(next.x, next.y);

	if (RampageDone()) {
		rampage = NO_RAMPAGE;
		aiMode = AIMode::NORMAL_MODE;
		currentAction = AIAction::NO_ACTION;
		return;
	}

	Chit* building = Context()->chitBag->QueryBuilding(IString(), next, nullptr);
	if (building) {
		// Cancels rampage. Maybe not the best plan,
		// hard to know what to do. (Destroy own buildings??)
		rampage = NO_RAMPAGE;
		aiMode = AIMode::NORMAL_MODE;
		currentAction = AIAction::NO_ACTION;
	}
	else if (wg1.RockHeight() || wg1.BlockingPlant()) {
		this->Target(next, false);
		GLASSERT(thisIC->SelectWeapon(ItemComponent::SELECT_MELEE));
	}
	else if (wg1.IsLand()) {
		this->Move(ToWorld2F(next), false);
	}
	else {
		rampage = NO_RAMPAGE;
		aiMode = AIMode::NORMAL_MODE;
		currentAction = AIAction::NO_ACTION;
	}
}


Vector2F AIComponent::GetWanderOrigin()
{
	Vector2F pos = ToWorld2F(parentChit->Position());
	Vector2I sector = ToSector(pos);
	return ToWorld2F(SectorBounds(sector).Center());
}


Vector2F AIComponent::ThinkWanderCircle()
{
	// In a circle?
	// This turns out to be creepy. Worth keeping for something that is,
	// in fact, creepy.
	Vector2F dest = GetWanderOrigin();
	Random random(parentChit->ID());

	float angleUniform = random.Uniform();
	float lenUniform = 0.25f + 0.75f*random.Uniform();

	static const U32 PERIOD = 40 * 1000;
	U32 t = wanderTime % PERIOD;
	float timeUniform = (float)t / (float)PERIOD;

	angleUniform += timeUniform;
	float angle = angleUniform * 2.0f * PI;
	Vector2F v = { cosf(angle), sinf(angle) };

	v = v * (lenUniform * WANDER_RADIUS);

	dest = GetWanderOrigin() + v;
	return dest;
}


Vector2F AIComponent::ThinkWanderRandom()
{
	Vector2I pos2i = ToWorld2I(parentChit->Position());
	Vector2I sector = ToSector(pos2i);
	CoreScript* cs = CoreScript::GetCore(sector);
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return ToWorld2F(parentChit->Position());

	// See also the EnterGrid(), where it takes over a neutral core.
	// Workers stay close to home, as 
	// do denizens trying to find a new home core.
	if ((gameItem->IsWorker())
		|| (gameItem->MOB() == ISC::denizen && Team::IsRogue(parentChit->Team()) && cs && !cs->InUse()))
	{
		Vector2F dest = GetWanderOrigin();
		dest.x += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
		dest.y += parentChit->random.Uniform() * WANDER_RADIUS * parentChit->random.Sign();
		return dest;
	}

	Vector2F dest = { 0, 0 };
	dest.x = (float)(sector.x*SECTOR_SIZE + 1 + parentChit->random.Rand(SECTOR_SIZE - 2)) + 0.5f;
	dest.y = (float)(sector.y*SECTOR_SIZE + 1 + parentChit->random.Rand(SECTOR_SIZE - 2)) + 0.5f;
	return dest;
}


// Lots of things herd wander, including denizens.
Vector2F AIComponent::ThinkWanderHerd()
{
	Vector2F origin = ToWorld2F(parentChit->Position());
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return origin;

	static const int NPLANTS = 4;
	static const float TOO_CLOSE = 2.0f;
	// +1 for origin, +4 for plants
	CArray<Vector2F, MAX_TRACK*2> pos;
	GLASSERT(MAX_TRACK >= SQUAD_SIZE);

	int squadID = 0;
	bool addPlants = true;
	int nRandom = friendList2.Size() >= 4 ? 2 : 1;

	CoreScript* myCore = CoreScript::GetCoreFromTeam(parentChit->Team());
	if (myCore && myCore->IsSquaddieOnMission(parentChit->ID(), &squadID)) {
		// Friends are other squaddies.
		CChitArray arr;
		myCore->Squaddies(squadID, &arr);
		for (int i = 0; i < arr.Size(); ++i) {
			pos.PushIfCap(ToWorld2F(arr[i]->Position()));
		}
		addPlants = false;
		nRandom = 1;
	}
	else if (friendList2.Size()) {
		// Friends are in the friends list.
		for (int i = 0; i < friendList2.Size(); ++i) {
			Chit* c = parentChit->Context()->chitBag->GetChit(friendList2[i]);
			if (c) {
				PathMoveComponent* pmc = GET_SUB_COMPONENT(c, MoveComponent, PathMoveComponent);
				if (pmc && pmc->IsMoving())
					pos.PushIfCap(pmc->DestPos());
				else
					pos.PushIfCap(ToWorld2F(c->Position()));
			}
		}
	}

	for (int i = 0; i < nRandom; ++i) {
		Rectangle2I inner = InnerSectorBounds(ToSector(parentChit->Position()));
		inner.Outset(-2);
		Vector2I p = { inner.min.x + parentChit->random.Rand(inner.Width()), inner.min.y + parentChit->random.Rand(inner.Height()) };
		pos.PushIfCap(ToWorld2F(p));
	}

	// For a worker, add in the wander origin.
	if (gameItem->IsWorker()) {
		pos.PushIfCap(GetWanderOrigin());
	}

	if (Team::IsDenizen(gameItem->Team()) && Team::IsRogue(gameItem->Team())) {
		Vector2I sector = ToSector(parentChit->Position());
		CoreScript* coreScript = CoreScript::GetCore(sector);
		if (coreScript && !coreScript->InUse()) {
			const SectorData& sd = Context()->worldMap->GetSectorData(sector);
			Vector2F loc = ToWorld2F(sd.CoreLoc());
			int n = 1 + friendList2.Size() / 2;
			for (int i = 0; i < n; ++i) {
				pos.PushIfCap(loc);
			}
		}
	}

#if 0
	if (addPlants) {
		// And plants are friends. This was originally in place
		// because some MOBs healed at plants. No longer the
		// case. But interesting randomizer, so left in place.
		Rectangle2I r;
		r.min = r.max = ToWorld2I(origin);
		r.Outset(int(PLANT_AWARE));
		r.DoIntersection(Context()->worldMap->Bounds());

		int nPlants = 0;
		for (Rectangle2IIterator it(r); !it.Done() && nPlants < NPLANTS; it.Next()) {
			const WorldGrid& wg = Context()->worldMap->GetWorldGrid(it.Pos());
			if (wg.Plant() && !wg.BlockingPlant()) {
				++nPlants;
				pos.PushIfCap(ToWorld2F(it.Pos()));
			}
		}
	}
#else
	if (addPlants) {
		Rectangle2I r;
		r.min = r.max = ToWorld2I(origin);
		r.Outset(int(PLANT_AWARE));

		CChitArray fruit;
		FruitElixirFilter filter;
		Context()->chitBag->QuerySpatialHash(&fruit, r, 0, &filter);

		for (Chit* c : fruit) {
			pos.PushIfCap(ToWorld2F(c->Position()));
		}
	}
#endif

	Vector2F mean = origin;
	// Get close to friends.
	for (int i = 0; i < pos.Size(); ++i) {
		mean = mean + pos[i];
	}
	Vector2F dest = mean * (1.0f / (float)(1 + pos.Size()));
	
#if 0
	// But not too close.
	Vector2F heading = parentChit->Heading2D();
	for (int i = 0; i < pos.Size(); ++i) {
		if ((pos[i] - dest).LengthSquared() < TOO_CLOSE*TOO_CLOSE) {
			dest += heading * TOO_CLOSE;
		}
	}
#endif
	return dest;
}


void AIComponent::GoSectorHerd(bool focus)
{
	SectorHerd(focus);
}


bool AIComponent::SectorHerd(bool focus)
{
	static const int NDELTA = 12;
	static const Vector2I delta[NDELTA] = {
		{ -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
		{ -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 },
		{ -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 },
	};
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;

	const ItemComponent* ic = parentChit->GetItemComponent();
	const ChitContext* context = Context();
	const Vector2F pos = ToWorld2F(parentChit->Position());
	const SectorPort start = context->worldMap->NearestPort(pos, 0);

	//Sometimes we can't path to any port. Hopefully rampage cuts in.
	if (!start.IsValid()) {
		return false;
	}

	Chit* directorChit = Context()->chitBag->GetNamedChit(ISC::Director);
	if (directorChit) {
		Director* director = (Director*)directorChit->GetComponent("Director");
		Vector2I destSector = director->ShouldSendHerd(parentChit);
		if (!destSector.IsZero() && DoSectorHerd(focus, destSector)) {
			return true;
		}
	}

	// Should be choose a shopping destination?
	if (gameItem->flags & GameItem::AI_USES_BUILDINGS) {

		int deityTeam = gameItem->Deity();
		CoreScript* deityCS = deityTeam ? CoreScript::GetCoreFromTeam(deityTeam) : 0;

		// Are we at a deity location? If so, travel far.
		if (deityCS && (ToSector(deityCS->ParentChit()->Position()) == ToSector(parentChit->Position()))) {
			Vector2I destSector = { int(parentChit->random.Rand(NUM_SECTORS)), int(parentChit->random.Rand(NUM_SECTORS)) };
			if (DoSectorHerd(focus, destSector))
				return true;
		}

		int reasonToShop = 0;
		if (!ic->QuerySelectMelee())		reasonToShop++;
		if (!ic->QuerySelectRanged())		reasonToShop++;
		if (!ic->GetShield())				reasonToShop++;
		if (gameItem->wallet.Gold() > 100)	reasonToShop++;
		if (ic->ItemToSell())				reasonToShop += 4;

		if (gameItem->wallet.Gold() < 20 && !ic->ItemToSell())
			reasonToShop = 0;

		if (parentChit->random.Rand(16) < reasonToShop) {
			CArray<Vector2I, 8> markets;

			if (MarketAI::CoreHasMarket(deityCS)) {
				markets.Push(ToSector(deityCS->ParentChit()->Position()));
			}
			Rectangle2I sectors(ToSector(parentChit->Position()));
			for (Rectangle2IIterator it(sectors); !it.Done(); it.Next()) {
				if (MarketAI::CoreHasMarket(CoreScript::GetCore(it.Pos()), parentChit)) {
					markets.PushIfCap(it.Pos());
				}
			}
			if (markets.Size()) {
				Vector2I destSector = markets[parentChit->random.Rand(markets.Size())];
				return DoSectorHerd(focus, destSector);			
			}
		}
	}

	// First pass: filter on attract / repel choices.
	// This is game difficulty logic!
	Rectangle2I sectorBounds;
	sectorBounds.Set(0, 0, NUM_SECTORS - 1, NUM_SECTORS - 1);
	IString mob = gameItem->keyValues.GetIString(ISC::mob);

	float rank[NDELTA] = { 0 };
	int possible = 0;
	for (int i = 0; i < NDELTA; ++i) {
		Vector2I destSector = start.sector + delta[i];
		if (!sectorBounds.Contains(destSector)) continue;
		CoreScript* cs = CoreScript::GetCore(destSector);

		// Check repelled / attracted.
		if (cs && cs->InUse() && Team::Instance()->GetRelationship(cs->ParentChit(), parentChit) == ERelate::ENEMY) {
			int nTemples = cs->NumTemples();

			// For enemies, apply rules to make the gameplay smoother.
			if (mob == ISC::lesser) {
				if (nTemples <= TEMPLES_REPELS_LESSER) {
					rank[i] = 0.5f;
					++possible;
				}
				else {
					rank[i] = 0;
				}
			}
			else if (mob == ISC::greater) {
				if (nTemples < TEMPLES_REPELS_GREATER) {
					rank[i] = 0;
				}
				else if (nTemples == TEMPLES_REPELS_GREATER) {
					rank[i] = 0.5;
					++possible;
				}
				else {
					rank[i] = float(nTemples - TEMPLES_REPELS_GREATER);
					++possible;
				}
			}
		}
		else if (cs) {
			++possible;
			rank[i] = 1;
		}

	}

	// 2nd pass: look for 1st match
	if (start.IsValid() && possible) {
		int destIdx = parentChit->random.Select(rank, NDELTA);
		if (rank[destIdx] > 0) {
			Vector2I dest = start.sector + delta[destIdx];
			if (DoSectorHerd(focus, dest)) {
				return true;
			}
		}
	}
	return false;
}


bool AIComponent::DoSectorHerd( bool focus, const grinliz::Vector2I& sector)
{
	Vector2F dst = ToWorld2F(SectorBounds(sector).Center());
	SectorPort sd = Context()->worldMap->NearestPort(dst, nullptr);
	return DoSectorHerd(focus, sd);
}


bool AIComponent::DoSectorHerd(bool focus, const SectorPort& dest)
{
	if (dest.IsValid()) {
		GLASSERT(dest.port);

		RenderComponent* rc = parentChit->GetRenderComponent();
		if (rc) {
			rc->AddDeco("horn", 10 * 1000);
		}
		const GameItem* gameItem = parentChit->GetItem();
		if (!gameItem) return false;

		CStr<32> str;
		str.Format("%s\nSectorHerd", gameItem->Name());
		ChitBag::CurrentNews news = { StringPool::Intern(str.c_str()), ToWorld2F(parentChit->Position()), parentChit->ID() };
		Context()->chitBag->PushCurrentNews(news);

		ChitMsg msg(ChitMsg::CHIT_SECTOR_HERD, focus ? 1 : 0, &dest);
		for (int i = 0; i < friendList2.Size(); ++i) {
			Chit* c = Context()->chitBag->GetChit(friendList2[i]);
			if (c) {
				c->SendMessage(msg);
			}
		}
		parentChit->SendMessage(msg);
		if (Log()) {
			GLOUTPUT(("AI ID=%d SectorHerd to sector %d,%d\n", parentChit->ID(), dest.sector.x, dest.sector.y));
		}
		return true;
	}
	return false;
}


void AIComponent::ThinkVisitor()
{
	if (parentChit->StackedMoveComponent()) {
		return;	 // grid travel, fixes, etc.
	}

	Vector2I pos2i = ToWorld2I(parentChit->Position());
	Vector2I sector = ToSector(pos2i);
	VisitorData* vd = Visitors::Get(visitorIndex);

	Chit* temple = Context()->chitBag->FindBuilding(ISC::temple, sector, 0, LumosChitBag::EFindMode::NEAREST, 0, 0);
	Chit* kiosk = Context()->chitBag->FindBuilding(ISC::kiosk,
													sector,
													0,
													LumosChitBag::EFindMode::RANDOM_NEAR, 0, 0);

	bool doneAtThisSector = (vd->visited.Find(sector) >= 0) || !kiosk || !temple;
	bool disconnect = false;

	if (!doneAtThisSector) {
		MapSpatialComponent* msc = GET_SUB_COMPONENT(kiosk, SpatialComponent, MapSpatialComponent);
		GLASSERT(msc);
		Rectangle2I porch = msc->PorchPos();

		if (Context()->worldMap->CalcPath(ToWorld2F(parentChit->Position()), ToWorld2F(porch.min), 0, 0)) {
			taskList.Push(Task::MoveTask(porch.min));
			taskList.Push(Task::StandTask(2000));
			taskList.Push(Task::UseBuildingTask());
		}
		else {
			// no path; wrap it up.
			doneAtThisSector = true;
		}
	}
	if (doneAtThisSector) {
		LumosChitBag* chitBag = Context()->chitBag;
		const Web& web = chitBag->GetSim()->GetCachedWeb();
		SectorPort sp = Visitors::Instance()->ChooseDestination(visitorIndex, web, chitBag, Context()->worldMap);

		if (sp.IsValid()) {
			this->Move(sp, true);
		}
		else {
			disconnect = true;
		}
	}
	if (disconnect) {
		parentChit->GetItem()->hp = 0;
		parentChit->SetTickNeeded();
	}
}


bool AIComponent::ThinkCollectNearFruit()
{
	if (parentChit->PlayerControlled()) return false;
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;
	if (gameItem->IsWorker()) return false;

	ItemComponent* ic = parentChit->GetItemComponent();
	if (!ic) return false;
	if (!ic->CanAddToInventory()) return false;

	int count = ic->NumItems(ISC::fruit) + ic->NumItems(ISC::elixir);

	if (gameItem->HPFraction() < EAT_WILD_FRUIT
		|| (ic->CanAddToInventory() && count < 2))
	{
		Vector2F pos2 = ToWorld2F(parentChit->Position());

		// Are we near fruit?
		CChitArray arr;
		FruitElixirFilter fruitFilter;
		Context()->chitBag->QuerySpatialHash(&arr, pos2, PLANT_AWARE, 0, &fruitFilter);

		for (int i = 0; i < arr.Size(); ++i) {
			Vector2I plantPos = ToWorld2I(arr[i]->Position());
			if (Context()->worldMap->HasStraightPath(pos2, ToWorld2F(plantPos), false)) {
				if (Log()) {
					GLOUTPUT(("AI ID=%d ThinkCollectNearFruit\n", parentChit->ID()));
				}
				taskList.Push(Task::MoveTask(plantPos));
				taskList.Push(Task::PickupTask(arr[i]->ID()));
				return true;
			}
		}
	}
	return false;
}


Vector2I AIComponent::RandomPosInRect( const grinliz::Rectangle2I& rect, bool excludeCenter )
{
	Vector2I v = { 0, 0 };
	while( true ) {
		 v.Set( rect.min.x + parentChit->random.Rand( rect.Width() ),
				rect.min.y + parentChit->random.Rand( rect.Height() ));
		if ( excludeCenter && v == rect.Center() )
			continue;
		break;
	}
	return v;
}


bool AIComponent::ThinkGuard() {
	const GameItem *gameItem = parentChit->GetItem();
	if (!gameItem) return false;

	if (!(gameItem->flags & GameItem::AI_USES_BUILDINGS)
		|| !(gameItem->flags & GameItem::HAS_NEEDS)
		|| parentChit->PlayerControlled()
		|| !AtHomeCore()) {
		return false;
	}

	if (gameItem->GetPersonality().Guarding() == Personality::DISLIKES) {
		return false;
	}

	Vector2I pos2i = ToWorld2I(parentChit->Position());
	Vector2I sector = ToSector(pos2i);
	Rectangle2I bounds = InnerSectorBounds(sector);

	CoreScript *coreScript = CoreScript::GetCore(sector);

	if (!coreScript) return false;

	ItemNameFilter filter(ISC::guardpost);
	Context()->chitBag->FindBuilding(IString(), sector, 0, LumosChitBag::EFindMode::NEAREST, &chitArr, &filter);

	if (chitArr.Empty()) return false;

	// Are we already guarding??
	for (int i = 0; i < chitArr.Size(); ++i) {
		Rectangle2I guardBounds;

		guardBounds.min = guardBounds.max = ToWorld2I(chitArr[i]->Position());
		guardBounds.Outset(GUARD_RANGE);
		guardBounds.DoIntersection(bounds);

		if (guardBounds.Contains(pos2i)) {
			taskList.Push(Task::MoveTask(ToWorld2F(RandomPosInRect(guardBounds, true))));
			taskList.Push(Task::StandTask(GUARD_TIME));
			if (Log()) {
				GLOUTPUT(("AI ID=%d ThinkGuard continued.\n", parentChit->ID()));
			}
			return true;
		}
	}

	int post = parentChit->random.Rand(chitArr.Size());
	Rectangle2I guardBounds;
	guardBounds.min = guardBounds.max = ToWorld2I(chitArr[post]->Position());
	guardBounds.Outset(GUARD_RANGE);
	guardBounds.DoIntersection(bounds);

	taskList.Push(Task::MoveTask(ToWorld2F(RandomPosInRect(guardBounds, true))));
	taskList.Push(Task::StandTask(GUARD_TIME));
	if (Log()) {
		GLOUTPUT(("AI ID=%d ThinkGuard\n", parentChit->ID()));
	}
	return true;
}


bool AIComponent::AtHomeCore()
{
	Vector2I sector = ToSector( parentChit->Position());
	CoreScript* coreScript = CoreScript::GetCore(sector);

	if ( !coreScript ) return false;
	return coreScript->IsCitizen( parentChit->ID() );
}


bool AIComponent::AtFriendlyOrNeutralCore()
{
	Vector2I sector = ToSector( parentChit->Position());
	CoreScript* coreScript = CoreScript::GetCore( sector );
	if (coreScript) {
		return Team::Instance()->GetRelationship(parentChit, coreScript->ParentChit()) != ERelate::ENEMY;
	}
	return false;
}


void AIComponent::FindFruit( const Vector2F& pos2, Vector2F* dest, CChitArray* arr, bool* nearPath )
{
	// Be careful of performance. Allow one full pathing, use
	// direct pathing for the rest.
	// Also, kind of tricky. Fruit is usually blocked, so it
	// can be picked up on its grid, or the closest neighbor
	// grid. And check near the mob and near farms.

	const ChitContext* context	= this->Context();
	LumosChitBag* chitBag		= this->Context()->chitBag;
	const Vector2I sector		= ToSector(pos2);
	CoreScript* cs				= CoreScript::GetCore(sector);

	FruitElixirFilter filter;
	*nearPath = false;

	// Check local. For local, use direct path.
	CChitArray chitArr;
	chitBag->QuerySpatialHash( &chitArr, pos2, FRUIT_AWARE, 0, &filter );
	for (int i = 0; i < chitArr.Size(); ++i) {
		Chit* chit = chitArr[i];
		Vector2F fruitPos = ToWorld2F(chit->Position());
		if (context->worldMap->HasStraightPath(pos2, fruitPos, false)) {
			*dest = fruitPos;
			arr->Push(chit);
			// Push other fruit at this same location.
			for (int k = i + 1; k < chitArr.Size(); ++k) {
				if (ToWorld2I(chitArr[k]->Position()) == ToWorld2I(chit->Position())) {
					arr->Push(chitArr[k]);
				}
			}
			*nearPath = true;
			return;
		}
	}

	// Okay, now check the farms and distilleries. 
	// Now we need to use full pathing for meaningful results.
	// Only need to check the farm porch - all fruit goes there.
	for (int pass = 0; pass < 2; ++pass) {
		CChitArray buildingArr;
		chitBag->FindBuildingCC(pass == 0 ? ISC::farm : ISC::distillery,
								ToSector(ToWorld2I(pos2)),
								0, LumosChitBag::EFindMode::NEAREST, &buildingArr, 0);

		for (int i = 0; i < buildingArr.Size(); ++i) {
			Chit* farmChit = buildingArr[i];
			MapSpatialComponent* farmMSC = GET_SUB_COMPONENT(farmChit, SpatialComponent, MapSpatialComponent);
			GLASSERT(farmMSC);
			if (farmMSC) {
				Rectangle2I porch = farmMSC->PorchPos();
				Vector2F farmLoc = ToWorld2F(porch).Center();
				chitBag->QuerySpatialHash(&chitArr, farmLoc, 1.0f, 0, &filter);

				for (int j = 0; j < chitArr.Size(); ++j) {
					Vector2F fruitLoc = ToWorld2F(chitArr[j]->Position());
					Vector2I fruitLoc2i = ToWorld2I(fruitLoc);
					// Skip if someone probably heading there.
					if (cs && cs->HasTask(fruitLoc2i)) {
						continue;
					}

					if (context->worldMap->CalcPath(pos2, fruitLoc, 0, 0, 0)) {
						*dest = fruitLoc;
						Chit* chit = chitArr[j];
						arr->Push(chit);
						// Push other fruit at this same location.
						for (int k = j + 1; k < chitArr.Size(); ++k) {
							if (ToWorld2I(chitArr[k]->Position()) == ToWorld2I(chit->Position())) {
								arr->Push(chitArr[k]);
							}
						}
						return;
					}
				}
			}
		}
	}
	dest->Zero();
	return;
}



bool AIComponent::ThinkFruitCollect()
{
	// Some buildings provide food - that happens in ThinkNeeds.
	// This is for picking up food on the group.

	if (parentChit->PlayerControlled()) {
		return false;
	}
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;
	if (gameItem->IsWorker()) return false;	// workers carrying fruit seemed strange

	double need = 1;
	if (gameItem->flags & GameItem::HAS_NEEDS) {
		const ai::Needs& needs = GetNeeds();
		need = needs.Value(Needs::FOOD);
	}

	// It is the duty of every citizen to take fruit to the distillery.
	if (AtHomeCore()) {
		if (gameItem->GetPersonality().Botany() != Personality::DISLIKES
			|| need < NEED_CRITICAL * 2.0)
		{
			// Can we go pick something up?
			if (parentChit->GetItemComponent() && parentChit->GetItemComponent()->CanAddToInventory()) {
				Vector2F fruitPos = { 0, 0 };
				CChitArray fruit;
				bool nearPath = false;
				FindFruit(ToWorld2F(parentChit->Position()), &fruitPos, &fruit, &nearPath);
				if (fruit.Size()) {
					Vector2I fruitPos2i = ToWorld2I(fruitPos);
					Vector2I sector = ToSector(fruitPos2i);
					CoreScript* cs = CoreScript::GetCore(sector);
					if (!nearPath && cs && cs->HasTask(fruitPos2i)) {
						// Do nothing; assume it is this task.
					}
					else {
						taskList.Push(Task::MoveTask(fruitPos));
						for (int i = 0; i < fruit.Size(); ++i) {
							taskList.Push(Task::PickupTask(fruit[i]->ID()));
						}
						if (Log()) {
							GLOUTPUT(("AI ID=%d ThinkFruitCollect\n", parentChit->ID()));
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}


bool AIComponent::ThinkFlag()
{
	if (parentChit->PlayerControlled()) {
		return false;
	}
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;

	bool usesBuilding = (gameItem->flags & GameItem::AI_USES_BUILDINGS) != 0;
	if (!usesBuilding) {
		return false;
	}

	// Flags are done by denizens, NOT workers.
	// This handles flags in the same domain.
	CoreScript* homeCoreScript = CoreScript::GetCoreFromTeam(parentChit->Team());
	if (!homeCoreScript)
		return false;

	Vector2I homeCoreSector = ToSector(homeCoreScript->ParentChit()->Position());
	Vector2I sector = ToSector(parentChit->Position());

	if (   homeCoreScript->IsCitizen(parentChit->ID())
		&& ( homeCoreSector == sector))
	{
		Vector2I flag = homeCoreScript->GetAvailableFlag();
		if (!flag.IsZero()) {
			if (!homeCoreScript->HasTask(flag)) {
				taskList.Push(Task::MoveTask(flag));
				taskList.Push(Task::FlagTask());
				if (Log()) {
					GLOUTPUT(("AI ID=%d ThinkFlag\n", parentChit->ID()));
				}
				return true;
			}
		}
	}
	return false;
}


bool AIComponent::ThinkHungry()
{
	if (parentChit->PlayerControlled()) return false;

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return false;
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;

	if (gameItem->IsWorker()) return false;

	const GameItem* fruit = thisIC->FindItem(ISC::fruit);
	const GameItem* elixir = thisIC->FindItem(ISC::elixir);
	bool carrying = fruit || elixir;

	// Are we carrying fruit?? If so, eat if hungry!
	if (carrying) {
		if (((gameItem->flags & GameItem::HAS_NEEDS) && needs.Value(Needs::FOOD) < NEED_CRITICAL)	// hungry
			|| (gameItem->HPFraction() < 0.4f))														// hurt
		{
			if (elixir) {
				GameItem* item = thisIC->RemoveFromInventory(elixir);
				delete item;
				this->GetNeedsMutable()->Set(Needs::FOOD, 1);
				parentChit->GetItem()->Heal(100);
			}
			else if (fruit) {
				GameItem* item = thisIC->RemoveFromInventory(fruit);
				delete item;
				this->GetNeedsMutable()->Set(Needs::FOOD, 1);
				parentChit->GetItem()->Heal(100);
			}
			if (Log()) {
				GLOUTPUT(("AI ID=%d ThinkHungry\n", parentChit->ID()));
			}
			return true;	// did something...?
		}
	}
	return false;
}


bool AIComponent::ThinkDelivery()
{
	if (parentChit->PlayerControlled()) {
		return false;
	}

	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;
	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return false;

	bool usesBuilding = (gameItem->flags & GameItem::AI_USES_BUILDINGS) != 0;
	bool worker = gameItem->IsWorker();

	if (worker) {
		bool needVaultRun = false;

		if (!thisIC->GetItem(0)->wallet.IsEmpty()) {
			needVaultRun = true;
		}

		if (needVaultRun == false) {
			for (int i = 1; i < thisIC->NumItems(); ++i) {
				const GameItem* item = thisIC->GetItem(i);
				if (item->Intrinsic())
					continue;
				if (item->ToRangedWeapon() || item->ToMeleeWeapon() || item->ToShield())
				{
					needVaultRun = true;
					break;
				}
			}
		}
		if (needVaultRun) {
			Vector2I sector = ToSector(parentChit->Position());
			Vector2F pos = ToWorld2F(parentChit->Position());
			Chit* vault = Context()->chitBag->FindBuilding(ISC::vault,
														   sector,
														   &pos,
														   LumosChitBag::EFindMode::RANDOM_NEAR, 0, 0);
			if (vault && vault->GetItemComponent() && vault->GetItemComponent()->CanAddToInventory()) {
				MapSpatialComponent* msc = GET_SUB_COMPONENT(vault, SpatialComponent, MapSpatialComponent);
				GLASSERT(msc);
				CoreScript* coreScript = CoreScript::GetCore(ToSector(msc->MapPosition()));
				if (msc && coreScript) {
					Rectangle2I porch = msc->PorchPos();
					for (Rectangle2IIterator it(porch); !it.Done(); it.Next()) {
						if (!coreScript->HasTask(it.Pos())) {
							taskList.Push(Task::MoveTask(it.Pos()));
							taskList.Push(Task::UseBuildingTask());
							if (Log()) {
								GLOUTPUT(("AI ID=%d ThinkDelivery\n", parentChit->ID()));
							}
							return true;
						}
					}
				}
			}
		}
	}

	if (worker || usesBuilding)
	{
		for (int pass = 0; pass < 2; ++pass) {
			const IString iItem = (pass == 0) ? ISC::elixir : ISC::fruit;
			const IString iBuilding = (pass == 0) ? ISC::bar : ISC::distillery;

			const GameItem* item = thisIC->FindItem(iItem);
			if (item) {
				Vector2I sector = ToSector(parentChit->Position());
				Vector2F pos = ToWorld2F(parentChit->Position());
				Chit* building = Context()->chitBag->FindBuilding(iBuilding, sector,
																  &pos,
																  LumosChitBag::EFindMode::RANDOM_NEAR, 0, 0);

				if (building && building->GetItemComponent() && building->GetItemComponent()->CanAddToInventory()) {
					MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
					GLASSERT(msc);
					CoreScript* coreScript = CoreScript::GetCore(sector);
					if (msc && coreScript) {
						Rectangle2I porch = msc->PorchPos();
						for (Rectangle2IIterator it(porch); !it.Done(); it.Next()) {
							if (!coreScript->HasTask(it.Pos())) {
								taskList.Push(Task::MoveTask(it.Pos()));
								taskList.Push(Task::UseBuildingTask());
								if (Log()) {
									GLOUTPUT(("AI ID=%d ThinkDelivery\n", parentChit->ID()));
								}
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}


bool AIComponent::ThinkRepair()
{
	if (parentChit->PlayerControlled()) {
		return false;
	}
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;

	bool worker = gameItem->IsWorker();
	if (!worker) return false;

	BuildingRepairFilter filter;
	Vector2I sector = ToSector(parentChit->Position());

	Vector2F pos = ToWorld2F(parentChit->Position());
	Chit* building = Context()->chitBag->FindBuilding(IString(),
		sector,
		&pos,
		LumosChitBag::EFindMode::RANDOM_NEAR, 0, &filter);

	if (!building) return false;

	MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
	GLASSERT(msc);
	Rectangle2I repair;
	repair.Set(0, 0, 0, 0);
	if (msc) {
		if (msc->HasPorch()) {
			repair = msc->PorchPos();
		}
		else {
			repair = msc->Bounds();
			repair.Outset(1);
		}
	}
	CoreScript* coreScript = CoreScript::GetCore(ToSector(msc->MapPosition()));
	if (!coreScript) return false;

	WorldMap* worldMap = Context()->worldMap;
	Vector2F pos2 = ToWorld2F(parentChit->Position());

	for (Rectangle2IIterator it(repair); !it.Done(); it.Next()) {
		if (!coreScript->HasTask(it.Pos()) 
			&& worldMap->CalcPath( pos2, ToWorld2F(it.Pos()), 0, 0, false)) 
		{
			taskList.Push(Task::MoveTask(it.Pos()));
			taskList.Push(Task::StandTask(REPAIR_TIME));
			taskList.Push(Task::RepairTask(building->ID()));
			if (Log()) {
				GLOUTPUT(("AI ID=%d ThinkRepair\n", parentChit->ID()));
			}
			return true;
		}
	}
	return false;
}


bool AIComponent::ThinkNeeds()
{
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;

	if (!(gameItem->flags & GameItem::AI_USES_BUILDINGS)
		|| !(gameItem->flags & GameItem::HAS_NEEDS)
		|| parentChit->PlayerControlled())
	{
		return false;
	}

	Vector2I pos2i = ToWorld2I(parentChit->Position());
	Vector2I sector = ToSector(pos2i);
	CoreScript* coreScript = CoreScript::GetCore(sector);

	if (!coreScript) return false;
	if (Team::Instance()->GetRelationship(parentChit, coreScript->ParentChit()) == ERelate::ENEMY) return false;
	Vector3<double> myNeeds = this->GetNeeds().GetOneMinus();
	if (myNeeds.IsZero()) return false;

	BuildingFilter filter;
	Context()->chitBag->FindBuilding(IString(), sector, 0, LumosChitBag::EFindMode::NEAREST, &chitArr, &filter);

	BuildScript	buildScript;
	float				score2Arr[BuildScript::NUM_TOTAL_OPTIONS] = { 0 };
	const BuildData*	bdArr[BuildScript::NUM_TOTAL_OPTIONS]     = { 0 };
	Vector2I			porchArr[BuildScript::NUM_TOTAL_OPTIONS];

	// Cheat a bit, so the denizen just doesn't go after food and sleep all the time.
	if (myNeeds.X(Needs::ENERGY) < 0.5 && gameItem->HPFraction() >= 0.9) myNeeds.X(Needs::ENERGY) = 0;
	if (myNeeds.X(Needs::FOOD) < 0.5) myNeeds.X(Needs::FOOD) = 0;

	double myMorale = this->GetNeeds().Morale();
	int hits = 0;

	bool debugBuildingOutput[BuildScript::NUM_TOTAL_OPTIONS] = { false };

	for (int i = 0; i < chitArr.Size(); ++i) {
		Chit* building = chitArr[i];
		GLASSERT(building->GetItem());
		int buildDataID = 0;
		const BuildData* bd = buildScript.GetDataFromStructure(building->GetItem()->IName(), &buildDataID);
		if (!bd || bd->needs.IsZero()) continue;
		GLASSERT(buildDataID >= 0 && buildDataID < BuildScript::NUM_TOTAL_OPTIONS);

		MapSpatialComponent* msc = GET_SUB_COMPONENT(building, SpatialComponent, MapSpatialComponent);
		GLASSERT(msc);

		// Check to make sure a task isn't reserved at this location:
		Vector2I porch = { 0, 0 };
		Rectangle2I porchRect = msc->PorchPos();
		for (Rectangle2IIterator it(porchRect); !it.Done(); it.Next()) {
			if (!coreScript->HasTask(it.Pos())) {
				porch = it.Pos();
				break;
			}
		}
		if (porch.IsZero())	continue;

		// It's viable: now eval needs.
		Vector3<double> buildingNeeds = ai::Needs::CalcNeedsFullfilledByBuilding(building, parentChit);
		if (buildingNeeds.IsZero()) continue;

		double score = DotProduct(buildingNeeds, myNeeds);
		if (score == 0) continue;

		// Variation - is this the last building visited?
		// This is here to break up the bar-sleep loop. But only if we have full morale.
		if (myMorale > 0.99) {
			for (int k = 0; k < TaskList::NUM_BUILDING_USED; ++k) {
				if (bd->structure == taskList.BuildingsUsed()[k]) {
					// Tricky number to get right; note the bestScore >= SOMETHING filter below.
					score *= 0.6 + 0.1 * double(k);
				}
			}
		}
		// Wobble - slight preference.
		score = score + 0.0001 * parentChit->random.Hash8(building->ID() + parentChit->ID());

		if (Log()) {
			if (!debugBuildingOutput[buildDataID]) {
				GLASSERT(buildDataID >= 0 && buildDataID < int(GL_C_ARRAY_SIZE(debugBuildingOutput)));
				debugBuildingOutput[buildDataID] = true;
				GLOUTPUT(("  %.2f %s\n", score, building->GetItem()->Name()));
			}
		}

		float score2 = float(score * score);

		if ((score > 0.1) && (score2 > score2Arr[buildDataID])) {
			score2Arr[buildDataID] = score2;
			bdArr[buildDataID] = bd;
			porchArr[buildDataID] = porch;
			++hits;
		}
	}

	if (hits) {
		int idx = parentChit->random.Select(score2Arr, BuildScript::NUM_TOTAL_OPTIONS);
		if (bdArr[idx]) {
			GLASSERT(porchArr[idx].x > 0);
			if (Log()) {
				GLOUTPUT(("  --> %s\n", bdArr[idx]->structure.c_str()));
			}

			taskList.Push(Task::MoveTask(porchArr[idx]));
			taskList.Push(Task::StandTask(bdArr[idx]->standTime));
			taskList.Push(Task::UseBuildingTask());
			return true;
		}
	}
	return false;
}


bool AIComponent::ThinkLoot()
{
	// Is there stuff around to pick up?
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return false;
	int flags = gameItem->flags;
	if ((flags & (GameItem::GOLD_PICKUP | GameItem::ITEM_PICKUP)) == 0) return false;
	if (parentChit->PlayerControlled()) return false;

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return false;

	const ChitContext* context = Context();
	Vector2I sector = ToSector(parentChit->Position());

	// RULE: If there isn't a Vault, then the workers shouldn't loot.
	if (gameItem->IsWorker() && !Context()->chitBag->QueryBuilding(ISC::vault, SectorBounds(sector), 0)) return false;

	// Which filter to use?
	GoldCrystalFilter	gold;
	LootFilter			loot;

	MultiFilter filter(MultiFilter::MATCH_ANY);
	bool canAdd = thisIC->CanAddToInventory();

	if (canAdd && (flags & GameItem::ITEM_PICKUP)) {
		filter.filters.Push(&loot);
	}
	if (flags & GameItem::GOLD_PICKUP) {
		filter.filters.Push(&gold);
	}

	Vector2F pos2 = ToWorld2F(parentChit->Position());
	CChitArray chitArr;
	parentChit->Context()->chitBag->QuerySpatialHash(&chitArr, pos2, GOLD_AWARE, 0, &filter);

	ChitDistanceCompare compare(parentChit->Position());
	SortContext(chitArr.Mem(), chitArr.Size(), compare);

	for (int i = 0; i < chitArr.Size(); ++i) {
		Vector2F goldPos = ToWorld2F(chitArr[i]->Position());
		if (context->worldMap->HasStraightPath(goldPos, pos2, false)) {
			taskList.Push(Task::MoveTask(goldPos));
			if (loot.Accept(chitArr[i])) {
				// Gold is hoovered-up. Only need pickup for loot.
				taskList.Push(Task::PickupTask(chitArr[i]->ID()));
			}
			if (Log()) {
				GLOUTPUT(("AI ID=%d ThinkLoot\n", parentChit->ID()));
			}
			return true;
		}
	}
	return false;
}


void AIComponent::ThinkNormal()
{
	// Wander in some sort of directed fashion.
	// - get close to friends
	// - but not too close
	// - given a choice, get close to plants.
	// - occasionally randomly wander about

	Vector2F dest = { 0, 0 };
	const GameItem* item = parentChit->GetItem();
	int itemFlags = item ? item->flags : 0;
	int wanderFlags = itemFlags & GameItem::AI_WANDER_MASK;

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return;
	const GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return;

	RangedWeapon* ranged = thisIC->GetRangedWeapon(0);
	if (ranged && ranged->CanReload()) {
		ranged->Reload(parentChit);
	}

	if (ThinkWaypoints())
		return;

	if (ThinkLoot())
		return;
	if (ThinkHungry())
		return;
	if (ThinkCollectNearFruit())
		return;

	// Be sure to deliver before collecting!
	if (ThinkDelivery())
		return;
	if (ThinkFruitCollect())
		return;
	if (ThinkFlag())
		return;
	if (ThinkNeeds())
		return;
	if (ThinkRepair())
		return;
	if (ThinkGuard())
		return;
	if (ThinkDoRampage())
		return;

	// Wander....
	if (dest.IsZero()) {
		if (parentChit->PlayerControlled()) {
			currentAction = AIAction::NO_ACTION;
			return;
		}
		Chit* directorChit = Context()->chitBag->GetNamedChit(ISC::Director);
		if (directorChit) {
			Director* director = (Director*)directorChit->GetComponent("Director");
			Vector2I destSector = director->PrioritySendHerd(parentChit);
			if (!destSector.IsZero() && DoSectorHerd(false, destSector)) {
				return;
			}
		}

		PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);

		// Denizens DO sector herd until they are members of a core.
		int oddsWander = 60;
		if (item->MOB() == ISC::lesser)  oddsWander = (friendList2.Size() >= MAX_TRACK * 3 / 4) ? 50 : 200;
		if (item->MOB() == ISC::denizen) oddsWander = 40;
		if (item->MOB() == ISC::greater) oddsWander = 20;

		if (item->IName() == ISC::troll) oddsWander = 20;
		if (item->IsWorker()) oddsWander = 0;

		bool sectorHerd = pmc
			&& (Team::ID(parentChit->Team()) == 0)			// rogue or chaos or etc.
			&& oddsWander
			&& (parentChit->random.Rand(oddsWander) == 0);

		if (sectorHerd)	{
			if (SectorHerd(false))
				return;
		}
		else {
			// ThinkWanderCircle() would be fun, but not working right now.
//			if (wanderFlags & GameItem::AI_WANDER_CIRCLE)
//				dest = ThinkWanderCircle();
//			else 
			dest = (pmc && pmc->ForceCountHigh()) ? ThinkWanderRandom() : ThinkWanderHerd();
		}
	}
	if (!dest.IsZero()) {
		if (Log()) {
			GLOUTPUT(("AI ID=%d Dest/Wander\n", parentChit->ID()));
		}
		Vector2I dest2i = { (int)dest.x, (int)dest.y };
		// If the move is very near (happens if friendList empty)
		// don't do the move to avoid jerk.
		if (dest2i != ToWorld2I(parentChit->Position())) {
			taskList.Push(Task::MoveTask(dest2i));
		}
		taskList.Push(Task::StandTask(STAND_TIME_WHEN_WANDERING));
	}
}


void AIComponent::ThinkBattle()
{
	PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
	if (!pmc) {
		currentAction = AIAction::NO_ACTION;
		return;
	}
	const Vector3F pos = parentChit->Position();
	Vector2F pos2 = { pos.x, pos.z };

	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return;

	// Call out to eat food, if we have it.
	ThinkHungry();

	// Use the current or reserve - switch out later if we need to.
	const RangedWeapon* rangedWeapon = thisIC->QuerySelectRanged();
	const MeleeWeapon*  meleeWeapon = thisIC->QuerySelectMelee();

	enum {
		OPTION_NONE,			// Stand around
		OPTION_MOVE_TO_RANGE,	// Move to shooting range or striking range
		OPTION_MELEE,			// Focused melee attack
		OPTION_SHOOT,			// Stand and shoot. (Don't pay run-n-gun accuracy penalty.)
		NUM_OPTIONS
	};

	float utility[NUM_OPTIONS] = { 0, 0, 0, 0 };
	int   target[NUM_OPTIONS] = { 0, 0, 0, 0 };

	// Moves are always to the target location, since intermediate location could
	// cause very strange pathing. Time is set to when to "rethink". If we are moving
	// to melee for example, the time is set to when we should actually switch to
	// melee. Same applies to effective range. If it isn't a straight path, we will
	// rethink too soon, which is okay.
	//float    moveToTime = 1.0f;	// Seconds to the desired location is reached.

	// Consider flocking. This wasn't really working in a combat situation.
	// May reconsider later, or just use for spreading out.
	//static  float FLOCK_MOVE_BIAS = 0.2f;
	Vector2F heading = parentChit->Heading2D();
	utility[OPTION_NONE] = 0;

	int nRangedEnemies = 0;	// number of enemies that could be shooting at me
	int nMeleeEnemies = 0;	// number of enemies that could be pounding at me

	// Count melee and ranged enemies.
	for (int k = 0; k < enemyList2.Size(); ++k) {
		Chit* chit = Context()->chitBag->GetChit(enemyList2[k]);
		if (chit) {
			ItemComponent* ic = chit->GetItemComponent();
			if (ic) {
				if (ic->GetRangedWeapon(0)) {
					++nRangedEnemies;
				}
				static const float MELEE_ZONE = (MELEE_RANGE + 0.5f) * (MELEE_RANGE + 0.5f);
				if (ic->GetMeleeWeapon() &&
					(pos2 - ToWorld2F(chit->Position())).LengthSquared() <= MELEE_ZONE)
				{
					++nMeleeEnemies;
				}
			}
		}
	}

	BuildingFilter buildingFilter;

	for (int k = 0; k < enemyList2.Size(); ++k) {
		const ChitContext* context = Context();
		int targetID = enemyList2[k];

		Chit* enemyChit = context->chitBag->GetChit(targetID);	// null if there isn't a chit
		//Vector2I voxelTarget = ToWG(targetID);					// zero if there isn't a voxel target

		const Vector3F	enemyPos = EnemyPos(targetID, true);
		const Vector2F	enemyPos2 = { enemyPos.x, enemyPos.z };
		const Vector2I  enemyPos2I = ToWorld2I(enemyPos2);
		float			range = (enemyPos - pos).Length();
		Vector3F		toEnemy = (enemyPos - pos);
		Vector2F		normalToEnemy = { toEnemy.x, toEnemy.z };
		bool			enemyMoving = (enemyChit && enemyChit->GetMoveComponent()) ? enemyChit->GetMoveComponent()->IsMoving() : false;

		// FIXME: fires every so often. system seems to recover. problem?
		// GLASSERT(FEFilter<RELATE_FRIEND>(parentChit, targetID) == true);

		normalToEnemy.Normalize();
		float dot = DotProduct(normalToEnemy, heading);

		// Prefer targets we are pointed at.
		static const float DOT_BIAS = 0.25f;
		float q = 1.0f + dot * DOT_BIAS;

		// Prefer the current target & focused target.
		if (targetID == lastTargetID) q *= 2.0f;
		if (k == 0 && focus == FOCUS_TARGET) q *= 20;

		// Prefer MOBs over buildings or terrain targets.
		if ((nMeleeEnemies || nRangedEnemies) && (!enemyChit || buildingFilter.Accept(enemyChit))) {
			q *= 0.1f;
		}
		if (Log()) {
			GLOUTPUT(("  id=%d rng=%.1f ", targetID, range));
		}

		// Consider ranged weapon options: OPTION_SHOOT, OPTION_MOVE_TO_RANGE
		if (rangedWeapon) {
			float radAt1 = BattleMechanics::ComputeRadAt1(parentChit->GetItem(),
														  rangedWeapon,
														  false,	// SHOOT implies stopping.
														  enemyMoving);

			float effectiveRange = BattleMechanics::EffectiveRange(radAt1);

			// 1.5f gives spacing for bolt to start.
			// The HasRound() && !Reloading() is really important: if the gun
			// is in cooldown, don't give up on shooting and do something else!
			if (range > 1.5f
				&& ((rangedWeapon->HasRound() && !rangedWeapon->Reloading())		// we have ammod
				|| (nRangedEnemies == 0 && range > 2.0f)))	// we have a gun and they don't
			{
				float u = 1.0f - (range - effectiveRange) / effectiveRange;
				u = Clamp(u, 0.0f, 2.0f);	// just to keep the point blank shooting down.
				u *= q;

				// If we have melee targets, focus in on those.
				if (nMeleeEnemies) {
					u *= 0.1f;
				}
				// This needs tuning.
				// If the unit has been shooting, time to do something else.
				// Stand around and shoot battles are boring and look crazy.
				if (currentAction == AIAction::SHOOT) {
					u *= 0.8f;	// 0.5f;
				}

				// Don't blow ourself up:
				if (rangedWeapon->flags & GameItem::EFFECT_EXPLOSIVE) {
					if (range < EXPLOSIVE_RANGE * 2.0f) {
						u *= 0.1f;	// blowing ourseves up is bad.
					}
				}

				bool lineOfSight = false;
				if (enemyChit) lineOfSight = LineOfSight(enemyChit, rangedWeapon);
				else if (!enemyPos2I.IsZero()) lineOfSight = LineOfSight(enemyPos2I);

				if (Log()) {
					GLOUTPUT(("r=%.1f ", u));
				}

				if (u > utility[OPTION_SHOOT] && lineOfSight) {
					utility[OPTION_SHOOT] = u;
					target[OPTION_SHOOT] = targetID;
				}
			}
			// Move to the effective range?
			float u = (range - effectiveRange) / effectiveRange;
			u *= q;
			if (!rangedWeapon->CanShoot()) {
				// Moving to effective range is less interesting if the gun isn't ready.
				u *= 0.5f;
			}
			if (Log()) {
				GLOUTPUT(("mtr(r)=%.1f ", u));
			}
			if (u > utility[OPTION_MOVE_TO_RANGE]) {
				utility[OPTION_MOVE_TO_RANGE] = u;
				target[OPTION_MOVE_TO_RANGE] = targetID;
			}
		}

		// Consider Melee
		if (meleeWeapon) {
			if (range < 0.001f) {
				range = 0.001f;
			}
			float u = MELEE_RANGE / range;
			u *= q;
			if (Log()) {
				GLOUTPUT(("m=%.1f ", u));
			}
			if (u > utility[OPTION_MELEE]) {
				utility[OPTION_MELEE] = u;
				target[OPTION_MELEE] = targetID;
			}
		}
		if (Log()) {
			GLOUTPUT(("\n"));
		}
	}

	int index = ArrayFindMax(utility, NUM_OPTIONS, (int)0, [](int, float f) { return f; });

	// Translate to action system:
	switch (index) {
		case OPTION_NONE:
		{
			pmc->Stop();
		}
		break;

		case OPTION_MOVE_TO_RANGE:
		{
			GLASSERT(target[OPTION_MOVE_TO_RANGE]);
			int idx = enemyList2.Find(target[OPTION_MOVE_TO_RANGE]);
			GLASSERT(idx >= 0);
			if (idx >= 0) {
				Swap(&enemyList2[0], &enemyList2[idx]);	// move target to 1st slot.
				lastTargetID = enemyList2[0];
				this->Move(ToWorld2F(EnemyPos(enemyList2[0], false)), false);
			}
		}
		break;

		case OPTION_MELEE:
		{
			currentAction = AIAction::MELEE;
			int idx = enemyList2.Find(target[OPTION_MELEE]);
			GLASSERT(idx >= 0);
			if (idx >= 0) {
				Swap(&enemyList2[0], &enemyList2[idx]);
				lastTargetID = enemyList2[0];
			}
		}
		break;

		case OPTION_SHOOT:
		{
			pmc->Stop();
			currentAction = AIAction::SHOOT;
			int idx = enemyList2.Find(target[OPTION_SHOOT]);
			GLASSERT(idx >= 0);
			if (idx >= 0) {
				Swap(&enemyList2[0], &enemyList2[idx]);
				lastTargetID = enemyList2[0];
			}
		}
		break;

		default:
		GLASSERT(0);
	};

	if (Log()) {
		static const char* optionName[NUM_OPTIONS] = { "none", "mtr", "melee", "shoot" };
		GLOUTPUT(("ID=%d BTL nEne=%d (m=%d r=%d) mtr=%.2f melee=%.2f shoot=%.2f -> %s [%d]\n",
			parentChit->ID(),
			enemyList2.Size(),
			nMeleeEnemies,
			nRangedEnemies,
			utility[OPTION_MOVE_TO_RANGE], utility[OPTION_MELEE], utility[OPTION_SHOOT],
			optionName[index],
			enemyList2[0]));
		(void)optionName;
	}

	// And finally, do a swap if needed!
	// This logic is minimal: what about the other states?
	if (currentAction == AIAction::MELEE) {
		thisIC->SelectWeapon(ItemComponent::SELECT_MELEE);
	}
	else if (currentAction == AIAction::SHOOT) {
		thisIC->SelectWeapon(ItemComponent::SELECT_RANGED);
	}
}


void AIComponent::WorkQueueToTask(   )
{
	// Is there work to do?		
	Vector2I sector = ToSector(parentChit->Position());
	CoreScript* coreScript = CoreScript::GetCore(sector);
	const ChitContext* context = Context();

	if ( coreScript ) {
		WorkQueue* workQueue = GetWorkQueue();
		if (!workQueue) return;

		// Get the current job, or find a new one.
		const WorkQueue::QueueItem* item = workQueue->GetJob( parentChit->ID() );
		if ( !item ) {
			item = workQueue->Find( ToWorld2I(parentChit->Position()) );
			if ( item ) {
				workQueue->Assign( parentChit->ID(), item );
			}
		}
		if ( item ) {
			Vector2F dest = { 0, 0 };
			float cost = 0;

			bool hasPath = context->worldMap->CalcWorkPath(ToWorld2F(parentChit->Position()), item->Bounds(), &dest, &cost);

			if (hasPath) {
				if (BuildScript::IsClear(item->buildScriptID)) {
					taskList.Push(Task::MoveTask(dest));
					taskList.Push(Task::StandTask(1000));
					taskList.Push(Task::RemoveTask(item->pos));
				}
				else if (BuildScript::IsBuild(item->buildScriptID)) {
					taskList.Push(Task::MoveTask(dest));
					taskList.Push(Task::StandTask(1000));
					taskList.Push(Task::BuildTask(item->pos, item->buildScriptID, LRint(item->rotation)));
				}
				else {
					GLASSERT(0);
				}
			}
		}
	}
}


void AIComponent::DoMoraleZero(  )
{
	enum {
		STARVE,
		BLOODRAGE,
		VISION_QUEST,
		NUM_OPTIONS
	};

	GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return;

	const Needs& needs = this->GetNeeds();
	const Personality& personality = gameItem->GetPersonality();
	double food = needs.Value( Needs::FOOD );

	float options[NUM_OPTIONS] = {	food < 0.05				? 1.0f : 0.0f,		// starve
									personality.Fighting()	? 0.7f : 0.2f,		// bloodrage
									personality.Spiritual() ? 0.7f : 0.2f };	// quest

	int option = parentChit->random.Select( options, NUM_OPTIONS );
	this->GetNeedsMutable()->SetMorale( 1 );

	Vector2F pos2 = ToWorld2F(parentChit->Position());
	parentChit->SetTickNeeded();

	switch ( option ) {
	case STARVE:
		gameItem->hp = 0;
		Context()->chitBag->GetNewsHistory()->Add(NewsEvent(NewsEvent::STARVATION, pos2, parentChit->GetItemID(), 0));
		break;

	case BLOODRAGE:
		gameItem->SetChaos();
		Context()->chitBag->GetNewsHistory()->Add(NewsEvent(NewsEvent::BLOOD_RAGE, pos2, parentChit->GetItemID(), 0));
		break;

	case VISION_QUEST:
		{
			PathMoveComponent* pmc = GET_SUB_COMPONENT( parentChit, MoveComponent, PathMoveComponent );
			const SectorData* sd = 0;
			Vector2I sector = { 0, 0 };
			for( int i=0; i<16; ++i ) {
				sector.Set( parentChit->random.Rand( NUM_SECTORS ), parentChit->random.Rand( NUM_SECTORS ));
				sd = &Context()->worldMap->GetSectorData( sector );
				if ( sd->HasCore() ) {
					break;
				}
			}
			if ( sd && pmc ) {
				SectorPort sectorPort;
				sectorPort.sector = sector;
				for( int k=0; k<4; ++k ) {
					if ( sd->ports & (1<<k)) {
						sectorPort.port = 1<<k;
						break;
					}
				}
				this->Move( sectorPort, true );
				Context()->chitBag->GetNewsHistory()->Add(NewsEvent(NewsEvent::VISION_QUEST, pos2, parentChit->GetItemID(), 0));
				gameItem->SetRogue();
			}
		}
		break;
	};
}


void AIComponent::EnterNewGrid()
{
	// FIXME: this function does way too much.

	Vector2I pos2i = ToWorld2I(parentChit->Position());
	GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return;
	ItemComponent* thisIC = parentChit->GetItemComponent();
	if (!thisIC) return;
	CoreScript* coreScript = CoreScript::GetCore(ToSector(pos2i));
	Vector2I sector = ToSector(pos2i);

	// Circuits.
	// FIXME: Not at all clear where this code should be...ItemComponent? MoveComponent?
	{
		if (coreScript) {
			if (coreScript->InUse() && Team::Instance()->GetRelationship(parentChit, coreScript->ParentChit()) == ERelate::ENEMY) {
				Context()->physicsSims->GetCircuitSim(ToSector(pos2i))->TriggerDetector(pos2i);
			}
			else if (!coreScript->InUse()) {
				Context()->physicsSims->GetCircuitSim(ToSector(pos2i))->TriggerDetector(pos2i);
			}
		}
	}


	// Is there food to eat or collect?
	// Here, just collect. There is another
	// bit of logic (ThinkHungry) to eat fruit.
	if (thisIC->CanAddToInventory() 
		&& visitorIndex < 0 
		&& !gameItem->IsWorker()
		&& !parentChit->PlayerControlled()) 
	{
		FruitElixirFilter fruitFilter;
		Vector2F pos2 = ToWorld2F(parentChit->Position());
		CChitArray arr;
		Context()->chitBag->QuerySpatialHash(&arr, pos2, 0.7f, 0, &fruitFilter);

		for (int i = 0; i < arr.Size() && thisIC->CanAddToInventory(); ++i) {
			ItemComponent* ic = arr[i]->GetItemComponent();
			arr[i]->Remove(ic);
			thisIC->AddToInventory(ic);
			Context()->chitBag->DeleteChit(arr[i]);
		}
	}

	// Pick up stuff - specifically upgrade weapons in battle mode.
	if (aiMode == AIMode::BATTLE_MODE && (gameItem->flags & GameItem::AI_USES_BUILDINGS)) {
		LootFilter filter;
		CChitArray arr;
		Context()->chitBag->QuerySpatialHash(&arr, ToWorld2F(parentChit->Position()), 0.7f, 0, &filter);
		for (int i = 0; i < arr.Size() && thisIC->CanAddToInventory(); ++i) {
			const GameItem* item = arr[i]->GetItem();
			if (thisIC->IsBetterItem(item)) {
				ItemComponent* ic = arr[i]->GetItemComponent();
				arr[i]->Remove(ic);
				thisIC->AddToInventory(ic);
				Context()->chitBag->DeleteChit(arr[i]);
			}
		}
	}

	// Auto-pickup for avatar.
	if (parentChit->PlayerControlled()) {
		// ONLY check this one grid. If we drop something,
		// don't want to immediately pick back up.
		Rectangle2I b;
		b.min = b.max = pos2i;
		LootFilter filter;
		CChitArray arr;
		Context()->chitBag->QuerySpatialHash(&arr, b, 0, &filter);
		// Pick up the most valuable stuff first. We'll pull off
		// the end, so sort to increasing value.
		arr.Sort([](Chit* a, Chit* b) {
			return a->GetItem()->GetValue() < b->GetItem()->GetValue();
		});
		while (!arr.Empty() && thisIC->CanAddToInventory()) {
			Chit* loot = arr.Pop();
			ItemComponent* ic = loot->GetItemComponent();
			loot->Remove(ic);
			thisIC->AddToInventory(ic);
			Context()->chitBag->DeleteChit(loot);
		}
	}

#ifdef ALTERA_MICRO
	// No domain takeover in mini mode.
#else
	// Domain Takeover of neutral domains.
	if (gameItem->MOB() == ISC::denizen
		&& coreScript
		&& !coreScript->InUse()
		&& ToWorld2I(coreScript->ParentChit()->Position()) == pos2i)
	{
		// Wandering denizens can takeover a neutral core, OR
		// A squad can takeover a neutral core
		CoreScript* myCore = CoreScript::GetCoreFromTeam(parentChit->Team());

		if (Team::IsRogue(parentChit->Team()) && Context()->chitBag->census.NumCoresInUse() < TYPICAL_AI_DOMAINS) {
			// Need some team. And some cash.
			Rectangle2I inner = InnerSectorBounds(sector);
			CChitArray arr;
			MOBKeyFilter filter;
			filter.value = ISC::denizen;
			Context()->chitBag->QuerySpatialHash(&arr, ToWorld2F(inner), parentChit, &filter);

			arr.Filter(parentChit, [](Chit* parent, Chit* ele) {
				return Team::IsRogue(ele->Team()) && (Team::Instance()->GetRelationship(ele, parent) != ERelate::ENEMY);
			});
			if (arr.Size() > 3) {
				Context()->chitBag->TakeOverCore(sector, parentChit);
			}
		}
	}
#endif

	// Check for morale-changing items (tombstones, at this writing.)
	if (aiMode == AIMode::NORMAL_MODE) {
		if (gameItem->flags & GameItem::HAS_NEEDS) {
			RenderComponent* rc = parentChit->GetRenderComponent();
			Vector2F center = ToWorld2F(parentChit->Position());	// center of the grid.
			CChitArray arr;
			//const ChitContext* context = this->Context();
			LumosChitBag* chitBag = this->Context()->chitBag;

			// For now, just tombstones.
			ItemNameFilter filter(ISC::tombstone);
			chitBag->QuerySpatialHash(&arr, center, 1.1f, parentChit, &filter);
			for (int i = 0; i < arr.Size(); ++i) {
				Chit* chit = arr[i];
				GameItem* item = chit->GetItem();
				GLASSERT(item);
				IString mob = item->keyValues.GetIString("tomb_mob");
				GLASSERT(!mob.empty());
				int team = -1;
				item->keyValues.Get("tomb_team", &team);
				GLASSERT(team >= 0);

				double boost = 0;
				if (mob == ISC::lesser)			boost = 0.05;
				else if (mob == ISC::greater)	boost = 0.20;
				else if (mob == ISC::denizen)	boost = 0.10;

				ERelate relate = Team::Instance()->GetRelationship(parentChit->Team(), team);
				if (relate == ERelate::ENEMY) {
					GetNeedsMutable()->AddMorale(boost);
					if (rc) {
						rc->AddDeco("happy", STD_DECO);
					}
				}
				else if (relate == ERelate::FRIEND) {
					GetNeedsMutable()->AddMorale(-boost);
					if (rc) {
						rc->AddDeco("sad", STD_DECO);
					}
				}
			}
		}
	}

	// Greaters should sector herd if they wander over a core.
	if (gameItem->MOB() == ISC::greater && aiMode == AIMode::NORMAL_MODE && coreScript && !coreScript->InUse() && pos2i == ToWorld2I(coreScript->ParentChit()->Position())) {
		SectorHerd(false);
	}
}


bool AIComponent::AtWaypoint()
{
	CoreScript* cs = CoreScript::GetCoreFromTeam(parentChit->Team());
	if (!cs) 
		return false;

	int squad = cs->SquadID(parentChit->ID());
	if ( squad < 0) 
		return false;

	Vector2I waypoint = cs->GetWaypoint(squad);
	if (waypoint.IsZero()) 
		return false;

	Vector2F dest2 = ToWorld2F(waypoint);

	static const float DEST_RANGE = 1.5f;
	Vector2F pos2 = ToWorld2F(ParentChit()->Position());
	float len = (dest2 - pos2).Length();
	return len < DEST_RANGE;
}


bool AIComponent::ThinkWaypoints()
{
	if (parentChit->PlayerControlled()) return false;

	CoreScript* cs = CoreScript::GetCoreFromTeam(parentChit->Team());
	if (!cs) return false;
	
	int squad = cs->SquadID(parentChit->ID());
	if ( squad < 0) 
		return false;

	Vector2I waypoint = cs->GetWaypoint(squad);
	if (waypoint.IsZero()) 
		return false;

	PathMoveComponent* pmc = GET_SUB_COMPONENT(ParentChit(), MoveComponent, PathMoveComponent);
	if (!pmc) return false;

	if (AtWaypoint()) {
		bool allHere = true;
		pmc->Stop();
		CChitArray squaddies;
		cs->Squaddies(squad, &squaddies);

		for (int i = 0; i < squaddies.Size(); ++i) {
			Chit* traveller = squaddies[i];
			if (traveller && traveller->GetAIComponent() && !traveller->PlayerControlled() && !traveller->GetAIComponent()->Rampaging()) {
				if (!traveller->GetAIComponent()->AtWaypoint()) {
					allHere = false;
					break;
				}
			}
		}
		if (allHere) {
			GLOUTPUT(("Waypoint All Here. squad=%d\n", squad));
			cs->PopWaypoint(squad);
		}
		return true;
	}

	Vector2F dest2 = ToWorld2F(waypoint);
	Vector2I destSector = ToSector(waypoint);
	Vector2I currentSector = ToSector(parentChit->Position());

	if (destSector == currentSector) {
		this->Move(dest2, false);
	}
	else {
		Vector2F pos = ToWorld2F(parentChit->Position());
		SectorPort sp = Context()->worldMap->NearestPort(dest2, &pos);
		this->Move(sp, false);
	}
	return true;
}




int AIComponent::DoTick(U32 deltaTime)
{
	PROFILE_FUNC();

	// If we are in some action, do nothing and return.
	if (parentChit->GetRenderComponent()
		&& !parentChit->GetRenderComponent()->AnimationReady())
	{
		return 0;
	}

	AIAction oldAction = currentAction;

	GameItem* gameItem = parentChit->GetItem();
	if (!gameItem) return VERY_LONG_TICK;

	// Clean up the enemy/friend lists so they are valid for this call stack.
	ProcessFriendEnemyLists(feTicker.Delta(deltaTime) != 0);

	// High level mode switch, in/out of battle?
	if (focus != FOCUS_MOVE &&  !taskList.UsingBuilding()) {
		CoreScript* cs = CoreScript::GetCore(ToSector(parentChit->Position()));
		// Workers only go to battle if the population is low. (Cuts down on continuous worked destruction.)
		bool goesToBattle = !gameItem->IsWorker() || (cs && cs->Citizens(0) <= 4);

		if (aiMode != AIMode::BATTLE_MODE
			&& goesToBattle
			&& enemyList2.Size())
		{
			aiMode = AIMode::BATTLE_MODE;
			currentAction = AIAction::NO_ACTION;
			taskList.Clear();

			if (Log()) {
				GLOUTPUT(("AI ID=%d Mode to Battle\n", parentChit->ID()));
			}

			if (parentChit->GetRenderComponent()) {
				parentChit->GetRenderComponent()->AddDeco("attention", STD_DECO);
			}
			for (int i = 0; i < friendList2.Size(); ++i) {
				Chit* fr = Context()->chitBag->GetChit(friendList2[i]);
				if (fr && fr->GetAIComponent()) {
					fr->GetAIComponent()->MakeAware(enemyList2.Mem(), enemyList2.Size());
				}
			}
		}
		else if (aiMode == AIMode::BATTLE_MODE && enemyList2.Empty()) {
			aiMode = AIMode::NORMAL_MODE;
			currentAction = AIAction::NO_ACTION;
			if (Log()) {
				GLOUTPUT(("AI ID=%d Mode to Normal\n", parentChit->ID()));
			}
		}
	}


	if (lastGrid != ToWorld2I(parentChit->Position())) {
		lastGrid = ToWorld2I(parentChit->Position());
		EnterNewGrid();
	}

	if (focus == FOCUS_MOVE) {
		return 0;
	}

	// This is complex; HAS_NEEDS and USES_BUILDINGS aren't orthogonal. See aineeds.h
	// for an explanation.
	// FIXME: remove "lower difficulty" from needs.DoTick()
	if (gameItem->flags & (GameItem::HAS_NEEDS | GameItem::AI_USES_BUILDINGS)) {
		if (needsTicker.Delta(deltaTime)) {
			CoreScript* cs = CoreScript::GetCore(ToSector(parentChit->Position()));
			CoreScript* homeCore = CoreScript::GetCoreFromTeam(parentChit->Team());
			bool atHomeCore = cs && (cs == homeCore);
			static const double LOW_MORALE = 0.25;

			if (parentChit->PlayerControlled())
				needs.SetFull();
			if (!(gameItem->flags & GameItem::HAS_NEEDS))
				needs.SetMorale(1);

			// Squaddies, on missions, don't have needs. Keeps them 
			// from running off or falling apart in the middle.
			// But for the other denizens:
			if (atHomeCore) {
				needs.DoTick(needsTicker.Period(), aiMode == AIMode::BATTLE_MODE, false, &gameItem->GetPersonality());
				if (needs.Morale() == 0)
					DoMoraleZero();
			}
			else if (homeCore) {
				if (!homeCore->IsSquaddieOnMission(parentChit->ID(), 0)) {
					needs.DoTravelTick(deltaTime);
				}
				if (needs.Morale() < LOW_MORALE) {
					bool okay = TravelHome(true);
					if (!okay) needs.SetMorale(1.0);
				}
			}
		}
	}

	// Is there work to do?
	if (aiMode == AIMode::NORMAL_MODE
		&& gameItem->IsWorker()
		&& taskList.Empty())
	{
		WorkQueueToTask();
	}

	int RETHINK = 2000;
	if (aiMode == AIMode::BATTLE_MODE) {
		RETHINK = 1000;	// so we aren't runnig past targets and such.
		// And also check for losing our target.
		if (enemyList2.Size() && (lastTargetID != enemyList2[0])) {
			if (Log()) {
				GLOUTPUT(("AI BTL retarget.\n"));
			}
			rethink += RETHINK;
		}
	}

	if (aiMode == AIMode::NORMAL_MODE && !taskList.Empty()) {
		taskList.DoTasks(parentChit, deltaTime);	
		if (taskList.Empty()) {
			rethink = 0;
		}
	}
	else if ((currentAction == AIAction::NO_ACTION) || (rethink > RETHINK)) {
		Think();
		rethink = 0;
	}

	// Are we doing something? Then do that; if not, look for
	// something else to do.
	switch (currentAction) {
		case AIAction::MOVE:
		DoMove();
		break;

		case AIAction::MELEE:
		DoMelee();
		break;
		
		case AIAction::SHOOT:
		DoShoot();
		break;
		
		case AIAction::NO_ACTION:
		break;

		default:
		GLASSERT(0);
		currentAction = AIAction::NO_ACTION;
		break;
	}

	if (Log() && (currentAction != oldAction)) {
		GLOUTPUT(("AI ID=%d mode=%s action=%s\n", parentChit->ID(), MODE_NAMES[int(aiMode)], ACTION_NAMES[int(currentAction)]));
	}

	// Without this rethink ticker melee fighters run past
	// each other and all kinds of other silliness.
	if (aiMode == AIMode::BATTLE_MODE) {
		rethink += deltaTime;
	}
	return (currentAction != AIAction::NO_ACTION) ? GetThinkTime() : 0;
}


void AIComponent::DebugStr( grinliz::GLString* str )
{
	str->AppendFormat( "[AI] %s %s nF=%d nE=%d dB=%d ", MODE_NAMES[int(aiMode)], ACTION_NAMES[int(currentAction)], friendList2.Size(), enemyList2.Size(), destinationBlocked );
}


void AIComponent::OnChitMsg(Chit* chit, const ChitMsg& msg)
{
	/* Remember this is a *synchronous* function.
	   Not safe to reach back and change other components.
	   */

	Vector2I mapPos = ToWorld2I(parentChit->Position());

	switch (msg.ID()) {
		case ChitMsg::CHIT_DAMAGE:
		{
			// If something hits us, make sure we notice.
			const ChitDamageInfo* info = (const ChitDamageInfo*)msg.Ptr();
			GLASSERT(info);
			this->MakeAware(&info->originID, 1);
		}
		break;

		case ChitMsg::PATHMOVE_DESTINATION_REACHED:
		destinationBlocked = 0;
		focus = 0;
		currentAction = AIAction::NO_ACTION;
		parentChit->SetTickNeeded();
		break;

		case ChitMsg::PATHMOVE_DESTINATION_BLOCKED:
		destinationBlocked++;

		if (aiMode == AIMode::BATTLE_MODE) {
			// We are stuck in battle - attack something! Keeps everyone from just
			// standing around wishing they could fight. Try to target anything
			// adjacent. If that fails, walk towards the core.
			if (!TargetAdjacent(mapPos, true)) {
				const WorldGrid& wg = Context()->worldMap->GetWorldGrid(mapPos);
				Vector2I next = mapPos + wg.Path(0);
				this->Move(ToWorld2F(next), true);
			}
		}
		else {
			focus = 0;
			currentAction = AIAction::NO_ACTION;
			parentChit->SetTickNeeded();

			// Generally not what we expected.
			// Do a re-think.get
			// Never expect move troubles in rampage mode.
			if (aiMode != AIMode::BATTLE_MODE) {
				aiMode = AIMode::NORMAL_MODE;
			}
			taskList.Clear();
			{
				PathMoveComponent* pmc = GET_SUB_COMPONENT(parentChit, MoveComponent, PathMoveComponent);
				if (pmc) {
					pmc->Clear();	// make sure to clear the path and the queued path
				}
			}
		}
		break;

		case ChitMsg::PATHMOVE_TO_GRIDMOVE:
		{
			//LumosChitBag* chitBag = Context()->chitBag;
			const SectorPort* sectorPort = (const SectorPort*)msg.Ptr();
			Vector2I sector = sectorPort->sector;
			CoreScript* cs = CoreScript::GetCore(sector);

			// Pulled out the old "summoning" system, but left in 
			// the cool message for Greaters attracted to tech.
			if (parentChit->GetItem()
				&& parentChit->GetItem()->MOB() == ISC::greater
				&& cs && (cs->GetTech() >= TECH_ATTRACTS_GREATER))
			{
				Vector2I target = ToWorld2I(cs->ParentChit()->Position());
				Context()->chitBag->GetNewsHistory()->Add(NewsEvent(NewsEvent::GREATER_SUMMON_TECH, ToWorld2F(target), parentChit->GetItemID(), 0));
			}
		}
		break;


		case ChitMsg::CHIT_SECTOR_HERD:
		if ((aiMode == AIMode::NORMAL_MODE)
			&& parentChit->GetItem()
			&& !parentChit->GetItem()->IsWorker())
		{
			// Read our destination port information:
			const SectorPort* sectorPort = (const SectorPort*)msg.Ptr();
			this->Move(*sectorPort, msg.Data() ? true : false);
		}
		break;

		case ChitMsg::WORKQUEUE_UPDATE:
		if (aiMode == AIMode::NORMAL_MODE) {
			currentAction = AIAction::NO_ACTION;
			parentChit->SetTickNeeded();
		}
		break;

		default:
		super::OnChitMsg(chit, msg);
		break;
	}
}


