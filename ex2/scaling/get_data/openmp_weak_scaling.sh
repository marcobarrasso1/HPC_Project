#!/bin/bash
#SBATCH --job-name=ex2
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --partition=THIN
#SBATCH --time=02:00:00
#SBATCH --error=osu_bcast_%j.err
#SBATCH --exclusive
#SBATCH --exclude=thin006

module load openMPI/4.1.5/gnu/12.2

np_values=$(seq 1 1 24)
repetitions=30
warmup_iterations=5
out_csv="./mp_weak.csv"
echo "Threads,Size,Time" > $out_csv

for np in $np_values; do
        times1=()
	size=$(($np * 1000))
        for rep in $(seq 1 $repetitions); do
            # Warm-up phase: Execute the program without storing the results
            if [ "$rep" -le "$warmup_iterations" ]; then
                mpirun -np 1 --map-by node --bind-to none ./mandel2 $size 1000 -2 -2 1.5 1.5 255 $np > /dev/null
            else
                time1=$(mpirun -np 1 --map-by node --bind-to none ./mandel2 $size 1000 -2 -2 1.5 1.5 255 $np | tail -n 1)
                times1+=($time1)
            fi
        done
        # Calculate mean for times1 using awk
        mean1=$(printf "%f\n" "${times1[@]}" | awk '{sum+=$1} END {print sum/NR}')
        echo "$np,$size,$mean1" >> $out_csv
done
