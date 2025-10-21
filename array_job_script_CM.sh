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

walltime="01:05:00"
seed=0 # random seed
TL=3650 # 4 hours

for NrTeams in 36; do
    for k in 1; do
        for i in 0; do
            for heuristic in 0 1; do
                if [ $heuristic -eq 1 ]; then
                    for MiNCostNB in 0; do
                        for HL in 1; do
                             sbatch --time=$walltime job_script_CM.sh $seed $NrTeams $k $i $heuristic $MinCostNB $HL $TL
                        done
                    done
                else
                     sbatch --time=$walltime job_script_CM.sh $seed $NrTeams $k $i $heuristic 0 0 $TL #last 2 parameters not relevant when doing IP
                fi
            done
        done
    done
done