#!/bin/bash

# Script to test parallelism in the dclient program
# Usage: ./teste_paralelismo.sh "keyword" <number_of_processes> 

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <keyword>"
    exit 1
fi

# Get the keyword from command line arguments
KEYWORD="$1"

# Check if the keyword is not empty
if [ -z "$KEYWORD" ]; then
    echo "Error: Keyword cannot be empty."
    exit 1
fi

# Function to run the dclient program with the given keyword and process count
run_dclient() {
    local num_processes="$1"
    ../bin/dclient -s "$KEYWORD" "$num_processes"
}

PROCESS_COUNTS=(1 2 5 10 30 50 100 200 500 1000 1500 2000)
for NUM_PROCESSES in "${PROCESS_COUNTS[@]}"; do
    echo "Running with $NUM_PROCESSES processes..."
    # Start timer in milliseconds
    START_TIME=$(date +%s%3N)
    run_dclient "$NUM_PROCESSES" > /dev/null 
    # End timer in milliseconds
    END_TIME=$(date +%s%3N)
    # Calculate the time taken in milliseconds
    TIME_TAKEN=$((END_TIME - START_TIME))
    echo -e "--------------------------\nFinished with $NUM_PROCESSES processes.\nFinal time: ${TIME_TAKEN} milliseconds\n----------------------------"
done
