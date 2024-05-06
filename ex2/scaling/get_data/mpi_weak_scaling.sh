#!/bin/bash
#SBATCH --job-name=ex2
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=24
#SBATCH --partition=THIN
#SBATCH --time=02:00:00
#SBATCH --error=osu_bcast_%j.err
#SBATCH --exclusive

module load openMPI/4.1.5/gnu/12.2

np_values=$(seq 1 1 96)
repetitions=30
warmup_iterations=5
out_csv="./mpi_weak_scaling.csv"

echo "Processes,Size,Time" > $out_csv

for np in $np_values; do
    times1=()
    size=$(($np * 1000))  # Correct arithmetic expansion
    for rep in $(seq 1 $repetitions); do  # Corrected loop syntax
        if [ "$rep" -le "$warmup_iterations" ]; then
            mpirun -np $np ./mandel2 $size 1000 -2 -2 1.5 1.5 255 1 > /dev/null
        else
            time1=$(mpirun -np $np ./mandel2 $size 1000 -2 -2 1.5 1.5 255 1 | tail -n 1)
            times1+=($time1)
        fi
    done
    mean1=$(printf "%s\n" "${times1[@]}" | awk '{sum+=$1} END {print sum/NR}')

    echo "$np,$size,$mean1" >> $out_csv
done

