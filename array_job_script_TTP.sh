#!/bin/bash
# array_job_script.sh

notification_flag=0 # =0 if we don't want to receive messages on teams when all jobs are done
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
make clean
make release PRINT=0

walltime="02:30:00"
seed=0 # random seed
TL=7260 # 4 hours

for instance in "N16"; do
    for NrRounds in 4 8 12; do
        for heuristic in 0 1; do
            if [ $heuristic -eq 1 ]; then
                for MinCostNB in 0 1; do
                    for HL in 1 10 50 500 5000; do
                            sbatch --time=$walltime job_script_TTP.sh $seed $instance $NrRounds $heuristic $MinCostNB $HL $TL
                    done
                done
            else
                    sbatch --time=$walltime job_script_TTP.sh $seed $instance $NrRounds $heuristic 1 1 $TL #last 2 parameters not relevant when doing IP
            fi
        done
    done
done