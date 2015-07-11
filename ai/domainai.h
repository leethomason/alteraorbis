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

#ifndef DOMAINAI_COMPONENT_INCLUDED
#define DOMAINAI_COMPONENT_INCLUDED

#include "../xegame/component.h"
#include "../xegame/chitbag.h"
#include "../xegame/cticker.h"

class CoreScript;
class WorkQueue;
class RoadAI;

class DomainAI : public Component, public IChitListener
{
	typedef Component super;

public:
	DomainAI();
	~DomainAI();

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();
	virtual void Serialize(XStream* xs);

	virtual const char* Name() const { return "DomainAI"; }
	virtual void OnChitMsg(Chit* chit, const ChitMsg& msg);
	virtual int DoTick(U32 delta);

	// subclasses implement to place build orders.
	virtual void DoBuild();
	// If using DoBuild(), a subclass can still override
	// what happens at special stages.
	enum {
		AFTER_ROADS,
		AFTER_BAR,
	};
	virtual bool DoSpecialBuild(int stage)	{ return false; }

	static DomainAI* Factory(int team);

protected:
	bool Preamble(grinliz::Vector2I* sector, CoreScript** cs, WorkQueue** wq, int *pave);
	bool CanBuild(const grinliz::Rectangle2I& r);

	bool ClearDisconnected();
	bool ClearRoadsAndPorches();
	bool BuyWorkers();
	bool BuildRoad();
	bool BuildRoad(int road, int distance);
	bool BuildPlaza();
	bool BuildBuilding(int id);
	bool BuildFarm();

	float CalcFarmEfficiency(const grinliz::Vector2I& sector);

	// The 4 default roads are set up by the DomainAI super class.
	// Additional roads should be set up by the sub-class
	// in OnAdd. Likewise, templeCap is defaulted, but should
	// be set in the subclass.
	RoadAI*	roads;
	int templeCap;

private:
	enum { MAX_ROADS = 8};
	CTicker ticker;			// build logic ticker.
	int buildDistance[MAX_ROADS];	// records how far the road is built, so we can check to keep it clear
};


class DeityDomainAI : public DomainAI
{
	typedef DomainAI super;

public:
	DeityDomainAI()		{}
	~DeityDomainAI()	{}

	virtual const char* Name() const { return "DeityDomainAI"; }
	virtual void Serialize(XStream* xs);

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();
	virtual int DoTick(U32 delta);

	virtual void DoBuild();

private:
};


class ForgeDomainAI : public DomainAI
{
	typedef DomainAI super;

public:
	ForgeDomainAI();
	~ForgeDomainAI();

	virtual const char* Name() const { return "ForgeDomainAI"; }
	virtual void Serialize(XStream* xs);

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();
	virtual int DoTick(U32 delta);

	virtual void DoBuild();
private:

	CTicker forgeTicker;	// every tick builds something
};


class GobDomainAI : public DomainAI
{
	typedef DomainAI super;

public:
	GobDomainAI();
	~GobDomainAI();

	virtual const char* Name() const { return "GobDomainAI"; }
	virtual void Serialize(XStream* xs);

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();

	// Uses default.
	//virtual void DoBuild();
	virtual bool DoSpecialBuild(int stage);

private:
};

class KamakiriDomainAI : public DomainAI
{
	typedef DomainAI super;
public:
	KamakiriDomainAI();
	~KamakiriDomainAI();

	virtual const char* Name() const { return "KamakiriDomainAI"; }
	virtual void Serialize(XStream* xs);

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();

	//virtual void DoBuild();
	virtual bool DoSpecialBuild(int stage);
};

class HumanDomainAI : public DomainAI
{
	typedef DomainAI super;
public:
	HumanDomainAI();
	~HumanDomainAI();

	virtual const char* Name() const { return "HumanDomainAI"; }
	virtual void Serialize(XStream* xs);

	virtual void OnAdd(Chit* chit, bool initialize);
	virtual void OnRemove();

	//virtual void DoBuild();
};

#endif // DOMAINAI_COMPONENT_INCLUDED

