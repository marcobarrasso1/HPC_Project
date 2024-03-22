#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h> 
#include <omp.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include "create_image.h"

// ================================================================
//  FUNCTIONS DEFINITION
// ================================================================

// Function for creating the matrix using openMP threads.
short int* mandelbrot(int id, int numproc, int n_x, int n_y, double x_L, double y_L, double dx, double dy, int Imax) {

    // Assigning rows to MPI processes
    int rows_per_process = n_y / numproc; 
  
    if (id < n_y % numproc) {
    rows_per_process++; 
    }

    // Every MPI processes on which the function is going to be called creates a matrix based on its number of rows
    short int* local_image = malloc(n_x * rows_per_process * sizeof(short int));

    // Every thread works on a different point
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

    // Starting the timer from the root process
    MPI_Barrier(MPI_COMM_WORLD);
    if (id == 0) {
        start_time = MPI_Wtime();
    }

  // The program takes in input also the number of threads
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
    // Specifies the number of elements expected to be received from each process
    int *recvcounts = NULL;

   // Specifies the displacement at which to place the incoming data from each process in the receive buffer on the root process
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
  
// Number of elements each process will send
int sendcount = rows_per_process * n_x;

// Allocate memory for the complete image on the root process
short int *complete_image = NULL;
if (id == 0) {
    complete_image = malloc(n_x * n_y * sizeof(short int));
}
  
// Generate the Mandelbrot set fragment for the current process
short int* local_image = mandelbrot(id, numproc, n_x, n_y, x_L, y_L, dx, dy, Imax);

// Gather all fragments of the Mandelbrot set at the root process
MPI_Gatherv(local_image, sendcount, MPI_SHORT,
            complete_image, recvcounts, displs, MPI_SHORT,
            0, MPI_COMM_WORLD);

// Synchronize before proceeding to ensure all processes have reached this point
MPI_Barrier(MPI_COMM_WORLD);

if (id == 0) {
    short int* final_image = malloc(n_x * n_y * sizeof(short int));

    // Rearrange the complete image in the root processes
    for (int i = 0; i < n_y; i++) {
        // Determine which process contributed the row
        int source_process = i % numproc;
      
        // Determine the row's position within the process's data
        int row_within_process = i / numproc;

         // Calculate the source index in the complete image array
        int source_index = displs[source_process] + row_within_process * n_x;

        // Copy the row from the complete_image buffer to the correct position in final_image
        memcpy(final_image + (i * n_x), complete_image + source_index, n_x * sizeof(short int));
    }

    end_time = MPI_Wtime();
    elapsed_time = end_time - start_time;
    printf("%f\n", elapsed_time);
    
    free(final_image);
    free(complete_image);
    free(recvcounts);
    free(displs);
}

free(local_image); 
MPI_Finalize();
return 0;
}
