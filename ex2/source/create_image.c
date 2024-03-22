#include "create_image.h"
#include <stdio.h>

void write_pgm_image(const short int* image, int width, int height, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        return;
    }

    fprintf(file, "P5\n%d %d\n65535\n", width, height);

    for (int i = 0; i < width * height; i++) {
        short int pixelValue = image[i];
        fwrite(&pixelValue, sizeof(short int), 1, file);
    }

    fclose(file);
}
