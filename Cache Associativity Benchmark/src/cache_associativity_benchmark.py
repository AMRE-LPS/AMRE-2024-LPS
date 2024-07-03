import matplotlib.pyplot as plt
from datetime import datetime
import subprocess
import os
import pandas as pd

#Runs the compiled C program for the L1 test using subprocess, asking the user for necessary informtaion to set the flags, and reads the created csv file and renames the file with a timestamp
#Returns a pandas object that contains the median access times for each associativity tested in the L1 test
def cache_L1associativity_output_obtain():
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')

    filename = "cache_L1associativity_benchmark_data.csv"

    # Prompt the user to optionally specify the --cache_linesize flag
    l1_size = input("Enter L1 size (in Bytes)(or press Enter to skip): ").strip()

    # Build the command based on user's input
    cmd = ["./cache_L1associativity_benchmark"]
    if l1_size:
        cmd.append(f" --l1_size={l1_size}")
    
    # Open a subprocess to run the C program 
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running benchmark: ")
    process.wait()
    
    # Read the CSV file in chunks
    chunk_list = []
    for chunk in pd.read_csv(filename, chunksize=10000):
        # Calculate the median access time for each associativity in the chunk
        chunk_median = chunk.groupby('associativity')['access_time'].median().reset_index()
        chunk_list.append(chunk_median)
   
    # Concatenate all chunk results
    median_access_times = pd.concat(chunk_list).groupby('associativity')['access_time'].median().reset_index()
    
    #saves data as a timestamped csv file
    new_filename = f"cache_L1associativity_benchmark_data_{timestamp}.csv"
    os.rename(filename, new_filename)
    print("L1 data saved.")

    return median_access_times

#runs the C program, collects the data, creates a graph, prints a prediction of the L1 associativity
def cache_l1associativity_detection():
    #Obtaining output
    print("L1 associativity benchmark:")
    data = cache_L1associativity_output_obtain()

    #Visualizing the output
    cache_associativity_output_visualization(data, "L1")
    print("L1 graph saved.")

    #Print out prediction
    print("L1 associativity prediction:", cache_associativity_output_calculation(data))


#Runs the compiled C program for the L2 test using subprocess, asking the user for necessary informtaion to set the flags, and reads the created csv file and renames the file with a timestamp
#Returns a pandas object that contains the median access times for each associativity tested in the L2 test
def cache_L2associativity_output_obtain():

    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = "cache_L2associativity_benchmark_data.csv"

    # Prompt the user to optionally specify the --cache_linesize flag
    l1_size = input("Enter L1 size (in Bytes)(or press Enter to skip): ").strip()
    l2_size = input("Enter L2 size (in Bytes)(or press Enter to skip): ").strip()
    l1_associativity = input("Enter the associativity of L1 (or press Enter to skip): ").strip()

    # Build the command based on user's input
    cmd = ["./cache_L2associativity_benchmark"]
    if l1_size:
        cmd.append(f"--l1_size={l1_size}")
    elif l2_size:
        cmd.append(f"--l2_size={l2_size}")
    elif l1_associativity:
        cmd.append(f"--l1_associativity={l1_associativity}")
    
    # Open a subprocess to run the C program 
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running benchmark: ")
    process.wait()

    # Read the CSV file in chunks
    chunk_list = []
    for chunk in pd.read_csv(filename, chunksize=10000):
        # Calculate the median access time for each associativity in the chunk
        chunk_median = chunk.groupby('associativity')['access_time'].median().reset_index()
        chunk_list.append(chunk_median)
   
    # Concatenate all chunk results
    median_access_times = pd.concat(chunk_list).groupby('associativity')['access_time'].median().reset_index()
    
    #saves data as a timestamped csv file
    new_filename = f"cache_L2associativity_benchmark_data_{timestamp}.csv"
    os.rename(filename, new_filename)
    print("L2 data saved.")
    

    return median_access_times

#runs the C program, collects the data, creates a graph, prints a prediction of the L2 associativity
def cache_l2associativity_detection():
    #Obtaining output 
    print("L2 associativity benchmark:")
    data = cache_L2associativity_output_obtain()

    #Visualizing the output
    cache_associativity_output_visualization(data, "L2")
    print("L2 graph saved.")

    #Print out prediction
    print("L2 associativity prediction:", cache_associativity_output_calculation(data))


#Runs the compiled C program for the L3 test using subprocess, asking the user for necessary informtaion to set the flags, and reads the created csv file and renames the file with a timestamp
#Returns a pandas object that contains the median access times for each associativity tested in the L3 test
def cache_L3associativity_output_obtain():

    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = "cache_L3associativity_benchmark_data.csv"

    # Prompt the user to optionally specify the --cache_linesize flag
    l1_size = input("Enter L1 size (in Bytes)(or press Enter to skip): ").strip()
    l2_size = input("Enter L2 size (in Bytes)(or press Enter to skip): ").strip()
    l3_size = input("Enter L3 size (in Bytes)(or press Enter to skip): ").strip()

    l1_associativity = input("Enter the associativity of L1 (or press Enter to skip): ").strip()
    l2_associativity = input("Enter the associativity of L2 (or press Enter to skip): ").strip()

    # Build the command based on user's input
    cmd = ["./cache_L3associativity_benchmark"]
    if l1_size:
        cmd.append(f"--l1_size={l1_size}")
    elif l2_size:
        cmd.append(f"--l2_size={l2_size}")
    elif l1_associativity:
        cmd.append(f"--l1_associativity={l1_associativity}")
    elif l2_associativity:
        cmd.append(f"--l2_associativity={l2_associativity}")
    elif l3_size:
        cmd.append(f"--l3_size={l3_size}")
    
    # Open a subprocess to run the C program
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running benchmark: ")
    process.wait()
    
    #Read the CSV file in chunks
    chunk_list = []
    for chunk in pd.read_csv(filename, chunksize=10000):
        # Calculate the median access time for each associativity in the chunk
        chunk_median = chunk.groupby('associativity')['access_time'].median().reset_index()
        chunk_list.append(chunk_median)
   
    # Concatenate all chunk results
    median_access_times = pd.concat(chunk_list).groupby('associativity')['access_time'].median().reset_index()
   
    #saves data as a timestamped csv file
    new_filename = f"cache_L3associativity_benchmark_data_{timestamp}.csv"
    os.rename(filename, new_filename)
    print("L3 data saved.")

    return median_access_times


#runs the C program, collects the data, creates a graph, prints a prediction of the L3 associativity
def cache_l3associativity_detection():
    #Obtaining output
    print("L3 associativity benchmark:") 
    data = cache_L3associativity_output_obtain()

    #Visualizing the output
    cache_associativity_output_visualization(data, "L3")
    print("L3 graph saved.")

    #Print out prediction
    print("L3 associativity prediction:", cache_associativity_output_calculation(data))


#returns the predicted cache associativity based on the filtered data from the csv file (data)
def cache_associativity_output_calculation(data):
    #calculates the difference between the data points and finds the first greatest change
    data['change'] = data['access_time'].diff()
    data['change'] = data['change'].apply(lambda x: x if x > 0 else 0)
    greatest_change_step = data['change'].idxmax()

    #returns the prediction
    if greatest_change_step > 0:
        return data['associativity'][greatest_change_step-1]
    else:
        return "No significant change detected."

#Creates a png file for the graph of the filtered csv file (data). 
def cache_associativity_output_visualization(data, cache_lvl):

    #create the plot
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
    print("---------------------")
    cache_l2associativity_detection()
    print("---------------------")
    cache_l3associativity_detection()
