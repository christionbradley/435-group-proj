# CSCE 435 Group project

## 0. Group number: 12

## 1. Group members:
1. Christion Bradley
2. Warren Wu
3. Sua Bae
4. Kunal Jain

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
- Merge Sort: For a list of size n, Merge Sort recursively splits the list into n sublists, where each sublist contains a single element and is therefore sorted. These sublists are then combined into larger lists where smaller elements appear before larger elements.
This is done until one list remains, which will be the final sorted list. We will be implementing this algorithm on the TAMU Grace Cluster using MPI to communicate between processes. 
- Radix Sort:

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

Merge Sort: 

void merge(arr, left, right){
}
void merge_sort(arr){
}
int main() {
  arr = array of elements to be sorted;
  MPI_Init();
  MPI_Comm_rank(...);
  MPI_Comm_size(...);

  int size = arr.size() / n_workers;
  int *workers_arr = malloc(size * sizeof(int)); // portion of array for worker to sort
  MPI_Scatter(arr, size, MPI_INT, workers_arr, size, MPI_INIT, 0, MPI_COMM_WORLD);

  int *w_arr = malloc(size * sizeof(int)); // portion of array for worker to sort
  mergeSort(workers_arr, w_arr, 0, size - 1);

  
  
  

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)