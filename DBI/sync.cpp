#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "xcorr.h"
#include "circshift.h"
#include "sync.h"

SYNC_STR sync(double* in, double* ref, int SYNC_BIAS, int SYNC_LENGTH, int OUTPUT_LENGTH) {
    assert(static_cast<int>(malloc_usable_size(in) / sizeof(double)) >= SYNC_BIAS + OUTPUT_LENGTH);
    double* sync_in = (double*)malloc(sizeof(double) * SYNC_LENGTH);
    assert(sync_in != NULL);
    for (int i = 0; i < SYNC_LENGTH; i++) {
        *(sync_in + i) = *(in + SYNC_BIAS + i);
    }
    double* syn = xcorr(sync_in, ref);
    int syn_len = static_cast<int>(malloc_usable_size(syn) / sizeof(double));
    for (int i = 0; i < syn_len; i++) {
        if (*(syn + i) < 0) {
            *(syn + i) = -*(syn + i);
        }
    }
    int max_location = 0;
    double max_value = 0;
    for (int i = 0; i < syn_len; i++) {
        if (*(syn + i) > max_value) {
            max_value = *(syn + i);
            max_location = i;
        }
    }
    SYNC_STR sync_out;
    sync_out.syn_head = max_location - (syn_len - 1) / 2;
    double* in_shift = circshift(in, -sync_out.syn_head);
    sync_out.data = (double*)malloc(sizeof(double) * OUTPUT_LENGTH);
    assert(sync_out.data != NULL);
    for (int i = 0; i < OUTPUT_LENGTH; i++) {
        *(sync_out.data + i) = *(in_shift + SYNC_BIAS + i);
    }
    free(sync_in);
    free(syn);
    free(in_shift);
    return sync_out;
}
