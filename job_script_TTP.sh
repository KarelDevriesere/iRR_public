#!/bin/bash
# job_script_CM.sh

#PBS -N iRR_CM # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l mem=64GB # request memory

# parameters:
seed=$1
instance=$2
NrRounds=$3
heuristic=$4
MinCostNB=$5
HL=$6
TL=$7

cd $PBS_O_WORKDIR

echo "Seed = $seed"
echo "Heuristic = $heuristic"
echo "MinCostNB = $MinCostNB"
echo "HistoryLength = $HL"
echo "instance = $instance"
echo "NrRounds = $NrRounds"
echo "TimeLimit = $TL"

./irr --Seed $seed --Heuristic $heuristic --MinCostNB $MinCostNB --HistoryLength $HL --InstanceTTP $instance --NrRounds $NrRounds --TimeLimit $TL