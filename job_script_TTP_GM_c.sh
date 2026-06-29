#!/bin/bash
# job_script_TTP_GM_c.sh

#PBS -N iRR_TTP_GM_c # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l mem=64GB # request memory

# parameters:

instance=$1
seed=$2

cd $PBS_O_WORKDIR

echo "Instance = $instance"
echo "TimeLimit = $TL"


./irr --InstanceTTP $instance --TimeLimit 43200 --GM 1 --Seed $seed --Constructive 1