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
#define GL_FIND_LEAST(mem, size, EVAL, outPtr, outN) {	\
	double _bestScore[outN];
	double _bestIndex
	for(int _i=0; _i<size; ++_i ) {
	
	}
}
*/

#endif //  GRINLIZ_ARRAY_UTIL_H
