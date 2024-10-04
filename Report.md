# CSCE 435 Group project

## 0. Group number: 12

## 1. Group members:
1. Christion Bradley
2. Warren Wu
3. Sua Bae
4. Kunal Jain

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort: Bitonic sort repeatedly creates bitonic sequences, which first monotonically increases and then monotonically decreases, then merges them into a fully sorted sequence through performing pairwise comparisons and swaps between elements. 
This sorting algorithm will be using MPI on TAMU Grace Cluster by dividing the array into multiple processes, locally sorting the subarrays within each process, then finally merging them in a parallel fashion, until the entire array is sorted.
- Sample Sort:
- Merge Sort: For a list of size n, Merge Sort recursively splits the list into n sublists, where each sublist contains a single element and is therefore sorted. These sublists are then combined into larger lists where smaller elements appear before larger elements.
This is done until one list remains, which will be the final sorted list. We will be implementing this algorithm on the TAMU Grace Cluster using MPI to communicate between processes. 
- Radix Sort:

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes
```
Bitonic Sort:

// Recursively sorts a bitonic sequence in ascending order if dir==1,
// and in descending order if dir==0.
void bitonic_merge(int a[], int low, int cnt, int dir)
{
  if (cnt > 1)
  {
    int k = cnt / 2;
    for (int i = low; i < low+k; i++)
      if (dir == (a[i] > a[i+k])) { // if out of order, swap elements
        int temp = arr[i];
        arr[i] = arr[i + k];
        arr[i + k] = temp;
      }
    bitonic_merge(a, low, k, dir);
    bitonic_merge(a, low+k, k, dir);
  }
}

// Generates a bitonic sequence by recursively sorting the first half in an 
// ascendng order and the second half in the descending order 
// & then merges the two together
void bitonic_sort(int a[], int low, int cnt, int dir)
{
  if (cnt > 1)
  {
    int k = cnt / 2;
    bitonic_sort(a, low, k, 1);
    bitonic_sort(a, low+k, k, 0);
    bitonic_merge(a, low, cnt, dir);
  }
}

int main() {
  int rank, size, n_workers, n;
  int* arr = array to be sorted;
  int* workers_arr = NULL; // Local sub-array for each worker
  int* final_list = NULL;  // Final sorted array on master process

  MPI_Init();
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &n_workers);

  // Initialize the array
  if (this is the master process) {
    n = sizeof(arr) / sizeof(arr[0]);  // get array size
  }

  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD); // broadcast array size to all processes
  int local_size = n / n_workers; // calculate the size of subarray for each worker
  workers_arr = (int*)malloc(local_size * sizeof(int)); // allocate memory

  MPI_Scatter(arr, local_size, MPI_INT, workers_arr, local_size, MPI_INT, 0, MPI_COMM_WORLD); // distribute subarrays to each worker process
  bitonic_sort(workers_arr, 0, local_size, 1); // perform bitonic sort within each worker process

  if (this is the master process) {
      final_list = (int*)malloc(n * sizeof(int));
  }

  MPI_Gather(workers_arr, local_size, MPI_INT, final_list, local_size, MPI_INT, 0, MPI_COMM_WORLD); // gather all subarrays

  if (this is the master process) {
    bitonic_sort(final_list, 0, n, 1); // final bitonic sort on the entire array
    free(final_list);
  }

  free(workers_arr);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
```
```
Merge Sort: 

void merge(arr, temp, left, right){
  int b = left;
  int l = left;
  int middle = ((left + right) / 1);
  int r = middle + 1;

  while ((l <= middle && r <= right) {
      if (arr[l] <= arr[r]) {
      temp[b] = arr[l]; l += 1;
    } else {
      temp[b] = arr[r]; r += 1;
    }
    b_idx += 1;
  }

  if (middle < left) { // still values in right array
      for i from r to right {
          temp[b_idx] = arr[i]; b_idx += 1;
      }
  } else { // still values in the left array
      for i from l to middle {
                temp[b_idx] = arr[i]; b_idx += 1;
      }
}

  for i from to right {
      a[i] = temp[i];
  }
  
}
    
}
void merge_sort(int* arr, int* temp, int left, int right){
  if (left < right) {
  middle = (left + right) // 2;
  merge_sort(arr1, arr2, left, middle);
  merge_sort(arr1, arr2, middle + 1, right);
  merge(arr1, arr2, left, right);
} 
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

  if (this is master process) {
    final_list = malloc(arr.size() * sizeof(int));
   }

MPI_Gather(worker_arr, size, MPI_INT, final_list, size, MPI_INT, 0, MPI_COMM_WORLD);

  if (this is master process) {
  int* helper_arr malloc(arr.size() * sizeof(int));
  mergeSort(final_list, helper_arr, 0, arr.size() - 1);
  print(final_list);

  free(final_list); free(helper_arr);
  
}

MPI_Barrier(MPI_COMM_WORLD);
MPI_Finalize();

}

```
  
  
  

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
