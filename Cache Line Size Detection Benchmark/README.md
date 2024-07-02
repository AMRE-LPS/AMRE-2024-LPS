# Cache Line Size Detection Tool

## Description
This is a micro-benchmarking tool utilizing the concept of **false sharing** in order to detect the cache line size of any system. Theoretically, it is capable of detecting in the range from 16 to 4096 Byte in cache line size. More details about this work can be found in the final paper report

## Limitation 
Due to the subtleties associated with false sharing and limited experimentation facilities, it has only been tested on a select collection of machine. In order to be eligible for actual use, future modifications must be considered. 

## Usage
1. Create the virtual environment: 'python3 -m venv myenv'
2. Activate the virtual environment: 'source myenv/bin/activate'
3. Install the required dependencies: 'pip3 install -r requirements.txt'
4. Run the provided *Makefile* to compile the C++ programs: 'make all'
5. Run the python script, this would store the outputs in csv files\, generate the graph and give out predictions: 'python3 cache_linesize_benchmark.py'

### Note
To facilitate modularity, the benchmark itself can be run alone using the C++ file or 'make run'. It should be noted that it would only print the measurements to the stdout in csv format, the actual calculation and prediction are done with *cache_linesize_benchmark.py*
