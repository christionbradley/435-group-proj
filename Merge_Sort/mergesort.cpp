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

    merge_sort(arr, mid);
    merge_sort(arr + mid, size - mid);
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
            if (perturb) {arr[i] = rand() % 100000;}
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
    if (argc >= 2) {
        list_size = atoi(argv[1]);
    }
    if (argc >= 3) {
        list_type = argv[2];
    }

    std::cout << list_type << std::endl;

    int* og_data = nullptr;
    if (rank == 0) {
        og_data = (int*)malloc(list_size * sizeof(int));
        CALI_MARK_BEGIN("data_init_runtime");
        data_init_runtime(og_data, list_size, list_type);
        CALI_MARK_END("data_init_runtime");
    }

    adiak::init(NULL);
    adiak::launchdate();
    adiak::libraries();
    adiak::cmdline();
    adiak::clustername();
    adiak::value("algorithm", "merge sort");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", "4");
    adiak::value("input_size", std::to_string(list_size));
    adiak::value("input_type", list_type);
    adiak::value("num_procs", std::to_string(num_tasks));
    adiak::value("scalability", "strong");
    adiak::value("group_num", "12");
    adiak::value("implementation_source", "online");

    int local_list_size = list_size / num_tasks;
    int* local_list = (int*)malloc(local_list_size * sizeof(int));

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Scatter(og_data, local_list_size, MPI_INT, local_list, local_list_size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if (rank == 0) {
        free(og_data);  // Free og_data after scatter
        og_data = nullptr;
    }

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    merge_sort(local_list, local_list_size);
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    // Parallel merge phase
    for (int step = 1; step < num_tasks; step *= 2) {
        if (rank % (2 * step) == 0) {
            if (rank + step < num_tasks) {
                int partner_rank = rank + step;
                int partner_size;
                MPI_Status status;
                
                // size of the partner's data
                CALI_MARK_BEGIN("comm");
                CALI_MARK_BEGIN("comm_small");
                MPI_Recv(&partner_size, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, &status);
                CALI_MARK_END("comm_small");
                CALI_MARK_END("comm");
                
                int* recv_buffer = (int*)malloc(partner_size * sizeof(int));
                
                CALI_MARK_BEGIN("comm");
                CALI_MARK_BEGIN("comm_small");
                // receive the actual data
                MPI_Recv(recv_buffer, partner_size, MPI_INT, partner_rank, 1, MPI_COMM_WORLD, &status);
                CALI_MARK_END("comm_small");
                CALI_MARK_END("comm");
                
                int new_size = local_list_size + partner_size;
                int* merged = (int*)malloc(new_size * sizeof(int));
                
                // perform the merge
                std::merge(local_list, local_list + local_list_size, 
                           recv_buffer, recv_buffer + partner_size, merged);
                
                free(local_list);
                free(recv_buffer);
                
                local_list = merged;
                local_list_size = new_size;
            }
        } else if (rank % step == 0) {
            int target_rank = rank - step;
            
            // send the size of the local data
            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_small");
            MPI_Send(&local_list_size, 1, MPI_INT, target_rank, 0, MPI_COMM_WORLD);
            
            // send the actual data
            MPI_Send(local_list, local_list_size, MPI_INT, target_rank, 1, MPI_COMM_WORLD);
            
            CALI_MARK_END("comm_small");
            CALI_MARK_END("comm");
            
        }
    }

    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");
    
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    // Gather the final sorted list
    int* recv_counts = nullptr;
    int* displacements = nullptr;
    if (rank == 0) {
        recv_counts = (int*)malloc(num_tasks * sizeof(int));
        displacements = (int*)malloc(num_tasks * sizeof(int));
    }

    // Gather the sizes of local lists from all processes
    MPI_Gather(&local_list_size, 1, MPI_INT, recv_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        displacements[0] = 0;
        int total_size = recv_counts[0];
        for (int i = 1; i < num_tasks; i++) {
            displacements[i] = displacements[i-1] + recv_counts[i-1];
            total_size += recv_counts[i];
        }
        og_data = (int*)malloc(total_size * sizeof(int));
    }
    MPI_Gatherv(local_list, local_list_size, MPI_INT, og_data, recv_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    free(local_list);
    
    if (rank == 0) {
        free(recv_counts);
        free(displacements);

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