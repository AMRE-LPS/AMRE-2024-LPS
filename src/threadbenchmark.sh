#!/bin/bash

# Check if Python3 is installed
if ! command -v python3 &> /dev/null; then
    echo "Python3 could not be found, please install it."
    exit 1
fi

# Create a virtual environment
python3 -m venv myenv

# Determine the OS and source the virtual environment accordingly
if [[ "$OSTYPE" == "linux-gnu"* || "$OSTYPE" == "darwin"* ]]; then
    source myenv/bin/activate
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    source myenv/Scripts/activate
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

# Install dependencies from requirements.txt
if [ -f requirements.txt ]; then
    pip3 install -r requirements.txt
else
    echo "requirements.txt not found, please ensure it exists in the current directory."
    deactivate
    exit 1
fi

# Run the Python file thread.py
if [ -f threadcount.py ]; then
    python threadcount.py
else
    echo "thread.py not found, please ensure it exists in the current directory."
    deactivate
    exit 1
fi

# Deactivate the virtual environment
deactivate
