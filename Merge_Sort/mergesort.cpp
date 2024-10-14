#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <stdexcept>

#include <iostream>
#include <algorithm>
#include <cstdlib>

bool correctness_check(int* arr, int list_size) {
    for (int i = 0; i < list_size - 1; i++){
        if (arr[i] > arr[i+1]) {return false;} 
    }
    return true;
}

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


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, num_tasks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);

    CALI_MARK_BEGIN("main");

    int list_size;
    std::string list_type;
    if (argc >= 2)
    {
        list_size = atoi(argv[1]);
    }
    if (argc >= 3)
    {
        list_type = argv[2];
    }

    std::cout << list_type << std::endl;

    int* og_data = nullptr;
    if (rank == 0) {
        og_data = (int*)malloc(list_size * sizeof(int));
        CALI_MARK_BEGIN("data_init_runtime");
        data_init_runtime(og_data, list_size, list_type);
        CALI_MARK_END("data_init_runtime");

        // std::cout << "Unsorted data: ";
        // for (int i = 0; i < list_size; i++) {
        //     std::cout << og_data[i] << " ";
        // }
        // std::cout << std::endl;
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "merge sort"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", "4"); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", std::to_string(list_size)); // The number of elements in input dataset (1000)
    adiak::value("input_type", "random"); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", std::to_string(num_tasks)); // The number of processors (MPI ranks)
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", "12"); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "online"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").


    int local_list_size = list_size / num_tasks;
    int* local_list = (int*)malloc(local_list_size * sizeof(int));

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    MPI_Scatter(og_data, local_list_size, MPI_INT, local_list, local_list_size, MPI_INT, 0, MPI_COMM_WORLD);
    
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");

    merge_sort(local_list, local_list_size);

    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    if (rank == 0) {
        og_data = (int*)realloc(og_data, list_size * sizeof(int));
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    MPI_Gather(local_list, local_list_size, MPI_INT, og_data, local_list_size, MPI_INT, 0, MPI_COMM_WORLD);

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    free(local_list);

    int* temp = nullptr;
    if (rank == 0) {
        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");
        for (int step = 1; step < num_tasks; step *= 2) {
            for (int i = 0; i < num_tasks; i += step * 2) {
                if (i + step < num_tasks) {
                    int left_size = local_list_size * step;
                    int right_size = (i + step * 2 <= num_tasks) ? local_list_size * step : (num_tasks - (i + step)) * local_list_size;
                    merge(og_data + i * local_list_size, left_size, right_size);
                }
            }
        }
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
        free(og_data);
    }
    
    CALI_MARK_END("main");
    MPI_Finalize();
    return 0;
}


