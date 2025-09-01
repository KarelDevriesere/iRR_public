# login node:
module load Boost/1.83.0-GCC-13.2.0
module load Gurobi
make # ..
# cluster:
module swap cluster/doduo
qsub array_job_script.sh
# Indien .h file aangepast->make clean
# Indien rare errors: ook make clean