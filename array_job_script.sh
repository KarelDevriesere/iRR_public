#!/bin/bash
# array_job_script.sh

cd $PBS_O_WORKDIR

for NrBreaksPerTeam in 0 1 2 3; do
    for Capacity in 0 1; do
        if [ $Capacity -eq 1 ]; then
            for Setting in 1 2:
                for algo in 0 1 2 3 4 5:
                    for inst in 1 2 3 4 5 7:
                        sbatch job_script.sh $algo 0 $inst $NrBreaksPerTeam $Capacity $Setting 
        else
            for algo in 0 1 2 3 4 5:
                for inst in 1 2 3 4 5 7:
                    sbatch job_script.sh $algo 0 $inst $NrBreaksPerTeam $Capacity 0
        fi