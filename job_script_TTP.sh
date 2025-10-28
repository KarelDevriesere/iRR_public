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
HL=$5
TL=$6
Base=$7

cd $PBS_O_WORKDIR

echo "Seed = $seed"
echo "Heuristic = $heuristic"
echo "HistoryLength = $HL"
echo "instance = $instance"
echo "NrRounds = $NrRounds"
echo "TimeLimit = $TL"
echo "Base = $Base"

./irr --Seed $seed --Heuristic $heuristic --HistoryLength $HL --InstanceTTP $instance --NrRounds $NrRounds --TimeLimit $TL --Base $Base