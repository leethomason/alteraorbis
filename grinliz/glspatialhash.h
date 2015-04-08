#ifndef GRINLIZ_SPATIAL_HASH_INCLUDED
#define GRINLIZ_SPATIAL_HASH_INCLUDED

#include "glvector.h"
#include "glcontainer.h"

namespace grinliz {

template<class V>
class SpatialHash
{
public:
	SpatialHash() {
	}

	~SpatialHash() {
		delete[] buckets;
	}

	void Add(const Vector2I& pos, const V& v) {
		EnsureCap();
		U32 h = Hash(pos) & (nBuckets - 1);
		const U32 hStart = h;
		while (true) {
			int state = buckets[h].state;
			if (state != IN_USE) {
				if (state == DELETED) {
					GLASSERT(nDeleted);
					--nDeleted;
				}
				buckets[h].pos = pos;
				buckets[h].state = IN_USE;
				buckets[h].value = v;
				++nItems;
				break;
			}
			++h;
			if (h == nBuckets) h = 0;
			GLASSERT(hStart != h);
		}
	}

	void Remove(const Vector2I& pos, const V& v) {
		U32 h = Hash(pos) & (nBuckets - 1);
		while (buckets[h].state != UNUSED) {
			if (   buckets[h].state == IN_USE
				&& buckets[h].pos == pos
				&& buckets[h].value == v)
			{
				buckets[h].state = DELETED;
				--nItems;
				++nDeleted;
				return;
			}
			++h;
			if (h == nBuckets) h = 0;
		}
		GLASSERT(false); // not found
	}

	int Query(const Vector2I& pos, CDynArray<V>* arr) {
		nProbes++;
		arr->Clear();
		U32 h = Hash(pos) & (nBuckets - 1);
		while (buckets[h].state != UNUSED) {
			if (buckets[h].state == IN_USE && buckets[h].pos == pos) {
				arr->Push(buckets[h].value);
			}
			else {
				nSteps++;
			}
			++h;
			if (h == nBuckets) h = 0;
		}
		return arr->Size();
	}

	bool Empty() const { return nItems == 0; }
	int NumProbes() const { return nProbes; }
	int NumSteps() const { return nSteps; }
	int NumAllocated() const { return nBuckets; }
	// Lower is better, 0 is best.
	float Efficiency() const { return float(nSteps) / float(nProbes); }
	float Density() const { return float(nItems + nDeleted) / float(nBuckets); }

private:

	U32 Hash(const Vector2I& v) {
		GLASSERT(v.x >= 0 && v.y < 65536);		// will still work, but less efficient.
		GLASSERT(v.y >= 0 && v.y < 65536);
		U32 hash = (v.x * 58111) ^ (v.y * 47269);	// spread out over full range.
		//U32 hash = (v.x * 4073) ^ (v.y * 10133);	// spread out over full range.
		//hash = hash ^ (hash >> 16);					// compress the top bits into the bottom (again, assume 2^16)
		//hash = hash % nBuckets;
		return hash;
	}

	void EnsureCap() {
		if ( (nItems + nDeleted + 1) >= nBuckets/2 ) {
			GLASSERT( !reallocating );
			reallocating = true;

			int n = CeilPowerOf2( nItems*3 );
			if (n < 2048) n = 2048;

			Bucket* oldBuckets = buckets;
			int oldN = nBuckets;
			nBuckets = n;
			buckets = new Bucket[nBuckets];
			memset(buckets, 0, sizeof(Bucket));

			nItems = 0;
			nDeleted = 0;
			for (int i = 0; i < oldN; ++i) {
				if (oldBuckets[i].state == IN_USE) {
					Add(oldBuckets[i].pos, oldBuckets[i].value);
				}
			}

			reallocating = false;
			delete[] oldBuckets;
		}
	}

	enum {
		UNUSED,
		IN_USE,
		DELETED
	};
	struct Bucket {
		Bucket() { pos.Zero(); state = UNUSED; }

		Vector2I pos;
		char state;
		V value;
	};

	int nBuckets = 0;		// allocated
	int nItems = 0;
	int nDeleted = 0;
	int nProbes = 0;
	int nSteps = 0;
	bool reallocating = false;
	Bucket *buckets = nullptr;
};

}; // namespace grinliz
#endif // GRINLIZ_SPATIAL_HASH_INCLUDED
