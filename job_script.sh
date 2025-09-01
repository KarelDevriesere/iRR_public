#!/bin/bash
# job_script.sh

#PBS -N iRR # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l mem=64GB # request memory

# parameters:
Algo=$1
InstanceSet=$2
Instance=$3
NrBreaksPerTeam=$4
Capacity=$5
Setting=$6
notification_flag=$7

cd $PBS_O_WORKDIR

if [ $Setting -eq 0 ]; then
    echo "run" $Algo $InstanceSet $Instance $NrBreaksPerTeam $Capacity
    ./irr $Algo $InstanceSet $Instance $NrBreaksPerTeam $Capacity
else
    echo "run" $Algo $InstanceSet $Instance $NrBreaksPerTeam $Capacity $Setting
    ./irr $Algo $InstanceSet $Instance $NrBreaksPerTeam $Capacity $Setting
fi

# After job finishes, check queue
if [ $notification_flag -eq 1 ]; then
    queued=$(qstat | grep " Q " | wc -l)
    running=$(qstat | grep " R " | wc -l)
    failed==$(qstat | grep " F " | wc -l)
    completed==$(qstat | grep " C " | wc -l)

    if [ "$queued" -eq 0 ] && [ "$running" -le 1 ]; then
        cd "/data/gent/470/vsc47002"
        ./notify "All jobs finished iRR finished! Nr completed = $completed, nr failed = $failed"
    fi
fi
