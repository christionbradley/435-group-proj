#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#include <iostream>
#include <algorithm>
#include <cstdlib>

void merge(int* arr, int left_size, int right_size) {

    int l = 0; int r = left_size; int i = 0;
    
    int total_size = left_size + right_size;
    int* temp = (int*)malloc(total_size * sizeof(int));
    
    while (l < left_size && r < total_size) {
        
        if (arr[l] <= arr[r]) {
            temp[i] = arr[l]; l++;
        } else {
            temp[i] = arr[r]; r++;
        }
        i++;
    }

    while (l < left_size) {
        temp[i] = arr[l]; l++;
        i++;
    }

    while (r < total_size) {
        temp[i] = arr[r]; r++;
        i++;
    }
    
    std::copy(temp, temp + total_size, arr);
    free(temp); 
}


void merge_sort(int* arr, int size) {

    if (size <= 1) {return;}
    int mid = size / 2;

    merge_sort(arr, mid); merge_sort(arr + mid, size - mid);
    merge(arr, mid, size - mid);
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, num_tasks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);

    int list_size;
    if (argc == 2)
    {
        list_size = atoi(argv[1]);
    }

    int* og_data = nullptr;
    if (rank == 0) {
        og_data = (int*)malloc(list_size * sizeof(int));
        for (int i = 0; i < list_size; i++) {
            og_data[i] = rand() % 10000;
        }

        // std::cout << "Unsorted data: ";
        // for (int i = 0; i < list_size; i++) {
        //     std::cout << og_data[i] << " ";
        // }
        // std::cout << std::endl;

    }

    int local_list_size = list_size / num_tasks;
    int* local_list = (int*)malloc(local_list_size * sizeof(int));

    MPI_Scatter(og_data, local_list_size, MPI_INT, local_list, local_list_size, MPI_INT, 0, MPI_COMM_WORLD);
    merge_sort(local_list, local_list_size);

    if (rank == 0) {
        og_data = (int*)realloc(og_data, list_size * sizeof(int));
    }

    MPI_Gather(local_list, local_list_size, MPI_INT, og_data, local_list_size, MPI_INT, 0, MPI_COMM_WORLD);

    free(local_list);

    int* temp = nullptr;
    if (rank == 0) {
        for (int step = 1; step < num_tasks; step *= 2) {
            for (int i = 0; i < num_tasks; i += step * 2) {
                if (i + step < num_tasks) {
                    int left_size = local_list_size * step;
                    int right_size = (i + step * 2 <= num_tasks) ? local_list_size * step : (num_tasks - (i + step)) * local_list_size;
                    merge(og_data + i * local_list_size, left_size, right_size);
                }
            }
        }
        
        // std::cout << "Sorted data: ";
        // for (int i = 0; i < list_size; i++) {
        //     std::cout << og_data[i] << " ";
        // }
        // std::cout << std::endl;
        
        free(og_data);
    }
    
    MPI_Finalize();
    return 0;
}


