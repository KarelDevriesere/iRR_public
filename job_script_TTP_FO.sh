#!/bin/bash
# job_script_TTP_FO.sh

#PBS -N iRR_TTP_FO # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l mem=64GB # request memory

# parameters:

instance=$1
TL=$2
M=$3

cd $PBS_O_WORKDIR

echo "Instance = $instance"
echo "TimeLimit = $TL"


./irr --InstanceTTP $instance --FO 1 --TimeLimit $TL --MAB $M