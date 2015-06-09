#include "../grinliz/gldebug.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glgeometry.h"
#include "../grinliz/glspatialhash.h"
#include "../grinliz/glthreadpool.h"

#include "../game/news.h"

#include <chrono>
#include <thread>

using namespace grinliz;

int TFunc(void* v, void*, void*, void*) {

	int i = reinterpret_cast<int>(v);
	std::chrono::milliseconds ms(i%10 + 1);
	std::this_thread::sleep_for(ms);
	//printf("ThreadFunc %d run.\n", i);
	return i;
}

void TestThreadPool()
{
	CDynArray<int> result;
	{
		// Empty:
		ThreadPool pool;
		pool.Wait(0);
		printf("Empty pool pass.\n");
	}
	{
		int N = 8;
		ThreadPool pool;
		for (int i = 0; i < N; ++i) {
			pool.Add(TFunc, (void*)i);
		}
		pool.Wait(&result);
		result.Sort();
		for (int i = 0; i < N; ++i) {
			GLASSERT(result[i] == i);
		}
		printf("Basic test pass.\n");
	}
	{
		for (int pass = 0; pass < 2; ++pass) {
			int N = 100;
			ThreadPool pool;
			for (int i = 0; i < N; ++i) {
				pool.Add(TFunc, (void*)i);
			}
			pool.Wait(&result);
			result.Sort();
			for (int i = 0; i < N; ++i) {
				GLASSERT(result[i] == i);
			}
			printf("Serial stress %d pass.\n", pass);
		}
	}
	{
		static const int P = 2;
		ThreadPool pool[P];
		int N = 100;
		for (int i = 0; i < N; ++i) {
			for (int pass = 0; pass < P; ++pass) {
				pool[pass].Add(TFunc, (void*)i);
			}
		}
		for (int pass = 0; pass < P; ++pass) {
			pool[pass].Wait(&result);
			result.Sort();
			for (int i = 0; i < N; ++i) {
				GLASSERT(result[i] == i);
			}
		}
		printf("Parallel stress pass.\n");
	}
}

int tc_order = 0;
std::mutex tc_mutex;
std::condition_variable tc_condition;

void TestConditions()
{
	std::thread pool[3];
	for (int i = 0; i < 3; ++i) {
		pool[i] = std::thread([i] {
			{
				std::unique_lock<std::mutex> lock(tc_mutex);
				tc_condition.wait(lock, [i] {
					printf("Order==%d\n", i);
					return tc_order == i;
				});
				tc_order = i + 1;
			}
			tc_condition.notify_one();
		});
	}
	printf("TestCondition Done!\n");
	for (int i = 0; i < 3; ++i) {
		pool[i].join();
	}
}

void TestSpatialHash()
{
	SpatialHash<int> spatial;
	CDynArray<int> query;

	Vector2I v = { 0, 0 };

	spatial.Add(v, 0);
	spatial.Query(v, &query);
	GLASSERT(query.Size() == 1 && query[0] == 0);
	spatial.Remove(v, 0);
	GLASSERT(spatial.Query(v, &query) == 0);
	GLASSERT(spatial.Empty());

	static int NUM = 2000;
	static int MULT = 7;
	static int MAP = 1024;

	for (int i = 0; i < NUM; ++i) {
		for (int j = 0; j < 4; ++j) {
			Vector2I pos = { (i*MULT) / MAP, (i*MULT) % MAP };
			spatial.Add(pos, j);
		}
	}
	for (int i = 0; i < NUM; ++i) {
		Vector2I pos = { (i*MULT) / MAP, (i*MULT) % MAP };
		spatial.Remove(pos, i%4);
	}
	for (int i = 0; i < NUM; ++i) {
		Vector2I pos = { (i*MULT) / MAP, (i*MULT) % MAP };
		query.Clear();
		spatial.Query(pos, &query);
		GLASSERT(query.Size() == 3);
		for (int j = 0; j < query.Size(); ++j) {
			GLASSERT(query[j] >= 0 && query[j] < 4 && query[j] != (i % 4));
		}
		for (int j = 0; j < query.Size(); ++j) {
			spatial.Remove(pos, query[j]);
		}
	}
	GLASSERT(spatial.Empty());

	printf("Spatial Hash test. nAllocated=%d nProbes=%d nSteps=%d eff=%.3f density=%.3f\n",
		   spatial.NumAllocated(), spatial.NumProbes(), spatial.NumSteps(), spatial.Efficiency(), spatial.Density());
}

int main(int argc, const char* argv[])
{
	Matrix4::Test();
	Quaternion::Test();
	TestSpatialHash();
	TestConditions();
	TestThreadPool();
	return 0;
}