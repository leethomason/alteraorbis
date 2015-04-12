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
				++nDeleted;
				buckets[h].state = DELETED;

				while (h > 0 && h < U32(nBuckets - 1) 
					   && buckets[h].state == DELETED 
					   && buckets[h + 1].state == UNUSED) 
				{
					buckets[h].state = UNUSED;
					--h;
					--nDeleted;
				}
				--nItems;
				return;
			}
			++h;
			if (h == nBuckets) h = 0;
		}
		GLASSERT(false); // not found
	}

	int Query(const Vector2I& pos, CDynArray<V>* arr) {
		nProbes++;
		int hits = 0;
		U32 h = Hash(pos) & (nBuckets - 1);
		while (buckets[h].state != UNUSED) {
			if (buckets[h].state == IN_USE && buckets[h].pos == pos) {
				arr->Push(buckets[h].value);
				++hits;
			}
			else {
				nSteps++;
			}
			++h;
			if (h == nBuckets) h = 0;
		}
		return hits;
	}

	bool Empty() const { return nItems == 0; }
	int NumAllocated() const { return nBuckets; }
	int NumItems() const { return nItems; }

	int NumProbes() const { return nProbes; }
	int NumSteps() const { return nSteps; }
	int NumAlloc() const { return nAlloc;  }
	// Lower is better, 0 is best.
	float Efficiency() const { return float(nSteps) / float(nProbes); }
	float Density() const { return float(nItems + nDeleted) / float(nBuckets); }
	void ClearMetrics() { nProbes = nSteps = nAlloc = 0; }

private:

	U32 Hash(const Vector2I& v) {
#if 0
		// Base
		GLASSERT(v.x >= 0 && v.y < 65536);		// will still work, but less efficient.
		GLASSERT(v.y >= 0 && v.y < 65536);
		U32 hash = (v.x * 58111) ^ (v.y * 47269);	// spread out over full range.
		return hash;
#endif
#if 1
		// Shifter2
		U32 x = v.x + v.y * 47269;
		x += ( x << 10u );
		x ^= ( x >>  6u );
		x += ( x <<  3u );
		x ^= ( x >> 11u );
		return x;
#endif
	}


	void EnsureCap() {
		if ( (nItems + nDeleted + 1) >= nBuckets/2 ) {
			GLASSERT( !reallocating );
			reallocating = true;
			++nAlloc;

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
	int nAlloc = 0;
	bool reallocating = false;
	Bucket *buckets = nullptr;
};

}; // namespace grinliz
#endif // GRINLIZ_SPATIAL_HASH_INCLUDED
