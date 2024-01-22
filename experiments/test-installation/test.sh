#!/bin/bash

if [ -z ${WINMUT_BASE_DIR+x} ]; then
	WINMUT_BASE_DIR="$(pwd)/../../cmake-build-release/"
fi

TMPFILE=$(mktemp -d)
cd ${TMPFILE}
cat << EOF > a.c
#include <stdio.h>
#include <assert.h>

__attribute__((noinline))
int f (int a, int b) {
	return a + b;
}

int main() {
	assert(f(1, 2) == 3);
	printf("This should print exactly once\n");
}
EOF

"${WINMUT_BASE_DIR}/bin/clang" -O2 -winmut a.c -o a.out
mkdir winmut_log_dir
LD_PRELOAD="${WINMUT_BASE_DIR}/lib/libLLVMWinMutRuntime.so" WINMUT_LOG_FILE_PREFIX="$(pwd)/winmut_log_dir" ./a.out > a.result

echo "This should print exactly once" > a.expect
diff a.expect a.result && echo "log file statistics" && wc winmut_log_dir/* && echo "Success" || echo "Fail"



