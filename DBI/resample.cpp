#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "resample.h"

double* upsample(double* in, int UPSAMPLE_RATE) {
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    int len_out = len_in * UPSAMPLE_RATE;
    double* out = (double*)malloc(sizeof(double) * len_out);
    assert(out != NULL);
    for (int i = 0; i < len_out; i++) {
        *(out + i) = 0;
    }
    for (int i = 0; i < len_in; i++) {
        *(out + i * UPSAMPLE_RATE) = *(in + i);
    }
    return out;
}

double* downsample(double* in, int DOWNSAMPLE_RATE) {
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    assert(!(len_in % DOWNSAMPLE_RATE));
    int len_out = len_in / DOWNSAMPLE_RATE;
    double* out = (double*)malloc(sizeof(double) * len_out);
    assert(out != NULL);
    for (int i = 0; i < len_out; i++) {
        *(out + i) = *(in + i * DOWNSAMPLE_RATE);
    }
    return out;
}
