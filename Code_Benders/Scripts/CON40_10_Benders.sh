# Jobscript for instance CON40_10

# Request wall time and nodes
#PBS -l walltime=259200
#PBS -l nodes=1:ppn=5
# Note PMEM is memory per node. Max possible is 4 GB per node
#PBS -l mem=64gb

cd /data/gent/vo/001/gvo00147/vsc41980/iRR_new/Code_Benders
# Load modules
module purge
module load GCC/12.3.0
module load Boost/1.74.0-GCC-12.3.0
# Disable core dumps
ulimit -S -c 0
# Solve the instance
./decompose  ../Instances/TTP/CON40_10.xml  252000 3
