import subprocess
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

# Get the current timestamp for the file name
timestamp = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')

# Get output of Cache Line Size Detection Program and store in a csv
def cache_linesize_output_obtain():
    cmd = "./cache_linesize_benchmark"
    
    filename = f'cache_linesize_benchmark_data_{timestamp}.csv'
    
    # Open a subprocess to run the C++ program and capture stdout 
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running Cache Line Size Detection Benchmark. Should take 15-20 seconds: ")

    # Open a CSV file to write the output data
    with open(filename, 'w', newline='') as csvfile:
        # Read and handle the output from the C++ program
        while True:
            output = process.stdout.readline()
        
            if output == '' and process.poll() is not None:
                break
        
            csvfile.write(output)
            csvfile.flush()

    # Close the subprocess
    print()
    process.stdout.close()
    process.wait()
    
    return pd.read_csv(filename)

#Calculate Cache Line Size based on obtained output
def cache_linesize_output_calculation(data):
    keys = list(data.keys())
    #Detect where there is at least a 70% drop in access time
    for i in range(len(keys) - 1):
        if data[keys[i]] >= 1.7 * data[keys[i + 1]]:
            return keys[i]
    #If no drop detected, indicating a flat line returning the biggest size
    return 4096

#Visualization of Cache Line Size data
def cache_linesize_output_visualization(data):
    # Plot the results
    plt.figure(figsize=(10, 6))
    plt.plot(list(data.keys()), list(data.values()), marker='o', linestyle='-', color = "blue", label = "Median")
    plt.xlabel('Cache Line Size (Bytes)')
    plt.ylabel('Time (Milliseconds)')
    plt.title('Cache Line Size Graph: Access Time for different Sizes')
    plt.legend()
    plt.grid(True)
    plt.xscale('log', base=2)  # Use a logarithmic scale for the x-axis if needed
    filename = f'cache_linesize_benchmark_graph_{timestamp}.png'
    plt.savefig(filename)
    plt.close()

#Function to run prediction of cache line size
def cache_linesize_detection():
    #Obtaining output from C++ benchmark
    output = cache_linesize_output_obtain()
    #Process the output by grouping them by strides and median access time
    processed_data = output.groupby(output.columns[0])[output.columns[1]].median().to_dict()
    #Visualizing the output
    cache_linesize_output_visualization(processed_data)
    #Print out prediction
    print("Cache Line Size predicted is:",cache_linesize_output_calculation(processed_data))
    
cache_linesize_detection()
