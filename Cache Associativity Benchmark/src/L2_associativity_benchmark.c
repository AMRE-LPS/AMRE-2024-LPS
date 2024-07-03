#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <string.h>

#define MEM_SIZE ((size_t)1024 * 1024 * 1024 * 3) //size of the test array (in bytes)
#define NUM_ITERATIONS 100000 // number of iterations in each test

// Structure with size 64 Bytes (size of cache line). Is used as the elements of the test array.
typedef struct {
    volatile int64_t a;
    volatile int64_t b;
    volatile int64_t c;
    volatile int64_t d;
    volatile int64_t e;
    volatile int64_t f;
    volatile int64_t g;
    volatile int64_t h;
} __attribute__((aligned(64))) int64byte_t;

// Returns a malloc array of size mem_size with 64 Byte elements of struct int64byte_t
int64byte_t *allocate_benchmark_memory(size_t mem_size) {
    int64byte_t *arr = malloc(mem_size);

    //initilizes all the elements of arr
    for (size_t i = 0; i < mem_size / sizeof(int64byte_t); i++) {
        arr[i].a = i;
    }
    return arr;
}

// Wrapper for two CPU/compiler optimization and pre-fetching inhibitors
void memory_barrier() {
    __asm__ __volatile__("" : : : "memory");
    _mm_mfence();
} __attribute__((optimize(0)))

// Removes elements in arr indexed by indices from all cache levels using an x86 clflush instruction
void clear_cache(int64byte_t *arr, size_t *indices, int indices_arr_size) {
    memory_barrier();
    for (size_t i = 0; i < indices_arr_size; i++) {
        _mm_clflush(&arr[indices[i]]); // x86 instruction
    }
} __attribute__((optimize(0)))


// Returns an array of indices for the L2 with size test_associativity + l1_associativity. Uses l2_size and cache_line_size to determine the stride of the indices for the first 
// test_associativity elements. Uses l1_size and cache_line_size to determine the next l1_associativity elements. Assigns the size of the returned array to *arr_size
size_t *generate_L2_indices(int test_associativity, int *arr_size, size_t l1_size, size_t l2_size, int l1_assoc, size_t cache_line_size) {
    
    // Calculate the number of cache lines in L1 and L2 
    size_t l1_cache_lines = l1_size / cache_line_size;
    size_t l2_cache_lines = l2_size / cache_line_size;

    //calculate the number of sets in each cache given function parameters
    size_t l1_num_sets = l1_cache_lines / l1_assoc;
    size_t l2_test_num_sets = l2_cache_lines / test_associativity;

    // Create array to store indices
    *arr_size = l1_assoc + test_associativity;
    size_t *arr = malloc((*arr_size) * sizeof(size_t));

    // Populate arr with indexes of elements that get mapped to same set in L1 & L2
    int index = 0;
    for (int i = 0; i < (MEM_SIZE / cache_line_size); i++) {
        if ((i % l2_cache_lines == 0) && (i % l1_num_sets == 0)) { 
            arr[index++] = i;
        }
        if (index >= test_associativity) {
            break;
        }
    }

    // Populate rest of arr with indexes of elements that get mapped to the same set in L1, but a different set in L2
    int index2 = test_associativity;
    for (int i = 0; i < (MEM_SIZE / cache_line_size); i++) {
        if ((i % l1_num_sets == 0) && (i % l2_test_num_sets != 0)) { 
            arr[index2++] = i;
        }
        if (index2 >= *arr_size) {
            break;
        }
    }

    return arr;
} __attribute__((optimize(0)))

// Measures access times for L2 test and outputs results into a CSV file. mem is the test array of size MEM_SIZE.
void run_L2_associativity_benchmark(int64byte_t *mem, size_t l1_size, size_t l2_size, int l1_assoc, size_t cache_line_size) {
    
    //create output csv file and print column names
    FILE* output_file = fopen("cache_L2associativity_benchmark_data.csv", "w");
    fprintf(output_file, "associativity,element_index,access_time\n");

    unsigned int aux; // Temp variable for __rdtscp()

    // ouput the access times for each associativity to be tested by creating an array of indices, iterating over it once it load into the target set, and once more to measure the time
    for (int i = 2; i <= 24; i += 2) {

        // Create array for test indices using generate_L2_indices() for each associativity
        int test_indices_arr_size;
        size_t *test_indices = generate_L2_indices(i, &test_indices_arr_size, l1_size, l2_size, l1_assoc, cache_line_size);

        // Run test NUM_ITERATIONS times for each test associativity
        for (int j = 0; j < NUM_ITERATIONS; j++) {

            // Load the elements into the target set by reading the elemnts at indices in test_indices
            memory_barrier();
            for (int k = 0; k < test_indices_arr_size; k++) {
                memory_barrier();
                volatile int16_t temp = mem[test_indices[k]].a;
            }

            // Measure access time of the unique elements in L2 (first test_associativity elements in test_indices) using an x86 instruction to read time stamp counter register
            memory_barrier();
            for (int k = 0; k < i; k++) {
                memory_barrier();
                size_t start_t = __rdtscp(&aux);
                volatile int16_t temp = mem[test_indices[k]].a;
                size_t end_t = __rdtscp(&aux);
                fprintf(output_file, "%d,%d,%ld\n", i, k, (end_t - start_t));
            }

            // Clear the cache of all accessed elements for next iteration using clear_cache()
            memory_barrier();
            clear_cache(mem, test_indices, test_indices_arr_size);
        }
        free(test_indices);
    }
    fclose(output_file);
} __attribute__((optimize(0)))

int main(int argc, char *argv[]) {

    size_t l1_size = 256 * 1024; //default L1d size
    size_t l2_size = 5 * 1024 * 1024; //default L2 size
    int l1_associativity = 10; //default L1d associativity
    size_t cache_line_size = 64; //default cache line size (must be the same as the alignment of int64byte_t structure)
    
    //properly assigns a value to above variables if a flag is set when running the program
     for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--l1_size=", 10) == 0) {
            char *arg_value = argv[i] + 10;
            if (strlen(arg_value) > 0) {
                l1_size = atoi(arg_value);
            }
        } else if (strncmp(argv[i], "--l2_size=", 10) == 0) {
            char *arg_value = argv[i] + 10;
            if (strlen(arg_value) > 0) {
                l2_size = atof(arg_value);
            }
        } else if (strncmp(argv[i], "--l1_associativity=", 19) == 0) {
            char *arg_value = argv[i] + 19;
            if (strlen(arg_value) > 0) {
                l1_associativity = atof(arg_value);
            }
        }
    }

    // Allocate memory for the benchmark of size MEM_SIZE
    int64byte_t *benchmark_memory = allocate_benchmark_memory(MEM_SIZE);

    // Run the L2 associativity benchmark with the created malloc array
    run_L2_associativity_benchmark(benchmark_memory, l1_size, l2_size, l1_associativity, cache_line_size);

    // Free the allocated memory
    free(benchmark_memory);

    return 0;
}
