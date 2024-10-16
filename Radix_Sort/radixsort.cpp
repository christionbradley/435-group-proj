#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#include <stdexcept>

#include <iostream>

int getMax(int arr[], int n) {
    int max = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

void countSort(int arr[], int n, int exp) {
    int* output = (int*)malloc(n * sizeof(int));
    int count[10] = {0};

    for (int i = 0; i < n; i++) {
        count[(arr[i] / exp) % 10]++;
    }

    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    for (int i = n - 1; i >= 0; i--) {
        output[count[(arr[i] / exp) % 10] - 1] = arr[i];
        count[(arr[i] / exp) % 10]--;
    }

    for (int i = 0; i < n; i++) {
        arr[i] = output[i];
    }

    free(output);
}

void radixSort(int arr[], int n) {
    int m = getMax(arr, n);

    for (int exp = 1; m / exp > 0; exp *= 10) {
        countSort(arr, n, exp);
    }
}

void data_init_runtime(int* arr, int list_size, std::string list_type){
    if (list_type == "random") {
        std::cout << "chose random" << std::endl;
        for (int i = 0; i < list_size; i++) {
            arr[i] = rand() % 10000;
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
            bool perturb = (rand() % 100 == 1);
            if (perturb) {
                arr[i] = rand() % 10000;
            } else {
                arr[i] = i;
            }
        }
    } else {
        throw std::runtime_error("Unknown List Type!");
    }
}

int main(int argc, char** argv) {
    int rank, size, n;
    int* arr = NULL;
    int* local_arr = NULL;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0) {
            printf("Usage: %s <array_size> <list_type>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    n = atoi(argv[1]);
    std::string list_type = argv[2];

    CALI_MARK_BEGIN("main");

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "radix sort");  // Name of the algorithm being used
    adiak::value("programming_model", "mpi"); // Programming model used (e.g., MPI)
    adiak::value("data_type", "int");         // The data type of the input elements (int)
    adiak::value("size_of_data_type", "4");   // Size of the data type (int) in bytes
    adiak::value("input_size", std::to_string(n)); // Size of the input array
    adiak::value("input_type", list_type);    // Type of input array (sorted, reverse_sorted, etc.)
    adiak::value("num_procs", std::to_string(size)); // Number of processors (MPI ranks)
    adiak::value("scalability", "strong");    // Scalability mode (strong or weak)
    adiak::value("group_num", "12");          // Your group number (e.g., 12)
    adiak::value("implementation_source", "online"); // Where the implementation was sourced from (online)

    if (rank == 0) {
        arr = (int*)malloc(n * sizeof(int));
        CALI_MARK_BEGIN("data_init_runtime");
        data_init_runtime(arr, n, list_type);
        CALI_MARK_END("data_init_runtime");
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int local_size = n / size;
    local_arr = (int*)malloc(local_size * sizeof(int));

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");

    MPI_Scatter(arr, local_size, MPI_INT, local_arr, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");

    radixSort(local_arr, local_size);

    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    MPI_Gather(local_arr, local_size, MPI_INT, arr, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if (rank == 0) {
        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");

        radixSort(arr, n);

        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");

        std::cout << "Sorted data: ";
        for (int i = 0; i < std::min(n, 1000); i++) {
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;

        CALI_MARK_BEGIN("correctness_check");
        for (int i = 1; i < n; i++) {
            if (arr[i] < arr[i - 1]) {
                std::cerr << "Error: Array not sorted!\n";
                break;
            }
        }
        CALI_MARK_END("correctness_check");

        free(arr);
    }

    free(local_arr);

    CALI_MARK_END("main");

    MPI_Finalize();
    return 0;
}
