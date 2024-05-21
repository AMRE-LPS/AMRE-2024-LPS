#!/bin/bash

# Array of thread counts
thread_counts=(1 2 4 8 16 32)

# Iterate over the thread counts
for thread_count in "${thread_counts[@]}"; do
	echo "Running sysbench with ${thread_count} threads"
	sysbench --test=cpu --cpu-max-prime=20000 run --num-threads=${thread_count}
done
