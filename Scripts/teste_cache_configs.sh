#!/bin/bash

# Script to test how cache size alters performance
# Usage: ./teste_cache_config.sh 



CACHE_SIZES=(1 3 5 7 10 20 50 100 200 500)
ACESSOS=(
    4 12 18 23 37 42 56 61 78 85 91 102 113 127 138
    4 12 18 42 23 91 102 113 127 138 56 78 85 91 102
    4 12 18 23 42 56 61 78 85 91 102 113 127 138 150
    160 170 180 190 200 4 12 18 23 37 42 56 61 78 85
    91 102 113 127 138 4 12 61 18 42 150 160 170 180 190
    4 12 18 42 23 91 102 113 127 138 150 160 170 180
    190 4 18 12 4 12 42 23 37 18 4 61 56 85 78 102
    113 4 12 42 18 23 61 56 91 102 4 12 18 42 23 61
    4 12 18 23 42 150 160 170 180 190 4 18 12 4 12 42
    4 12 18 42 23 61 127 138 91 102 113 4 12 18 4 12
    42 56 61 78 85 91 102 113 150 160 170 180 190 200
    4 12 18 23 37 42 61 56 78 85 91 102 113 127 138
    4 12 18 42 23 61 127 138 91 102 113 4 12 18 23 42
    61 150 160 170 180 190 4 12 42 18 23 4 12 18 42 61
    56 91 102 113 4 12 42 18 23 37 56 85 91 102 113 4
)


REPETICOES=5

for size in "${CACHE_SIZES[@]}"; do

SOMA=0
for ((i=1; i<=REPETICOES; i++)); do

    ./bin/dserver docs "$size" > /dev/null &
    SERVER_PID=$! 
    
    
    sleep 2  

    START_TIME=$(date +%s%3N)
        
    for id in "${ACESSOS[@]}"; do
        ./bin/dclient -c "$id" > /dev/null
    done
    
    END_TIME=$(date +%s%3N)
    TEMPO=$((END_TIME - START_TIME))
    SOMA=$((SOMA + TEMPO))

    ./bin/dclient -f > /dev/null # Stop the server

    sleep 1  

   done
    MEDIA=$((SOMA / REPETICOES))
    echo -e "Cache size $size → Média: ${MEDIA} ms"
done