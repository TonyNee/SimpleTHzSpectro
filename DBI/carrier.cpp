#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "carrier.h"

double carrier_sync(double* in, double fc, int CAL_LEN) {
    double pi = 3.14159265358979323846264338;
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    assert(len_in >= CAL_LEN);
    double lo_coh_I = 0, lo_coh_Q = 0;
    for (int i = 0; i < CAL_LEN; i++) {
        lo_coh_I = lo_coh_I + *(in + i) * sin(2 * pi * fc * i);
        lo_coh_Q = lo_coh_Q + *(in + i) * cos(2 * pi * fc * i);
    }
    double lo_phi = atan2(lo_coh_Q, lo_coh_I);
    return lo_phi;
}

double* carrier_removal(double* in, double fc, int CAL_LEN) {
    double pi = 3.14159265358979323846264338;
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    assert(len_in >= CAL_LEN);
    double lo_coh_I = 0, lo_coh_Q = 0;
    for (int i = 0; i < CAL_LEN; i++) {
        lo_coh_I = lo_coh_I + *(in + i) * sin(2 * pi * fc * i);
        lo_coh_Q = lo_coh_Q + *(in + i) * cos(2 * pi * fc * i);
    }
    double lo_phi = atan2(lo_coh_Q, lo_coh_I);
    double lo_amp = 2 * sqrt(lo_coh_Q * lo_coh_Q + lo_coh_I * lo_coh_I) / CAL_LEN;

    double* out = (double*)malloc(sizeof(double) * len_in);
    assert(out != NULL);
    for (int i = 0; i < len_in; i++) {
        *(out + i) = *(in + i) - lo_amp * sin(2 * pi * fc * i + lo_phi);
    }
    return out;
}
