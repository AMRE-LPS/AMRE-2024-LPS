import matplotlib.pyplot as plt
from datetime import datetime
import subprocess
import pandas as pd

def cache_L2associativity_output_obtain():
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')

    cmd = "./cache_L2associativity_benchmark"

    filename = f'cache_L2asociativity_benchmark_data_{timestamp}.csv'

    # Prompt the user to optionally specify the --cache_linesize flag
    l1_size = input("Enter L1 size (in Bytes)(or press Enter to skip): ").strip()
    l2_size = input("Enter L2 size (in Bytes)(or press Enter to skip): ").strip()
    l1_associativity = input("Enter the associativity of L1 (or press Enter to skip): ").strip()
    cache_line_size = input("Enter cache line size (in Bytes)(or press Enter to skip): ").strip()

    # Build the command based on user's input
    cmd = ["Benchmark_Results/Executables/cache_L1associativity_benchmark"]
    if l1_size:
        cmd.append(f"--l1_size={l1_size}")
    elif cache_line_size:
        cmd.append(f"--cache_line_size={cache_line_size}")
    elif l2_size:
        cmd.append(f"--l2_size={l2_size}")
    elif l1_associativity:
        cmd.append(f"--l1_associativity={l1_associativity}")
    
    # Open a subprocess to run the C++ program and capture stdout
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running Cache L2 Associativity Benchmark: ")
 
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

    #Process the output by grouping them by associativity and calculate median
    chunk_list = []
    # Read the CSV file in chunks
    for chunk in pd.read_csv(filename, chunksize=10000):
        # Calculate the median access time for each associativity in the chunk
        chunk_median = chunk.groupby('associativity')['access_time'].median().reset_index()
        chunk_list.append(chunk_median)
    
    # Concatenate all chunk results
    median_access_times = pd.concat(chunk_list).groupby('associativity')['access_time'].median().reset_index()
    return median_access_times


def cache_L3associativity_output_obtain():
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')

    cmd = "./cache_L3associativity_benchmark"

    filename = f'cache_L3asociativity_benchmark_data_{timestamp}.csv'

    # Prompt the user to optionally specify the --cache_linesize flag
    l1_size = input("Enter L1 size (in Bytes)(or press Enter to skip): ").strip()
    l2_size = input("Enter L2 size (in Bytes)(or press Enter to skip): ").strip()
    l3_size = input("Enter L3 size (in Bytes)(or press Enter to skip): ").strip()

    l1_associativity = input("Enter the associativity of L1 (or press Enter to skip): ").strip()
    l2_associativity = input("Enter the associativity of L2 (or press Enter to skip): ").strip()

    cache_line_size = input("Enter cache line size (in Bytes)(or press Enter to skip): ").strip()

    # Build the command based on user's input
    cmd = ["Benchmark_Results/Executables/cache_L1associativity_benchmark"]
    if l1_size:
        cmd.append(f"--l1_size={l1_size}")
    elif cache_line_size:
        cmd.append(f"--cache_line_size={cache_line_size}")
    elif l2_size:
        cmd.append(f"--l2_size={l2_size}")
    elif l1_associativity:
        cmd.append(f"--l1_associativity={l1_associativity}")
    elif l2_associativity:
        cmd.append(f"--l2_associativity={l2_associativity}")
    elif l3_size:
        cmd.append(f"--l3_size={l3_size}")
    
    # Open a subprocess to run the C++ program and capture stdout
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running Cache L3 Associativity Benchmark: ")
 
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

    #Process the output by grouping them by associativity and calculate median
    chunk_list = []
    # Read the CSV file in chunks
    for chunk in pd.read_csv(filename, chunksize=10000):
        # Calculate the median access time for each associativity in the chunk
        chunk_median = chunk.groupby('associativity')['access_time'].median().reset_index()
        chunk_list.append(chunk_median)
    
    # Concatenate all chunk results
    median_access_times = pd.concat(chunk_list).groupby('associativity')['access_time'].median().reset_index()
    return median_access_times


def cache_L1associativity_output_obtain():
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')

    #cmd = "./cache_L1associativity_benchmark"

    filename = f'cache_L1asociativity_benchmark_data_{timestamp}.csv'

    # Prompt the user to optionally specify the --cache_linesize flag
    l1_size = input("Enter L1 size (in Bytes)(or press Enter to skip): ").strip()
    cache_line_size = input("Enter cache line size (in Bytes)(or press Enter to skip): ").strip()

    # Build the command based on user's input
    cmd = ["./cache_L1associativity_benchmark"]
    if l1_size:
        cmd.append(f"--l1_size={l1_size}")
    elif cache_line_size:
        cmd.append(f"--cache_line_size={cache_line_size}")
    
    # Open a subprocess to run the C++ program and capture stdout
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running Cache L1 Associativity Benchmark: ")
 
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

    #Process the output by grouping them by associativity and calculate median
    chunk_list = []
    # Read the CSV file in chunks
    for chunk in pd.read_csv(filename, chunksize=10000):
        # Calculate the median access time for each associativity in the chunk
        chunk_median = chunk.groupby('associativity')['access_time'].median().reset_index()
        chunk_list.append(chunk_median)
    
    # Concatenate all chunk results
    median_access_times = pd.concat(chunk_list).groupby('associativity')['access_time'].median().reset_index()
    return median_access_times


def cache_l1associativity_detection():
    #Obtaining output from C++ benchmark
    data = cache_L1associativity_output_obtain()

    #Visualizing the output
    cache_associativity_output_visualization(data, "L1")
    #Print out prediction
    print("L1 associativity predicted is:", cache_associativity_output_calculation(data))


def cache_l2associativity_detection():
    #Obtaining output from C++ benchmark
    data = cache_L2associativity_output_obtain()

    #Visualizing the output
    cache_associativity_output_visualization(data, "L2")
    #Print out prediction
    print("L2 associativity predicted is:", cache_associativity_output_calculation(data))


def cache_l3associativity_detection():
    #Obtaining output from C++ benchmark
    data = cache_L3associativity_output_obtain()

    #Visualizing the output
    cache_associativity_output_visualization(data, "L3")
    #Print out prediction
    print("L3 associativity predicted is:", cache_associativity_output_calculation(data))


#Calculate Cache Associativity based on obtained output
def cache_associativity_output_calculation(data):
    data['change'] = data['access_time'].diff()
    data['change'] = data['change'].apply(lambda x: x if x > 0 else 0)
    greatest_change_step = data['change'].idxmax()
    if greatest_change_step > 0:
        return data['associativity'][greatest_change_step-1]
    else:
        return "No significant change detected."

def cache_associativity_output_visualization(data, cache_lvl):
    plt.plot(data['associativity'], data['access_time'], marker='o', linestyle='-', markersize=3)
    plt.title(f'{cache_lvl}: Cache Associativity vs Median Access Time')
    plt.xlabel('Cache Associativity')
    plt.ylabel('Median Access Time')
    plt.grid(True)
    plt.xticks(data['associativity'])
    
    # Save the plot with a timestamp
    timestamp = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
    filename = f'cache_{cache_lvl}associativity_benchmark_graph_{timestamp}.png'
    plt.savefig(filename)
    plt.clf()

if __name__ == '__main__':
    
    cache_l1associativity_detection()
    cache_l2associativity_detection()
    cache_l3associativity_detection()
