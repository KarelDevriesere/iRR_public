#!/bin/bash
# job_script.sh

#PBS -N run_simulation # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l walltime=02:00:00 # max. 2h of wall time

#PBS -l mem=64GB # request memory

# modules to load:

# parameters:
Algo=$1
InstanceSet=$2
Instance=$3
NrBreaksPerTeam=$4
Capacity=$5
Setting=$6

cd $PBS_O_WORKDIR

if [$Setting -eq 0]; then
    .. $Algo $InstanceSet $Instance $NrBreaksPerTeam $Capacity
else
    .. $Algo $InstanceSet $Instance $NrBreaksPerTeam $Capacity $Setting
fi
