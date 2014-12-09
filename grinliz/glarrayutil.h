#ifndef GRINLIZ_ARRAY_UTIL_H
#define GRINLIZ_ARRAY_UTIL_H

#include "glcontainer.h"

// 'auto' in a #define. Not sure if C11 is going to give me trouble.
// Transforms a CDynArray or CArary (in place)
// so that each element meets the criteria EVAL
// EVAL is called with the element 'ele'
// NOTE: re-orders the array
//
#define GL_ARRAY_FILTER( arr, EVAL ) {		\
	int _k_ = 0;						\
	while (_k_ < arr.Size()) {			\
		auto& ele = arr[_k_];			\
		if (EVAL) {						\
			++_k_;						\
		}								\
		else {							\
			arr.SwapRemove(_k_);		\
		}								\
	}									\
}


// Transforms a CDynArray or CArary (in place)
// so that each element meets the criteria EVAL
// EVAL is called with the element 'ele'
// Maintains array order.
//
#define GL_ARRAY_FILTER_ORDERED( arr, EVAL ) {		\
	int _k_ = 0;						\
	while (_k_ < arr.Size()) {			\
		auto& ele = arr[_k_];			\
		if (f) {						\
			++_k_;						\
		}								\
		else {							\
			arr.Remove(_k_);			\
		}								\
	}									\
}


// Sort an array (c-array, CDynArray, CArray)
// using the EVAL expression to evaluate
// element 'ele'. EVAL must evaluate to a 
// double.
#define GL_ARRAY_SORT_EXPR(mem, size, EVAL) {	\
	for (int _gap = size;;) {					\
		_gap = CombSortGap(_gap);					\
		bool _swapped = false;					\
		const int end = size - _gap;				\
		for (int i = 0; i < end; i++) {			\
			int j = i + _gap;					\
			double valI=0, valJ = 0;			\
			{ auto& ele = mem[i]; valI = EVAL; }	\
			{ auto& ele = mem[j]; valJ = EVAL; }	\
			if ( valJ < valI ) {				\
				Swap(mem+i, mem+j);				\
				_swapped = true;					\
			}									\
		}										\
		if (_gap == 1 && !_swapped) {				\
			break;								\
		}										\
	}											\
}


/*
	Search an array and find the index
	with the greatest EVAL. Works for
	1 or more outputs. (So it can find
	the 3 highest, for example.) If
	no answer found -1 is returned.
*/
#define GL_ARRAY_FIND_MAX(mem, size, EVAL, outIndexPtr, outN) {	\
	double bestScore[outN] = { 0 };							\
	for( int _k=0; _k < outN; ++_k) {						\
		(outIndexPtr)[_k] = -1;								\
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
