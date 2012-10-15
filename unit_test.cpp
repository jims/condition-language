#include "condition_language.h"
#include <stdio.h>

namespace instrinsic {
	bool defined(IdString32* params, unsigned n_params, void* ud) {
		printf("kjahsdkhjasdkhj");
		return false;
	}
}

void test_single_intrinsic() {
	const char* source = "defined(VAL1) && !defined(VAL2)";
	condition_language::Intrinsic intrin[] = {
		{12345, instrinsic::defined}
	};
	unsigned char stack[128] = {0};
	condition_language::Result r = condition_language::run(source, stack, sizeof(stack), intrin, 1, 0);
}

int main(int argc, char* argv[]) {
	test_single_intrinsic();
	return 0;
}
