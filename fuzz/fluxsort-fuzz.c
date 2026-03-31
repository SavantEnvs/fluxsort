#include <stdint.h>
#include <string.h>
#include "../src/fluxsort.h"

uint8_t cmp_func(const void* a, const void* b) {
    return *(uint8_t *) a - *(uint8_t *) b;
}

uint8_t input_data[16000];

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Make new array since the fuzzer does not like it modifying the const data
    memcpy(input_data, data, size);

    fluxsort(input_data, size, sizeof(char), cmp_func);
}
