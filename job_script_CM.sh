#!/bin/bash
# job_script_CM.sh

#PBS -N iRR_CM # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l mem=64GB # request memory

# parameters:
seed=$1
NrTeams=$2
k=$3
i=$4
heuristic=$5
MinCostNB=$6
HL=$7
TL=$8

cd $PBS_O_WORKDIR

echo "run" 0 $seed $heuristic $MiNCostNB $HL $NrTeams $k $i $TL
./irr --seed $seed --Heuristic $heuristic --MinCostNB $MiNCostNB --HistoryLength $HL --NrTeams $NrTeams --k $k --i $i --TL $TL