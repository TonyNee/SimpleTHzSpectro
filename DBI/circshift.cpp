#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "circshift.h"

double* circshift(double* data, int BIAS) {
    int len_data = static_cast<int>(malloc_usable_size(data) / sizeof(double));
    int bias_local = BIAS % len_data;
    double* out = (double*)malloc(sizeof(double) * len_data);
    assert(out != NULL);
    if (bias_local < 0) {
        bias_local = bias_local + len_data;
    }
    for (int i = 0; i < bias_local; i++) {
        *(out + i) = *(data + len_data - bias_local + i);
    }
    for (int i = bias_local; i < len_data; i++) {
        *(out + i) = *(data - bias_local + i);
    }
    return out;
}
