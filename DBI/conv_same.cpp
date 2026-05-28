#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "conv_same.h"

double* conv_same(double* in1, double* in2) {
    int len_in1 = static_cast<int>(malloc_usable_size(in1) / sizeof(double));
    int len_in2 = static_cast<int>(malloc_usable_size(in2) / sizeof(double));
    double* out = (double*)malloc(sizeof(double) * len_in1);
    assert(out != NULL);

    for (int i = 0; i < len_in1; i++) {
        *(out + i) = 0;
        int len_in2_hf;
        if (len_in2 % 2 == 0) {
            len_in2_hf = len_in2 / 2;
            if (i < len_in2_hf - 1) {
                for (int j = 0; j < len_in2_hf + 1 + i; j++) {
                    *(out + i) = *(out + i) + in1[j] * in2[len_in2_hf + i - j];
                }
            }
            else if (i > len_in1 - 1 - len_in2_hf) {
                for (int j = 0; j < len_in2_hf + (len_in1 - 1 - i); j++) {
                    *(out + i) = *(out + i) + in1[len_in1 - 1 - j] * in2[len_in2_hf - (len_in1 - 1 - i) + j];
                }
            }
            else {
                for (int j = 0; j < len_in2; j++) {
                    *(out + i) = *(out + i) + in1[i - len_in2_hf + 1 + j] * in2[len_in2 - 1 - j];
                }
            }
        }
        else {
            len_in2_hf = (len_in2 - 1) / 2;
            if (i < len_in2_hf) {
                for (int j = 0; j < len_in2_hf + 1 + i; j++) {
                    *(out + i) = *(out + i) + in1[j] * in2[len_in2_hf + i - j];
                }
            }
            else if (i > len_in1 - 1 - len_in2_hf) {
                for (int j = 0; j < len_in2_hf + 1 + (len_in1 - 1 - i); j++) {
                    *(out + i) = *(out + i) + in1[len_in1 - 1 - j] * in2[len_in2_hf - (len_in1 - 1 - i) + j];
                }
            }
            else {
                for (int j = 0; j < len_in2; j++) {
                    *(out + i) = *(out + i) + in1[i - len_in2_hf + j] * in2[len_in2 - 1 - j];
                }
            }
        }
    }
    return out;
}
