/*
Copyright (c) 2000-2012 Lee Thomason (www.grinninglizard.com)
Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include "glcontainer.h"
#include "glrandom.h"

using namespace grinliz;

template<typename T>
bool InOrder(const T* mem, int N)
{
	for (int i = 0; i < N - 1; ++i) {
		bool inOrder = mem[i] < mem[i + 1];
		GLASSERT(inOrder);
		if (!inOrder) return false;
	}
	return true;
}

void grinliz::TestContainers()
{
	Random random;
	// Global functions.
	{
		static const int N = 10;
		int arr[N] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		random.ShuffleArray(arr, N);

		grinliz::Sort(arr, N);
		GLASSERT(InOrder(arr, N));
		random.ShuffleArray(arr, N);

		grinliz::Sort(arr, N, [](const int& a, const int& b) {
			return a < b;
		});
		GLASSERT(InOrder(arr, N));
		random.ShuffleArray(arr, N);

		struct Context {
			int origin = -1;
			bool Less(const int&a, const int& b) const {
				return (origin + a) < (origin + b);
			}
		};
		Context context;
		grinliz::SortContext(arr, N, context);
		GLASSERT(InOrder(arr, N));
	}
	// CArray
	{
		CArray<int, 10> arr;
		for (int i = 0; i < 10; ++i) arr.Push(i);
		random.ShuffleArray(arr.Mem(), arr.Size());

		arr.Sort();
		GLASSERT(InOrder(arr.Mem(), arr.Size()));

		random.ShuffleArray(arr.Mem(), arr.Size());
		arr.Sort([](const int& a, const int& b) {
			return a < b;
		});
		GLASSERT(InOrder(arr.Mem(), arr.Size()));
	}
}


