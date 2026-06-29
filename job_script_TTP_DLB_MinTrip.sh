#!/bin/bash
# job_script_irr.sh

#PBS -N iRR_ttp_dlb_MinTrip # job name

#PBS -l nodes=1:ppn=8 # 1 node, 8 cores

#PBS -l mem=64GB # request memory

cd $PBS_O_WORKDIR

instance=$1

./irr --InstanceTTP $instance --TimeLimit 172800 --Bounds 1 --DLB 1 --addMinTripConstraint 1