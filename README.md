# Virtual Core Identification With Sysbench


## Description
Simple scripting program utilizing cpu test of sysbench and Piecewise Linear Regression to detect the number of virtual cores on a computer system. Here, we define virtual cores to be *processing units* available to the OS. It is an observed phenomenon that the computer system runs at its full potential on the benchmark when the number of threads equal to the number of virtual cores (the output when running the command *nproc --all*). Utilizing this pattern, we develop a program to estimate the number of virtual cores that includes the following steps:

- Obtaining the output as the number of events from *sysbench cpu --cpu-max-prime={num_prime} run --num-threads={thread_count}* with different cpu-max-prime and num-threads
- A 2-segment linear regression is applied for each set of data (grouped by cpu-max-prime, each color line on the graph) in order to calculate the *breakpoint* (the x-value of the intersection between 2 linear lines) which happens to be the number of threads.
- By default, there are 5 cases with varying *cpu-max-prime* arguments returning 5 results. The mode of those results is reported as the final answer.

## Limitation

The program has been tested on 1 Linux (Fedora) computer, 2 Window machines with WSL and 1 MacOS device. So far the result has been accurate with high level of confidence except in the following conditions:

- The machine is not running the program at full performance mode
- The cpu becomes too hot
- Program is run on Apple's M* chip

## Usage

The program is expected to run on Unix/Unix-like OS or Window with WSL. It is assumed that the system has [sysbench](https://github.com/akopytov/sysbench) installed. 

To run the program, execute the provided *threadbenchmark.sh* file. Should this does not work, please install the Python dependencies specified in *requirements.txt* and manually run *python3 threadcount.py* 

### Options

To modify the program to cater for specific use case, please head to *config.toml* for more details. In total, *sysbench* is run (*thread_count * prime_number*) times, each at *max_time* second. Here, there are three array parameters to change:

- *thread_count*: different number of virtual cores to test (extend this array for high-performance computer system)
- *prime_number*: different number limit for Prime Number Algorithm used in cpu test. The test will iteratively test if a number is prime from 3 to *prime_number* (extending this array creates more colored line in the graph) 
- *max_time*: time it takes to run each *sysbench* command