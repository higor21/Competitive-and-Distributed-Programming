#!/bin/bash
#SBATCH --job-name=OMP_HelloOfHigor
#SBATCH --time=0-0:5
#SBATCH --cpus-per-task=1
#SBATCH --hint=compute_bound
export OMP_NUM_THREADS=32

./omp_odd_even1 32 10 g
