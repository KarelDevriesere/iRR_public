#!/bin/bash
# array_job_script_initial_solutions_hockey.sh

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

seed=42 # random seed
for algo in 0; do # 0 1 2 3 4 5
    walltime="48:00:00" # ask for 2 days of computation time
    for inst in 3 4 5 6; do 
        sbatch --time=$walltime job_script.sh $notification_flag $seed $algo 1 $inst
    done
done