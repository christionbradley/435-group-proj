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

bool correctness_check(int* arr, int list_size) {
    for (int i = 0; i < list_size - 1; i++){
        if (arr[i] > arr[i+1]) {return false;} 
    }
    return true;
}

void data_init_runtime(int* arr, int list_size, std::string list_type){

    if (list_type == "random"){
        std::cout << "chose random" << std::endl;
        for (int i = 0; i < list_size; i++) {
        arr[i] = rand() % 100000;
    }
    } else if (list_type == "sorted") {
        std::cout << "chose sorted" << std::endl;
        for (int i = 0; i < list_size; i++) {
            arr[i] = i;
        }
    } else if (list_type == "reverse_sorted") {
        std::cout << "chose reverse sorted" << std::endl;
        int num = 0;
        for (int i = list_size - 1; 0 <= i; i--) {
            arr[i] = num; num++;
        }
    } else if (list_type == "1%perturbed") {
        std::cout << "chose 1%perturbed" << std::endl;
        for (int i = 0; i < list_size; i++) {
            bool perturb = (rand() % 100 == 1) ? true : false;

            if (perturb) {arr[i] = rand() % 100000;;}
            else {arr[i] = i;}
            
        }
    } else {throw std::runtime_error("Unknown List Type!");}
    
}

void bitonic_merge(int a[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            if (dir == (a[i] > a[i + k])) {
                int temp = a[i];
                a[i] = a[i + k];
                a[i + k] = temp;
            }
        }
        bitonic_merge(a, low, k, dir);
        bitonic_merge(a, low + k, k, dir);
    }
}

void bitonic_sort(int a[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        bitonic_sort(a, low, k, 1);  // ascending
        bitonic_sort(a, low + k, k, 0);  // descending
        bitonic_merge(a, low, cnt, dir);
    }
}

int main(int argc, char** argv) {
    int rank, size, local_size;
    int* og_arr = NULL;
    int* workers_arr = NULL;
    int* final_list = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    CALI_MARK_BEGIN("main");

    int list_size;
    if (argc >= 2)
    {
        list_size = atoi(argv[1]);
    }
    if (argc >= 3)
    {
        list_type = argv[2];
    }

    std::cout << list_type << std::endl;

    if (rank == 0) {
        og_arr = (int*)malloc(list_size * sizeof(int));
        CALI_MARK_BEGIN("data_init_runtime");
        data_init_runtime(og_data, list_size, list_type);
        CALI_MARK_END("data_init_runtime");
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "bitonic sort"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", "4"); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", std::to_string(list_size)); // The number of elements in input dataset (1000)
    adiak::value("input_type", "random"); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", std::to_string(num_tasks)); // The number of processors (MPI ranks)
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", "12"); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "ai"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    MPI_Bcast(&list_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    local_size = list_size / size;
    workers_arr = (int*)malloc(local_size * sizeof(int));

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    MPI_Scatter(og_arr, local_size, MPI_INT, workers_arr, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");

    bitonic_sort(workers_arr, 0, local_size, 1);

    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    if (rank == 0) {
        final_list = (int*)malloc(n * sizeof(int));
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    MPI_Gather(workers_arr, local_size, MPI_INT, final_list, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    free(workers_arr);

    if (rank == 0) {
        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");

        bitonic_sort(final_list, 0, n, 1);

        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");

        std::cout << "Sorted data: ";
        for (int i = 0; i < std::min(list_size, 1000); i++) {
            std::cout << og_data[i] << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Sorted!" << std::endl;
        
        CALI_MARK_BEGIN("correctness_check");
        if (!correctness_check(og_data, list_size)) {
            throw std::runtime_error("List not Sorted!");
        }
        CALI_MARK_END("correctness_check");

        free(og_arr)
        free(final_list);
    }

    CALI_MARK_END("main");

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
