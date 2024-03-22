#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h> 
#include <omp.h>
#include <time.h>
#include <mpi.h>
#include <string.h>

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


short int* mandelbrot(int id, int numproc, int n_x, int n_y, double x_L, double y_L, double dx, double dy, int Imax) {

    int rows_per_process = n_y / numproc; 

    if (id < n_y % numproc) {
    rows_per_process++; 
    }

    short int* local_image = malloc(n_x * rows_per_process * sizeof(short int));
    
    for (int i = id, row_index = 0; i < n_y; i += numproc, row_index++) {
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < n_x; j++) {
            double x = x_L + j * dx;
            double y = y_L + i * dy;
            double complex c = x + y * I;
            double complex val = c;
            int k = 0;

            while (k < Imax && cabs(val) < 2) {
                val = cpow(val, 2) + c;
                k++;
            }
    
            local_image[row_index * n_x + j] = (k == Imax ? 0 : k);
        }
    }
    return local_image;
}


int main(int argc, char** argv){

    int mpi_provided_thread_level;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &mpi_provided_thread_level);
    
    if (mpi_provided_thread_level < MPI_THREAD_FUNNELED) {
        printf("The MPI implementation does not support MPI_THREAD_FUNNELED.\n");
        MPI_Finalize();
        exit(1);
    }

    int numproc, id;
    double start_time, end_time, elapsed_time;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);

    MPI_Barrier(MPI_COMM_WORLD);
    if (id == 0) {
        start_time = MPI_Wtime();
    }

    if (argc != 9) {
        printf("Usage: %s nx ny x_L y_L x_R y_R Imax number_of_threads\n", argv[0]);
        return 1;
    }    

    int n_x = atoi(argv[1]);
    int n_y = atoi(argv[2]);
    double x_L = atof(argv[3]);
    double y_L = atof(argv[4]);
    double x_R = atof(argv[5]);
    double y_R = atof(argv[6]);
    int Imax = atoi(argv[7]);
    int num_threads = atoi(argv[8]);

    omp_set_num_threads(num_threads);


    double dx = (x_R - x_L) / n_x;
    double dy = (y_R - y_L) / n_y;

    int *recvcounts = NULL;
    int *displs = NULL;
    if (id == 0) {
        recvcounts = (int *)malloc(numproc * sizeof(int));
        displs = (int *)malloc(numproc * sizeof(int));

    for (int i = 0; i < numproc; i++) {
        int rows_for_this_process = 0;
            for (int row = i; row < n_y; row += numproc) {
                rows_for_this_process++; 
            }
        recvcounts[i] = rows_for_this_process * n_x; 
    }

    displs[0] = 0;
    for (int i = 1; i < numproc; i++) {
        displs[i] = displs[i - 1] + recvcounts[i - 1];
    }
}

int rows_per_process = n_y / numproc; 
if (id < n_y % numproc) {
    rows_per_process++; 
}

int sendcount = rows_per_process * n_x;

short int *complete_image = NULL;
if (id == 0) {
    complete_image = malloc(n_x * n_y * sizeof(short int));
}

short int* local_image = mandelbrot(id, numproc, n_x, n_y, x_L, y_L, dx, dy, Imax);

MPI_Gatherv(local_image, sendcount, MPI_SHORT,
            complete_image, recvcounts, displs, MPI_SHORT,
            0, MPI_COMM_WORLD);

MPI_Barrier(MPI_COMM_WORLD);

if (id == 0) {
    short int* final_image = malloc(n_x * n_y * sizeof(short int));


    for (int i = 0; i < n_y; i++) {
        int source_process = i % numproc;
        int row_within_process = i / numproc;
        int source_index = displs[source_process] + row_within_process * n_x;
        
        memcpy(final_image + (i * n_x), complete_image + source_index, n_x * sizeof(short int));
    }

    end_time = MPI_Wtime();
    elapsed_time = end_time - start_time;
    printf("Total execution time: %f seconds\n", elapsed_time);

    char* filename = "mandelbrot_hybrid.pgm"; 
    write_pgm_image(final_image, n_x, n_y, "mandelbrot_hybrid.pgm");
    
    free(final_image);
    free(complete_image);
    free(recvcounts);
    free(displs);
}

free(local_image); 
MPI_Finalize();
return 0;
}
