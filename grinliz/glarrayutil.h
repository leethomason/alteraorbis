#ifndef GRINLIZ_ARRAY_UTIL_H
#define GRINLIZ_ARRAY_UTIL_H

#include "glcontainer.h"
#include <float.h>

/*
	Search an array (linear) and find
	the first match for EVAL. IF
	no answer found -1 is written.
*/
#define GL_ARRAY_FIND(mem, size, EVAL, outIndexPtr) {	\
	*outIndexPtr = -1;									\
	for (int index = 0; index < size; index++ ) {	\
		auto& ele = mem[index];							\
		if (EVAL) {										\
			*outIndexPtr = index;						\
			break;										\
		}												\
	}													\
}

/*
	Search an array and find the index
	with the greatest EVAL. Works for
	1 or more outputs. (So it can find
	the 3 highest, for example.) If
	no answer found -1 is written.
*/
#define GL_ARRAY_FIND_MAX(mem, size, EVAL, outIndexPtr, outN) {	\
	double bestScore[outN] = { 0 };							\
	for( int _k=0; _k < outN; ++_k) {						\
		(outIndexPtr)[_k] = -1;								\
		bestScore[_k] = -DBL_MAX;							\
	}														\
	for (int _k = 0; _k < size; ++_k) {						\
		auto &ele = mem[_k];								\
		double score = EVAL;								\
		for (int a = 0; a < outN; ++a) {					\
			if (score > bestScore[a]) {						\
				for (int b = outN - 1; b>a; --b) {			\
					bestScore[b] = bestScore[b - 1];		\
					(outIndexPtr)[b] = (outIndexPtr)[b - 1];	\
				}											\
				bestScore[a] = score;						\
				(outIndexPtr)[a] = _k;						\
				break;										\
			}												\
		}													\
	}														\
}

#endif //  GRINLIZ_ARRAY_UTIL_H
