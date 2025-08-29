#!/bin/bash
# array_job_script.sh

notification_flag=1 # =0 if we don't want to receive messages on teams when all jobs are done

if [ $notification_flag -eq 1 ]; then
    NOTIFY_DIR="/data/gent/470/vsc47002" 
    g++ -o "$NOTIFY_DIR/notify" "$NOTIFY_DIR/notify.cpp"
    echo "compiled notify"
fi

cd $PBS_O_WORKDIR

for NrBreaksPerTeam in 0,1,2,3; do
    for Capacity in 0,1; do
        if [ $Capacity -eq 1 ]; then
            for Setting in 1,2; do
                for algo in 0,1,2,3,4,5; do
                    for inst in 1,2,3,4,5,7; do
                        sbatch job_script.sh $algo 0 $inst $NrBreaksPerTeam $Capacity $Setting $notification_flag 
                    done
                done
            done
        else
            for algo in 0,1,2,3,4,5; do
                for inst in 1,2,3,4,5,7; do
                    sbatch job_script.sh $algo 0 $inst $NrBreaksPerTeam $Capacity 0 $notification_flag
                done
            done
        fi
    done
done