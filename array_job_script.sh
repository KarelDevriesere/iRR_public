#!/bin/bash
# array_job_script.sh

notification_flag=1 # =0 if we don't want to receive messages on teams when all jobs are done
walltime=0

if [ $notification_flag -eq 1 ]; then
    NOTIFY_DIR="/data/gent/470/vsc47002" 
    g++ -o "$NOTIFY_DIR/notify" "$NOTIFY_DIR/notify.cpp"
    echo "compiled notify"
fi

cd $PBS_O_WORKDIR

# modules to load:
module load Boost/1.83.0-GCC-13.2.0
module load Gurobi

# compile project only once here
make release PRINT=1 

for algo in 0 5; do # 0 1 2 3 4 5
    if [ $algo -le 2 ]; then
        walltime="02:10:00" # 2 hours for IP or RF (+10 minutes)
    else
        walltime="00:10:00" # 5 minutes (5x60s for heuristics, say 10 minutes to be sure)
    fi
    for inst in 1; do # 1 2 3 4 5 7
        for NrBreaksPerTeam in 0; do # 0 1 2 3
            for Capacity in 0; do # 0 1
                if [ $Capacity -eq 1 ]; then
                    for Setting in 1 2; do
                        sbatch --time=$walltime job_script.sh $algo 0 $inst $NrBreaksPerTeam $Capacity $Setting $notification_flag
                    done
                else
                    sbatch --time=$walltime job_script.sh $algo 0 $inst $NrBreaksPerTeam $Capacity 0 $notification_flag
                fi
            done
        done
    done
done