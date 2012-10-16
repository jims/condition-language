#include "condition_language.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

unsigned hash (const char * key, unsigned len)
{
	const unsigned  m = 0x5bd1e995;
	const int r = 24;
	unsigned h = 0 ^ len;
	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4) {
		unsigned k = *(unsigned *)data;
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		len -= 4;
	}

	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}

struct Defines {
	const char** defines;
	unsigned n_defines;
};

namespace instrinsic {
	bool defined(IdString32* params, unsigned n_params, void* ud) {
		Defines& defs = *(Defines*)ud;
		for (unsigned i = 0; i != defs.n_defines; ++i) {
			if (params[0] == hash(defs.defines[i], strlen(defs.defines[i])))
				return true;
		}
		return false;
	}

	bool equal(IdString32* params, unsigned n_params, void* ud) {
		return params[0] == params[1];
	}
}

void test_single_intrinsic() {
	const char* source = "equal(A, A) && !!(equal(B, B) || equal(B, C))";
	condition_language::Intrinsic intrin[] = {
		{ hash("equal", 5), instrinsic::equal },
	};
	unsigned char stack[64] = {0};
	condition_language::Result r = condition_language::run(source, stack, sizeof(stack), intrin, 1, hash, 0);
	assert(r.exit_status == condition_language::SUCCESS && r.value == true);
}

void test_several_intrinsics() {
	const char* source = "equal(A, A) && !!(equal(B, B) || equal(B, C)) && (!defined(DEFINED) || defined(AVALUE))";
	condition_language::Intrinsic intrin[] = {
		{ hash("defined", 7), instrinsic::defined },
		{ hash("equal", 5), instrinsic::equal },
	};
	const char* defines[] = { "DEFINED", "AVALUE" };
	Defines macros = { defines, 2 };
	unsigned char stack[64] = {0};
	condition_language::Result r = condition_language::run(source, stack, sizeof(stack), intrin, 2, hash, &macros);
	assert(r.exit_status == condition_language::SUCCESS && r.value == true);
}

void benchmark(const char* source, const char* defines[], unsigned runs) {
	unsigned char stack[64];
	condition_language::Intrinsic intrin[] = {
		{ hash("equal", 5), instrinsic::equal },
		{ hash("defined", 7), instrinsic::defined }
	};

	Defines macros = { defines, sizeof(defines) / sizeof(char*) };

	printf("Running '%s' %d times:\n", source, runs);
	clock_t start = clock();
	for (unsigned i = 0; i != runs; ++i)
		condition_language::run(source, stack, sizeof(stack), intrin, 2, hash, &macros);
	clock_t end = clock();
	double total = (end - start) * (1000.0 / CLOCKS_PER_SEC);
	printf("Total %.4fms\nAvg: %.4fms/run\n\n", total, total / runs);
}

int main(int argc, char* argv[]) {
	test_single_intrinsic();
	test_several_intrinsics();

	const char* macros[] = {"VAL1"};
	benchmark("equal(A, A) && !!(equal(B, B) || equal(B, C))", macros, 100000);
	benchmark("equal(A, A) && !!(equal(B, B) || equal(B, C)) && (!defined(DEFINED) || defined(VAL1))", macros, 100000);

	getc(stdin);
	return 0;
}
