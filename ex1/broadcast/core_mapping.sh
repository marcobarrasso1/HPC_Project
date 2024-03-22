#!/bin/bash
#SBATCH --job-name=ex1
#SBATCH --nodes=2 
#SBATCH --ntasks-per-node=24
#SBATCH --cpus-per-task=1
#SBATCH --partition=THIN
#SBATCH --time=02:00:00
#SBATCH --output=osu_bcast_results%j.out
#SBATCH --error=osu_bcast_%j.err
#SBATCH --exclusive

module load openMPI/4.1.5/gnu/12.2.1

CSV_FILE="bcast_core_thin_results.csv"
OSU_BCAST_PATH="../osu-micro-benchmarks-7.3/c/mpi/collective/blocking/osu_bcast"
MESSAGE_SIZE="1"
WARMUP_ITER="5000"
TOTAL_ITER="15000"
ALGORITHMS=(1 2 5) # Array of algorithms to test

# Header for CSV file
echo "cores,basic_linear,chain,binary_tree" > $CSV_FILE

for CORES in $(seq 2 48); do
    echo -n "$CORES" >> $CSV_FILE # Write the number of cores
    
    # Loop over algorithms
    for ALG in "${ALGORITHMS[@]}"; do
        # Run the benchmark with the current number of cores and algorithm
        OUTPUT=$(mpirun --mca coll_tuned_use_dynamic_rules true --mca coll_tuned_bcast_algorithm $ALG -np $CORES --map-by core $OSU_BCAST_PATH -x $WARMUP_ITER -i $TOTAL_ITER -m $MESSAGE_SIZE:$MESSAGE_SIZE)
        
        # Extract the average latency for the fixed message size
        AVG_LAT=$(echo "$OUTPUT" | awk -v size=$MESSAGE_SIZE '($1==size){print $2}')
        
        echo -n ",$AVG_LAT" >> $CSV_FILE
    done
    
    echo "" >> $CSV_FILE 
done
