#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../compat.h"
#include "operate_file.h"

double* load_file(const char* PATH, int LENGTH) {
    FILE* fp;
    fp = fopen(PATH, "r");
    assert(fp != NULL);

    double* data = (double*)malloc(sizeof(double) * LENGTH);
    assert(data != NULL);

    for (int i = 0; i < LENGTH; i++) {
        fscanf(fp, "%lf", data + i);
    }

    fclose(fp);
    return data;
}

int* load_file_sync(const char* PATH, int LENGTH) {
    FILE* fp;
    fp = fopen(PATH, "r");
    assert(fp != NULL);

    int* data = (int*)malloc(sizeof(int) * LENGTH);
    assert(data != NULL);

    for (int i = 0; i < LENGTH; i++) {
        fscanf(fp, "%d", data + i);
    }

    fclose(fp);
    return data;
}

void write_file(const char* PATH, double* data, int len) {
    FILE* fp;
    fp = fopen(PATH, "w");
    assert(fp != NULL);

    for (int i = 0; i < len; i++) {
        fprintf(fp, "%lf\n", *(data + i));
    }

    fclose(fp);
}
