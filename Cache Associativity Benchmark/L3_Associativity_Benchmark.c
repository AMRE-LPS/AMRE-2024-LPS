#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MEM_SIZE ((size_t)1024 * 1024 * 1024 * 3) //size of the test array (in bytes)
#define NUM_ITERATIONS 1000000 // number of iterations in each test

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
    __asm__ __volatile__("" : : : "memory");
    _mm_mfence();
} __attribute__((optimize(0)))

// Removes elements of arr from all cache levels
void clear_cache(int64byte_t *arr, size_t *indices, int indices_arr_size) {
    memory_barrier();
    for (size_t i = 0; i < indices_arr_size; i++) {
        _mm_clflush(&arr[indices[i]]); // x86 instruction
    }
} __attribute__((optimize(0)))

//returns an array of indicies for L3 test
size_t *generate_L3_indices(int test_associativity, int *arr_size, size_t l1_size, size_t l2_size, size_t l3_size, int l1_assoc, int l2_assoc, size_t cache_line_size) {
    size_t l1_cache_lines = l1_size / cache_line_size;
    size_t l2_cache_lines = l2_size / cache_line_size;
    size_t l3_cache_lines = l3_size / cache_line_size;
    
    size_t l1_num_sets = l1_cache_lines / l1_assoc;
    size_t l2_num_sets = l2_cache_lines / l2_assoc;
    size_t l3_test_num_sets = l3_cache_lines / test_associativity;

    //create array
    *arr_size = MAX(L2_ASSOCIATIVITY, L1d_ASSOCIATIVITY) + test_associativity;
    size_t *arr = malloc((*arr_size) * sizeof(size_t));

    int index = 0;
    for (int i = 0; i < (MEM_SIZE / cache_line_size); i++) {
        if ((i % l3_cache_lines == 0) && (i % l1_num_sets == 0) && (i % l2_num_sets)) { //each element gets mapped to same sets in L1, L2, and L3
            arr[index++] = i;
        }
        if (index >= test_associativity) {
            break;
        }
    }

    //override values in L1 and L2
    size_t index1 = test_associativity;
    for (size_t i = 0; i < (MEM_SIZE / 64); i++) {
        if ((i % l1_num_sets == 0) && (i % l2_num_sets == 0) && (i % l3_test_num_sets != 0)) { //each element gets mapped to same set in L1 & L2, but different L3 set
            arr[index1++] = i;
        }
        if (index1 >= *arr_size) {
             break;
        }
    }

    return arr;
} __attribute__((optimize(0)))
//measures access times for L3 test 
void run_L3_associativity_benchmark(int64byte_t *mem, size_t l1_size, size_t l2_size, size_t l3_size, int l1_assoc, int l2_assoc, size_t cache_line_size) {

    // FILE* output_file = fopen("cache_associativity_L3_data.csv", "w");
    // fprintf(output_file, "associativity,element_index,access_time\n");

    printf("associativity,element_index,access_time\n");

    unsigned int aux; //temp variable for __rdtscp()

    for (int i = 2; i < 25; i += 2) {

        //create array for test indicies
        int test_indices_arr_size;
        size_t *test_indices = generate_L3_indices(i, &test_indices_arr_size, l1_size, l2_size, l3_size, l1_assoc, l2_assoc, cache_line_size);

        //run test
        for (size_t j = 0; j < NUM_ITERATIONS; j++) {

            //load elements into the caches
            memory_barrier();
            for (size_t k = 0; k < test_indices_arr_size; k++) {
                memory_barrier();
                volatile int16_t temp = mem[test_indices[k]].a;
            }
            
            //measure access times
            memory_barrier();
            for (int k = 0; k < i; k++) {
                memory_barrier();
                size_t start_t = __rdtscp(&aux);
                volatile int16_t temp = mem[test_indices[k]].a;
                size_t end_t = __rdtscp(&aux);
                printf("%d,%d,%ld\n", i, k, (end_t - start_t));
            }

            //clear cache
            memory_barrier();
            clear_cache(mem, test_indices, test_indices_arr_size);
        }
        free(test_indices);
    }
    //fclose(output_file);
} __attribute__((optimize(0)))

int main(int argc, char *argv[]) {

    
    size_t l1_size = 256 * 1024; //default L1d size
    size_t l2_size = 5 * 1024 * 1024; //default L2 size
    size_t l3_size = 10 * 1024 * 1024; //default L3 size
    int l1_associativity = 10; //default L1d associativity
    int l2_associativity = 12; //default L2 associativity
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

    // Allocate memory for the benchmark
    int64byte_t *benchmark_memory = allocate_benchmark_memory(MEM_SIZE);

    // Run the L3 associativity benchmark
    run_L3_associativity_benchmark(benchmark_memory, l1_size, l2_size, l3_size, l1_associativity, l2_associativity, cache_line_size);

    // Free the allocated memory
    free(benchmark_memory);

    return 0;
}
