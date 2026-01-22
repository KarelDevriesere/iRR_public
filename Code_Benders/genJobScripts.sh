# Requirements of job
WALLTIME=259200	# 72h
NODES=1
PPN=5
# Note PMEM is memory per node. Max possible is 4 GB per node
# However, we probably want more than 4, so request instead MEM if PPN=1
MEM=64

# Submit the jobscripts on kirlia
module swap cluster/doduo

#for method in 1 2 3 4 
for inst in $(ls -v '../Instances/TTP/CON'*)
do
	BASENAME=$(basename "$inst" | sed 's/\.[^.]*$//')
	echo $BASENAME
	FILE=${BASENAME}"_Benders.sh"
	echo "# Jobscript for instance ${BASENAME}" > $FILE
	echo "" >> $FILE
	echo "# Request wall time and nodes" >> $FILE
	echo "#PBS -l walltime=$WALLTIME" >> $FILE
	echo "#PBS -l nodes=$NODES:ppn=$PPN" >> $FILE
	echo "# Note PMEM is memory per node. Max possible is 4 GB per node" >> $FILE
	echo "#PBS -l mem="$MEM"gb" >> $FILE
	echo "" >> $FILE
	echo "cd $VSC_DATA_VO_USER/iRR_new/Code_Benders" >> $FILE
	echo "# Load modules" >> $FILE
	echo "module purge" >> $FILE
	echo "module load GCC/12.3.0" >> $FILE
	echo "module load Boost/1.74.0-GCC-12.3.0" >> $FILE
	echo "# Disable core dumps" >> $FILE
	echo "ulimit -S -c 0" >> $FILE
	echo "# Solve the instance" >> $FILE
	# 70 hour time limit, at most 3 consec home/awa
	echo "./decompose " ${inst} " 252000 3" >> $FILE
	## Submit the script
	qsub $FILE	
done
