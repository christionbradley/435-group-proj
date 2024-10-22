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

#include <vector>
#include <numeric>

void printLocalArr(int* local_list, int local_list_size, int rank) {
	std::cout << "\nLocal list for process " << rank << ":\n[";
	for(int i = 0; i < local_list_size;i++) {
		std::cout << local_list[i] << ", ";	
	}
	std::cout << "]\n";
}

void sample_sort(int* arr, int& local_size, int num_tasks, int rank) {
	// ~ sort local array
	std::sort(arr, arr + local_size);
	// std::cout << "---SORTED LOCAL ARRAY---";
	// printLocalArr(arr, local_size, rank);	

	// ~ select splitters
	int num_splitters = num_tasks - 1;
	int* local_splitters = (int*)malloc(num_splitters * sizeof(int));	
	for(int i = 0; i < num_splitters; i++) {
		local_splitters[i] = arr[(i+1) * local_size / num_tasks];
	}

	int* global_splitters = nullptr;
	if (rank == 0) {
		global_splitters = (int*)malloc(num_splitters * num_tasks * sizeof(int));
	}

	MPI_Gather(local_splitters, num_splitters, MPI_INT, global_splitters, num_splitters, MPI_INT, 0, MPI_COMM_WORLD);

	int* final_splits = (int*)malloc(num_splitters*sizeof(int));
	if (rank == 0) {
		std::sort(global_splitters, global_splitters+num_splitters*num_tasks);
		// std::cout << "Sorted global splitters to choose from: ";
		// printLocalArr(global_splitters, num_splitters*num_tasks, rank);
		// std::cout << "total number of splitters: " << num_splitters << "\nSplit Calcs: ";
		for(int i=0;i<num_splitters;i++) {
			// std::cout << (i+1) * num_tasks << ", ";
			final_splits[i] = global_splitters[(i+1) * (num_tasks - 1)];
		}
		// std::cout << "Final array of splitters: ";
		// printLocalArr(final_splits, num_splitters, rank);
	}

	// ~ broadcast final split values to all processes to be used in partitioning
	MPI_Bcast(final_splits, num_splitters, MPI_INT, 0, MPI_COMM_WORLD);

	// ? partition array based on final split values
	std::vector<int> partitions[num_tasks];
	int idx = 0;
	for (int i = 0; i < local_size; i++) {
		while (idx < num_splitters && arr[i] > final_splits[idx]) {
			idx++;
		}
		partitions[idx].push_back(arr[i]);
	}

	// ? send and receive data from various processes to correctly populate with correct values
	int* sendcounts = (int*)malloc(num_tasks * sizeof(int));
	int* recvcounts = (int*)malloc(num_tasks * sizeof(int));
	int* sdispls = (int*)malloc(num_tasks * sizeof(int));
	int* rdispls = (int*)malloc(num_tasks * sizeof(int));

	for(int i = 0; i < num_tasks; i++) {
		sendcounts[i] = partitions[i].size();
	}

	MPI_Alltoall(sendcounts, 1, MPI_INT, recvcounts, 1, MPI_INT, MPI_COMM_WORLD);

	int total_recv = 0;
	sdispls[0] = 0;
	rdispls[0] = 0;
	for (int i = 0; i < num_tasks; i++) {
		total_recv += recvcounts[i];
		if (i > 0) {
			sdispls[i] = sdispls[i - 1] + sendcounts[i - 1];
			rdispls[i] = rdispls[i - 1] + recvcounts[i - 1];
		}
	}

	int* sendbuf = (int*)malloc(local_size * sizeof(int));
	int* recvbuf = (int*)malloc(total_recv * sizeof(int));

	for (int i = 0; i < num_tasks; i++) {
		std::copy(partitions[i].begin(), partitions[i].end(), sendbuf + sdispls[i]);
	}

	int result = MPI_Alltoallv(sendbuf, sendcounts, sdispls, MPI_INT, recvbuf, recvcounts, rdispls, MPI_INT, MPI_COMM_WORLD);

	if (result != MPI_SUCCESS) {
		std::cout << "alltoallv failed\n";
	}

	std::sort(recvbuf, recvbuf+total_recv);
	// std::cout << "---FINAL LOCAL ARR---";
	// printLocalArr(recvbuf, total_recv, rank);

	arr = (int*)realloc(arr, total_recv*sizeof(int));
	std::copy(recvbuf, recvbuf+total_recv, arr);

	local_size = total_recv;
	printf("[%d] New local size: %d\n", rank, local_size)

	// ? clean up memory
	if (rank == 0) {
		free(global_splitters);
	}
	free(local_splitters);
	free(final_splits);
	free(sendcounts);
	free(recvcounts);
	free(sdispls);
	free(rdispls);
	free(sendbuf);
	free(recvbuf);
}

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

    int* og_data = nullptr;
    if (rank == 0) {
        og_data = (int*)malloc(list_size * sizeof(int));
        CALI_MARK_BEGIN("data_init_runtime");
        data_init_runtime(og_data, list_size, list_type);
        CALI_MARK_END("data_init_runtime");

	std::cout << "List before sample sort:[";
	for (int i = 0; i < std::min(list_size, 1000); i++) {
		std::cout << og_data[i] << " ";
	}
	std::cout << "]\n";
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "sample sort"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
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
    
	if (rank == 0) {
		free(og_data);
		og_data = nullptr;
	}

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");

    sample_sort(local_list, local_list_size, num_tasks, rank);

    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");

    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

	int* all_local_list_sizes = nullptr;
	int* displs = nullptr;
	if (rank == 0) {
		all_local_list_sizes = (int*)malloc(num_tasks*sizeof(int));
		displs = (int*)malloc(num_tasks * sizeof(int));
	}

	MPI_Gather(&local_list_size, 1, MPI_INT, all_local_list_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		displs[0] = 0;
		int total_size = all_local_list_sizes[0];
		for (int i=1;i<num_tasks;i++) {
			displs[i] = displs[i-1] + all_local_list_sizes[i-1];
			total_size += all_local_list_sizes[i];
		}
		// std::cout << "fml: " << total_size << "\n";
		// for(int i=0;i<num_tasks;i++) {
		// 	std::cout << all_local_list_sizes[i] << " ";
		// }
		// std::cout << "\n";
		og_data = (int*)malloc(total_size * sizeof(int));
		
		// gather all the locally sorted list back into og
		MPI_Gatherv(local_list, local_list_size, MPI_INT, og_data, all_local_list_sizes, displs, MPI_INT, 0, MPI_COMM_WORLD);
	}

    if (rank == 0) {
	    std::cout << "\n---\nList after sample sort:[";
	    for (int i = 0; i < std::min(list_size, 1000); i++) {
		    std::cout << og_data[i] << " ";
	    }
	    std::cout << "]\n";

		free(all_local_list_sizes);
		free(displs);
	    free(og_data);
    }
    free(local_list);
    CALI_MARK_END("main");
    MPI_Finalize();
    return 0;
}


