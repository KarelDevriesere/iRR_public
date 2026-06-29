#!/bin/bash
# array_job_script_vcr.sh

cd $PBS_O_WORKDIR

# modules to load:
module load Boost/1.83.0-GCC-13.2.0
module load Gurobi/12.0.0-GCCcore-13.2.0

walltime="02:05:00" 
TL=7230
MAB=0

for instance in "CIRC40" "LINE40" "CON40" "GAL40" "INCR40" "NL16" "NFL32" "BRA24"; do
    if [ "$instance" = "NL16" ]; then
        rounds="4 8 12"
    elif [ "$instance" = "NFL32" ]; then
        rounds="8 16 24"
    elif [ "$instance" = "BRA24" ]; then
        rounds="6 12 18"
    else
        rounds="10 20 30"
    fi
    for round in $rounds; do
        merged_instance="Instances/TTP/${instance}_${round}.xml"
        echo "submit ${merged_instance}"
        sbatch --time=$walltime job_script_TTP_FO.sh $merged_instance $TL $MAB
    done
done