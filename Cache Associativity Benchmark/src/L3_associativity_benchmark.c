#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

// Returns an array of indices for the L3 with size MAX(l2_assoc, l1_assoc) + test_associativity. Uses l3_size and cache_line_size to determine the stride of the indices for the first 
// test_associativity elements. Uses l1_size, l2_size, and cache_line_size to determine the next MAX(l2_assoc, l1_assoc) elements. Assigns the size of the returned array to *arr_size
size_t *generate_L3_indices(int test_associativity, int *arr_size, size_t l1_size, size_t l2_size, size_t l3_size, int l1_assoc, int l2_assoc, size_t cache_line_size) {
    size_t l1_cache_lines = l1_size / cache_line_size;
    size_t l2_cache_lines = l2_size / cache_line_size;
    size_t l3_cache_lines = l3_size / cache_line_size;
    
    size_t l1_num_sets = l1_cache_lines / l1_assoc;
    size_t l2_num_sets = l2_cache_lines / l2_assoc;
    size_t l3_test_num_sets = l3_cache_lines / test_associativity;

    // Create array to store indices
    *arr_size = MAX(l2_assoc, l1_assoc) + test_associativity;
    size_t *arr = malloc((*arr_size) * sizeof(size_t));

    // Populate arr with indexes of elements that get mapped to same set in L1 & L2 & l3
    int index = 0;
    for (int i = 0; i < (MEM_SIZE / cache_line_size); i++) {
        if ((i % l3_cache_lines == 0) && (i % l1_num_sets == 0) && (i % l2_num_sets)) { //each element gets mapped to same sets in L1, L2, and L3
            arr[index++] = i;
        }
        if (index >= test_associativity) {
            break;
        }
    }

    // Populate rest of arr with indexes of elements that get mapped to the same set in L1 & l2, but a different set in L3
    size_t index1 = test_associativity;
    for (size_t i = 0; i < (MEM_SIZE / 64); i++) {
        if ((i % l1_num_sets == 0) && (i % l2_num_sets == 0) && (i % l3_test_num_sets != 0)) { 
            arr[index1++] = i;
        }
        if (index1 >= *arr_size) {
             break;
        }
    }

    return arr;
} __attribute__((optimize(0)))


// Measures access times for L3 test and outputs results into a CSV file. mem is the test array of size MEM_SIZE.
void run_L3_associativity_benchmark(int64byte_t *mem, size_t l1_size, size_t l2_size, size_t l3_size, int l1_assoc, int l2_assoc, size_t cache_line_size) {

    //create output csv file and print column names
    FILE* output_file = fopen("cache_L3associativity_benchmark_data.csv", "w");
    fprintf(output_file, "associativity,element_index,access_time\n");

    unsigned int aux; //temp variable for __rdtscp()

    // ouput the access times for each associativity to be tested by creating an array of indices, iterating over it once it load into the target set, and once more to measure the time
    for (int i = 2; i < 25; i += 2) {

        // Create array for test indices using generate_L3_indices() for each associativity
        int test_indices_arr_size;
        size_t *test_indices = generate_L3_indices(i, &test_indices_arr_size, l1_size, l2_size, l3_size, l1_assoc, l2_assoc, cache_line_size);

        // Run test NUM_ITERATIONS times for each test associativity
        for (size_t j = 0; j < NUM_ITERATIONS; j++) {

            // Load the elements into the target set by reading the elemnts at indices in test_indices
            memory_barrier();
            for (size_t k = 0; k < test_indices_arr_size; k++) {
                memory_barrier();
                volatile int16_t temp = mem[test_indices[k]].a;
            }
            
            // Measure access time of the unique elements in L3 (first test_associativity elements in test_indices) using an x86 instruction to read time stamp counter register
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
    size_t l3_size = 10 * 1024 * 1024; //default L3 size
    int l1_associativity = 10; //default L1d associativity
    int l2_associativity = 12; //default L2 associativity
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
        } else if (strncmp(argv[i], "--l3_size=", 10) == 0) {
            char *arg_value = argv[i] + 10;
            if (strlen(arg_value) > 0) {
                l3_size = atof(arg_value);
            }
        } else if (strncmp(argv[i], "--l1_associativity=", 19) == 0) {
            char *arg_value = argv[i] + 19;
            if (strlen(arg_value) > 0) {
                l1_associativity = atof(arg_value);
            }
        } else if (strncmp(argv[i], "--l2_associativity=", 19) == 0) {
            char *arg_value = argv[i] + 19;
            if (strlen(arg_value) > 0) {
                l2_associativity = atof(arg_value);
            }
        }
    }

    // Allocate memory for the benchmark of size MEM_SIZE
    int64byte_t *benchmark_memory = allocate_benchmark_memory(MEM_SIZE);

    // Run the L3 associativity benchmark with the created malloc array
    run_L3_associativity_benchmark(benchmark_memory, l1_size, l2_size, l3_size, l1_associativity, l2_associativity, cache_line_size);

    // Free the allocated memory
    free(benchmark_memory);

    return 0;
}
