#!/bin/bash
# array_job_script_vcr.sh

cd $PBS_O_WORKDIR

# modules to load:
module load Boost/1.83.0-GCC-13.2.0
module load Gurobi/12.0.0-GCCcore-13.2.0

walltime="02:05:00" 
TL=7230
MAB=0

for instance in 3; do
    for b in 3 100; do 
        for s in 0 1; do
            sbatch --time=$walltime job_script_miao_FO.sh $instance $s $b $TL $MAB
        done
    done
done