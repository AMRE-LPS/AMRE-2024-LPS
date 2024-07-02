#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <string.h>

#define MEM_SIZE ((size_t)1024 * 1024 * 1024 * 3) //size of the test array (in bytes)
#define NUM_ITERATIONS 100000 // number of iterations in each test

// Structure with size 64 Bytes (size of cache line)
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

// Returns a malloc'd array of size MEM_SIZE with 64 Byte elements of struct int64byte_t
int64byte_t *allocate_benchmark_memory(size_t mem_size) {
    int64byte_t *arr = malloc(mem_size); 

    for (size_t i = 0; i < mem_size / sizeof(int64byte_t); i++) {
        arr[i].a = i;
    }
    return arr; 
}

// CPU/compiler optimization and pre-fetching inhibitor
void memory_barrier() {
    __asm__ __volatile__ ("" : : : "memory");
    _mm_mfence();
} __attribute__((optimize(0)))

// Removes elements of arr from all cache levels
void clear_cache(int64byte_t *arr, size_t *indices, int indices_arr_size) { 
    memory_barrier();
    for (size_t i = 0; i < indices_arr_size; i++) {
        _mm_clflush(&arr[indices[i]]); // x86 instruction
    }
} __attribute__((optimize(0)))

// Returns an array of indices for the L1 test
size_t *generate_L1_indices(int test_associativity, int *arr_size, size_t l1_size, size_t cache_line_size) {
    // Calculate the number of cache lines in L1 using the L1 size and the cache line size
    size_t l1_cache_lines = l1_size / cache_line_size;

    // Create array to store indices
    *arr_size = test_associativity;
    size_t *arr = malloc((*arr_size) * sizeof(size_t));

    // Populate index array 
    for (size_t i = 0; i < test_associativity; i++) {
        arr[i] = i * l1_cache_lines; // Ensures that indexed elements get mapped to the same set in L1
    }

    return arr;
}

// Measures access times for L1 test 
void run_L1_associativity_benchmark(int64byte_t *mem, size_t l1_size, size_t cache_line_size) {
    printf("associativity,element_index,access_time\n");

    unsigned int aux; // Temp variable for __rdtscp()
    
    // Run the tests for each associativity
    for (int i = 2; i <= 24; i += 2) { // Associativities to test
        // Create array for test indices using generate_L1_indices() for each associativity
        int test_indices_arr_size;
        size_t *test_indices = generate_L1_indices(i, &test_indices_arr_size, l1_size, cache_line_size);
        
        // Run test NUM_ITERATIONS times for each test associativity
        for (int j = 0; j < NUM_ITERATIONS; j++) {
            // Load the elements into the target set
            memory_barrier();
            for (int k = 0; k < test_indices_arr_size; k++) {
                memory_barrier();
                volatile int16_t temp = mem[test_indices[k]].a; // Read operation to load into cache
            }

            // Measure access time
            memory_barrier();
            for (int k = 0; k < test_indices_arr_size; k++) {
                memory_barrier();
                size_t start_t = __rdtscp(&aux); // x86 instruction to read time stamp counter register
                volatile int16_t temp = mem[test_indices[k]].a;
                size_t end_t = __rdtscp(&aux);
                printf("%d,%d,%ld\n", i, k, (end_t - start_t));
            }
            
            // Clear the cache of all accessed elements for next iteration
            memory_barrier();
            clear_cache(mem, test_indices, test_indices_arr_size);  
        }
        free(test_indices);
    }
} __attribute__((optimize(0)))

int main(int argc, char *argv[]) {

    size_t l1_size = 256 * 1024; //default L1d size
    size_t cache_line_size = 64; //default cache line size
 
     for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--l1_size=", 10) == 0) {
            char *arg_value = argv[i] + 10;
            if (strlen(arg_value) > 0) {
                l1_size = atoi(arg_value);
            }
        } else if (strncmp(argv[i], "--cache_line_size=", 18) == 0) {
            char *arg_value = argv[i] + 18;
            if (strlen(arg_value) > 0) {
                cache_line_size = atof(arg_value);
            }
        }
    }

    // Allocate memory for the benchmark
    int64byte_t *benchmark_memory = allocate_benchmark_memory(MEM_SIZE);

    // Run the L1 associativity benchmark
    run_L1_associativity_benchmark(benchmark_memory, l1_size, cache_line_size);

    // Free the allocated memory
    free(benchmark_memory);

    return 0;
}
