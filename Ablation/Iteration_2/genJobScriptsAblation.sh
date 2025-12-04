# All default values defined in the main
FIXED_PARAMS="";


declare -A configs=(
###############
# Iteration 1 #
###############
["Source"]="--LowerBoundGapHillClimbing 2.3025 --Weight TS 0.3313 --Weight RS 0.1241 --Weight PRS 0.5984 --Weight PTS 1.4343 --ConstrViolationCost 19649 --TimeLimit 21600"
["Source_C"]="--LowerBoundGapHillClimbing 2.3025 --Weight TS 0.3313 --Weight RS 0.1241 --Weight PRS 0.5984 --Weight PTS 1.4343 --Weight C 0.7227 --ConstrViolationCost 19649 --TimeLimit 21600"
###############
# Iteration 2 #
###############
["iPTS"]="--LowerBoundGapHillClimbing 2.3025 --Weight TS 0.3313 --Weight RS 0.1241 --Weight PRS 0.5984 --Weight iPTS_Random_PR 0.4226 --Weight iPTS_MinCost_PR 0.9923 --Weight C 0.7227 --ConstrViolationCost 19649 --TimeLimit 21600"
["iPRS_M"]="--Weight PTS 1.4343 --LowerBoundGapHillClimbing 2.3025 --Weight TS 0.3313 --Weight RS 0.1241 --Weight PRS 0.5984 --Weight C 0.7227 --Weight Random_M_Random_PR 0.8367 --Weight Random_M_MinCost_PR 0.1264 --Weight MinCost_M_MinCost_PR 0.0106 --ConstrViolationCost 19649 --TimeLimit 21600"
["iPRS_B"]="--Weight PTS 1.4343 --LowerBoundGapHillClimbing 2.3025 --Weight TS 0.3313 --Weight RS 0.1241 --Weight PRS 0.5984 --Weight C 0.7227 --Weight Random_BM 0.7475 --Weight MinCost_BM 0.156 --ConstrViolationCost 19649 --TimeLimit 21600"
##############
# Final Iter #
##############
["Target"]="--LowerBoundGapHillClimbing 2.3025 --Weight TS 0.3313 --Weight RS 0.1241 --Weight PRS 0.5984 --Weight iPTS_Random_PR 0.4226 --Weight iPTS_MinCost_PR 0.9923 --Weight C 0.7227 --Weight Random_BM 0.7475 --Weight MinCost_BM 0.156 --Weight Random_M_Random_PR 0.8367 --Weight Random_M_MinCost_PR 0.1264 --Weight MinCost_M_MinCost_PR 0.0106 --ConstrViolationCost 19649 --TimeLimit 21600"
)



# Swap to right cluster
module swap cluster/doduo

# Run all configurations
for c in "${!configs[@]}"
do
        for f in BRA24_12.xml BRA24_18.xml BRA24_6.xml CIRC40_10.xml CIRC40_20.xml CIRC40_30.xml CON40_10.xml CON40_20.xml CON40_30.xml GAL40_10.xml GAL40_20.xml GAL40_30.xml INCR40_10.xml INCR40_20.xml INCR40_30.xml LINE40_10.xml LINE40_20.xml LINE40_30.xml NFL32_16.xml NFL32_24.xml NFL32_8.xml NL16_12.xml NL16_4.xml NL16_8.xml
	do
		for r in {1..5}
		do
			DIR="$VSC_DATA_VO_USER/iRR_new"
			INSTANCE="${DIR}/Instances/TTP/${f}"
			STDOUT="${DIR}/Ablation/${c}-${f}-${r}.stdout"
			STDERR="${DIR}/Ablation/${c}-${f}-${r}.stderr"
			qsub <<EOF
#!/bin/bash
#PBS -l walltime=06:20:00
#PBS -l nodes=1:ppn=1
#PBS -l mem=4gb
#PBS -N ${c}-${f}-${r} # Jobname (otherwise, the jobname is STDIN since we echo the file to qsub)
#PBS -o ${STDOUT} 	# Output file	
#PBS -e ${STDERR} 	# Error file

# Go to the right directory
cd ${DIR}

# Load Boost
module load Boost/1.83.0-GCC-13.2.0
module load Gurobi

# Disable core dumps
ulimit -S -c 0

# Solve the instance
./irr --InstanceTTP ${INSTANCE} --Seed $r ${configs[$c]} 

exit 0
EOF
		done
	done
done

