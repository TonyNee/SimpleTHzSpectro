#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../compat.h"
#include "conv_same.h"
#include "ffe.h"

FFT_STR FFE(double* in, double* train, int TAP_LENGTH, int SYMBOL_WIDTH, double u) {
    int len_in = static_cast<int>(malloc_usable_size(in) / sizeof(double));
    int len_train = static_cast<int>(malloc_usable_size(train) / sizeof(double));
    assert(len_in == len_train * SYMBOL_WIDTH);

    FFT_STR ffe_out;
    ffe_out.out = (double*)malloc(sizeof(double) * len_train);
    ffe_out.w = (double*)malloc(sizeof(double) * TAP_LENGTH);
    ffe_out.e = (double*)malloc(sizeof(double) * len_in);
    ffe_out.z = (double*)malloc(sizeof(double) * len_in);
    double* d = (double*)malloc(sizeof(double) * TAP_LENGTH);
    assert(ffe_out.out != NULL);
    assert(ffe_out.w != NULL);
    assert(ffe_out.e != NULL);
    assert(ffe_out.z != NULL);
    assert(d != NULL);

    int TAP_BIAS = (TAP_LENGTH - 1) / 2;
    int I_START = floor((float)TAP_BIAS / SYMBOL_WIDTH) + 1;
    int I_END = len_train - floor(TAP_BIAS / SYMBOL_WIDTH) - 2;

    for (int t = 0; t < TAP_LENGTH; t++) {
        *(ffe_out.w + t) = 0;
    }

    for (int i = I_START; i <= I_END; i++) {
        int D_START = i * SYMBOL_WIDTH - TAP_BIAS - 1;
        for (int t = 0; t < TAP_LENGTH; t++) {
            *(d + t) = *(in + D_START + TAP_LENGTH - t);
        }

        *(ffe_out.z + i) = 0;
        for (int t = 0; t < TAP_LENGTH; t++) {
            *(ffe_out.z + i) = *(ffe_out.z + i) + *(d + t) * *(ffe_out.w + t);
        }

        *(ffe_out.e + i) = *(train + i) - *(ffe_out.z + i);

        for (int t = 0; t < TAP_LENGTH; t++) {
            *(ffe_out.w + t) = *(ffe_out.w + t) + u * *(ffe_out.e + i) * *(d + t);
        }
    }

    double* conv_out = conv_same(in, ffe_out.w);

    for (int i = 0; i < len_train; i++) {
        *(ffe_out.out + i) = *(conv_out + i * SYMBOL_WIDTH);
    }

    free(conv_out);
    free(d);

    return(ffe_out);
}

MISO_FFT_STR MISO_FFE(double** in, double* train, int TAP_LENGTH, double u, int group_num) {
    int len_in = static_cast<int>(malloc_usable_size(*(in)) / sizeof(**(in)));
    int len_train = static_cast<int>(malloc_usable_size(train) / sizeof(double));
    assert(len_in == len_train);

    MISO_FFT_STR ffe_out;
    ffe_out.w = (double**)malloc(sizeof(double*) * group_num);
    assert(ffe_out.w != NULL);
    for (int g = 0; g < group_num; g++) {
        *(ffe_out.w + g) = (double*)malloc(sizeof(double) * TAP_LENGTH);
        assert(*(ffe_out.w + g) != NULL);
    }
    double** d = (double**)malloc(sizeof(double*) * group_num);
    assert(d != NULL);
    for (int g = 0; g < group_num; g++) {
        *(d + g) = (double*)malloc(sizeof(double) * TAP_LENGTH);
        assert(*(d + g) != NULL);
    }
    ffe_out.e = (double*)malloc(sizeof(double) * len_in);
    ffe_out.z = (double*)malloc(sizeof(double) * len_in);
    assert(ffe_out.e != NULL);
    assert(ffe_out.z != NULL);

    int TAP_BIAS = floor((float)TAP_LENGTH / 2);
    int I_START = TAP_BIAS + 1;
    int I_END = len_train - TAP_BIAS - 2;

    for (int g = 0; g < group_num; g++) {
        for (int j = 0; j < TAP_LENGTH; j++) {
            *(*(ffe_out.w + g) + j) = 0;
        }
    }

    for (int i = I_START; i <= I_END; i++) {
        int D_START = i - TAP_BIAS - 1;
        for (int g = 0; g < group_num; g++) {
            for (int t = 0; t < TAP_LENGTH; t++) {
                *(*(d + g) + t) = *(*(in + g) + D_START + TAP_LENGTH - t);
            }
        }

        *(ffe_out.z + i) = 0;
        for (int g = 0; g < group_num; g++) {
            for (int t = 0; t < TAP_LENGTH; t++) {
                *(ffe_out.z + i) = *(ffe_out.z + i) + *(*(d + g) + t) * *(*(ffe_out.w + g) + t);
            }
        }

        *(ffe_out.e + i) = *(train + i) - *(ffe_out.z + i);

        for (int g = 0; g < group_num; g++) {
            for (int t = 0; t < TAP_LENGTH; t++) {
                *(*(ffe_out.w + g) + t) = *(*(ffe_out.w + g) + t) + u * *(ffe_out.e + i) * *(*(d + g) + t);
            }
        }
    }

    ffe_out.out = conv_same(*in, *(ffe_out.w));
    for (int g = 1; g < group_num; g++) {
        double* tmp = conv_same(*(in + g), *(ffe_out.w + g));
        for (int i = 0; i < len_train; i++) {
            *(ffe_out.out + i) = *(ffe_out.out + i) + *(tmp + i);
        }
        free(tmp);
    }

    for (int g = 0; g < group_num; g++) {
        free(*(d + g));
    }
    free(d);

    return(ffe_out);
}
