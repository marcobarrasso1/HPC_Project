#!/bin/bash
#SBATCH --job-name=ex1
#SBATCH --nodes=2 # Adjust based on the maximum number of processes you plan to test
#SBATCH --ntasks-per-node=24 # This might need adjustment depending on your cluster's configuration
#SBATCH --cpus-per-task=1
#SBATCH --partition=THIN
#SBATCH --time=02:00:00
#SBATCH --output=osu_bcast_results%j.out
#SBATCH --error=osu_bcast_%j.err
#SBATCH --exclusive


module load openMPI/4.1.5/gnu/12.2.1

CSV_FILE="gather_socket_thin_results.csv"
OSU_GATHER_PATH="../osu-micro-benchmarks-7.3/c/mpi/collective/blocking/osu_gather"
MESSAGE_SIZE="1"
WARMUP_ITER="5000"
TOTAL_ITER="15000"
ALGORITHMS=(1 2)

# Header for CSV file
echo "cores,basic_linear,binomial" > $CSV_FILE

# Loop over cores
for CORES in $(seq 2 48); do
    echo -n "$CORES" >> $CSV_FILE # Write the number of cores
    
    # Loop over algorithms
    for ALG in "${ALGORITHMS[@]}"; do
        # Run the benchmark with the current number of cores and algorithm
        OUTPUT=$(mpirun --mca coll_tuned_use_dynamic_rules true --mca coll_tuned_gather_algorithm $ALG -np $CORES --map-by socket $OSU_GATHER_PATH -x $WARMUP_ITER -i $TOTAL_ITER -m $MESSAGE_SIZE:$MESSAGE_SIZE)
        
        # Extract the average latency for the fixed message size
        AVG_LAT=$(echo "$OUTPUT" | awk -v size=$MESSAGE_SIZE '($1==size){print $2}')
        
        echo -n ",$AVG_LAT" >> $CSV_FILE # Write the latency next to the corresponding cores and algorithm
    done
    
    echo "" >> $CSV_FILE # New line after each row of cores
done

