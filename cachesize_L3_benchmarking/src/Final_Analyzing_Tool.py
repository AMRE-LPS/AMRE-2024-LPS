#!/usr/bin/env python3
import csv
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import subprocess
from scipy.signal import savgol_filter
from kneed import KneeLocator
from sklearn.neighbors import KNeighborsRegressor
import pwlf
from datetime import datetime

# Get the current timestamp for the file name
timestamp = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')

def cache_L3size_output_obtain_maximum(cache_line_size=64):
    cmd = ["./cachesize_maximum", f"--cache_line_size={cache_line_size}"]
    filename = f'cache_L3size_benchmark_data_maximum_{timestamp}.csv'
    
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running Maximum Cache Size Detection Benchmark. Should take 10-20 minutes: ")
 
    with open(filename, 'w', newline='') as csvfile:
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            csvfile.write(output)
            csvfile.flush()

    print()
    process.stdout.close()
    process.wait()
    return pd.read_csv(filename)

def cache_L3size_output_obtain_estimated(cache_line_size, max_cache_size):
    cmd = ["./cachesize_estimated", f"--cache_line_size={cache_line_size}", f"--max_cache_size={max_cache_size}"]
    filename = f'cache_L3size_benchmark_data_estimated_{timestamp}.csv'
    
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Running L3 Cache Size Detection Benchmark. Should take 30 mins - 2 hours: ")
 
    with open(filename, 'w', newline='') as csvfile:
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            csvfile.write(output)
            csvfile.flush()

    print()
    process.stdout.close()
    process.wait()
    return pd.read_csv(filename)

def get_data(file_name):
    with open(file_name, 'r') as f:
        reader = csv.reader(f)
        data_dict = {}
        for row in reader:
            size = float(row[0].split(' ')[0])
            number_interested = [int(val) for val in row[1:] if val != ''] 
            if size in data_dict:
                data_dict[size].extend(number_interested)
            else:
                data_dict[size] = number_interested
    x = []
    y_mean = []
    y_median = []
    for size, values in data_dict.items():
        x.append(size)
        values_array = np.array(values)
        y_mean.append(np.mean(values_array))
        y_median.append(np.median(values_array))
    return x, y_mean, y_median

def cutting_interval(x, y, ax):
    x = np.array(x)
    y = np.array(y)
    
    kneedle = KneeLocator(x, y, S=1.0, curve='concave', direction='increasing', interp_method="interp1d")
    knee_point = kneedle.knee
    ax.axvline(x=knee_point, color='y', linestyle='--')
    print('Your total cache size based on the knee point detection would not exceed:', knee_point, 'MB')
    return knee_point

def piecewise_regression(x, y, ax, piecewise_segments_number):
    x = np.array(x)
    y = np.array(y)
    my_pwlf = pwlf.PiecewiseLinFit(x, y)
    res = my_pwlf.fit(piecewise_segments_number)
    print(f'Your total cache size based on the piecewise regression is within the interval in MB: [{res[piecewise_segments_number-1]/1024-1:.2f}, {res[piecewise_segments_number-1]/1024+1:.2f}]')
    ax.plot(x, my_pwlf.predict(x), linewidth=3.0)
    ax.axvline(x=res[piecewise_segments_number-1]-1024, color='g', linestyle='--')
    ax.axvline(x=res[piecewise_segments_number-1]+1024, color='g', linestyle='--')

def plot_data_simple(title, x, y, method, window_size, piecewise_segments_number):
    fig, ax = plt.subplots(figsize=(16, 8))
    ax.plot(x, y, color='lightgray', marker='o', label='Original')

    if method is not None:
        if method == 'SG':
            y_smooth = savgol_filter(y, window_size, 3)
        elif method == 'KNN':
            X = np.array(x).reshape(-1, 1)
            clf = KNeighborsRegressor(window_size, weights='uniform')
            clf.fit(X, y)
            y_smooth = clf.predict(X)
        ax.plot(x, y_smooth, marker='o', label='Denoised')
        print('Denoising Method', method, ':')
        piecewise_regression(x, y_smooth, ax, piecewise_segments_number)
    else:
        ax.plot(x, y, marker='o', label='Raw')
        print('Raw Data:')
        piecewise_regression(x, y, ax, piecewise_segments_number)
    
    ax.set_title(title)
    ax.set_xlabel('Data Size in KB')
    ax.set_ylabel('Cycles per load')
    ax.grid(True)
    ax.legend()
    plt.tight_layout()

def plot_denoising_data(x, y, suffix, window_size, piecewise_segments_number):
    plot_data_simple('Linked Graph Raw', x, y, None, window_size, piecewise_segments_number)
    plt.savefig('Linked Graph Raw' + suffix + '.png')
    
    #plot_data_simple('Linked Graph Denoised with Savitzky-Golay', x, y, 'SG', window_size, piecewise_segments_number)
    #plt.savefig('Linked Graph Savitzky-Golay' + suffix + '.png')
    
    plot_data_simple('Linked Graph Denoised with K-Nearest Neighbors', x, y, 'KNN', window_size, piecewise_segments_number)
    plt.savefig('Linked Graph KNN' + suffix + '.png')

if __name__ == '__main__':
    cache_line_size = input("Please enter the cache line size (default is 64): ")
    if not cache_line_size:
        cache_line_size = 64

    knows_max_cache_size = input("Do you know how big your biggest cache size is? (Y/N): ")

    if knows_max_cache_size.lower() == 'n':
        output = cache_L3size_output_obtain_maximum(int(cache_line_size))
        file_name = f'cache_L3size_benchmark_data_maximum_{timestamp}.csv'

        window_size = 5

        x, y_mean, y_median = get_data(file_name)
        y = y_mean

        y_smooth = savgol_filter(y, window_size, 3)
        x = np.array(x)
        y = np.array(y_smooth)
    
        kneedle = KneeLocator(x, y, S=1.0, curve='concave', direction='increasing', interp_method="interp1d")
        max_cache_size = kneedle.knee
        # Plotting the diagram
        plt.figure(figsize=(16, 8))
        plt.plot(x, y, label='Smoothed Data',marker='o')
        plt.axvline(x=max_cache_size, color='red', linestyle='--', label=f'Max Cache Size: {max_cache_size}')
        plt.xlabel('Data Size in MB')
        plt.ylabel('Cycles per load')
        plt.title('Linked Graph Savitzky-Golay Maximum Cache Size')
        plt.legend()
        plt.grid(True)

        # Save the plot
        plt.savefig('Linked Graph Savitzky-Golay Maximum Cache Size.png')
        print("Based on the SG smoothing with Kneedle Algorithm, your biggest Cache Size should not exceed: ", max_cache_size)

    else:
        max_cache_size = input("Please enter the max cache size in MB, please add 4MB to your assumption: ")
        if not max_cache_size:
            max_cache_size = 32

    output = cache_L3size_output_obtain_estimated(int(cache_line_size), float(max_cache_size))
    file_name = f'cache_L3size_benchmark_data_estimated_{timestamp}.csv'
    piecewise_segments_number = 6
    window_size = 5

    x, y_mean, y_median = get_data(file_name)
    y = y_median

    plot_denoising_data(x, y, ' estimated_L3_size', window_size, piecewise_segments_number)
