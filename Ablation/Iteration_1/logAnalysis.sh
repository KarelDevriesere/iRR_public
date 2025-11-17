LOGFILE="results.csv"
LOGFILEMIN="min.csv"

# Run logBase to determine these numbers!
# Median performance of the base configuration for each of the instances
declare -A bound=(
	["BRA24_6.xml"]=38745
	["BRA24_12.xml"]=98815
	["BRA24_18.xml"]=167657
	["CIRC40_10.xml"]=560
	["CIRC40_20.xml"]=1760
	["CIRC40_30.xml"]=3600
	["CON40_10.xml"]=280
	["CON40_20.xml"]=555
	["CON40_30.xml"]=803
	["GAL40_10.xml"]=23617
	["GAL40_20.xml"]=50962
	["GAL40_30.xml"]=81661
	["INCR40_10.xml"]=13284
	["INCR40_20.xml"]=46402
	["INCR40_30.xml"]=102306
	["LINE40_10.xml"]=656
	["LINE40_20.xml"]=2322
	["LINE40_30.xml"]=5118
	["NFL32_8.xml"]=70127
	["NFL32_16.xml"]=173493
	["NFL32_24.xml"]=297068
	["NL16_4.xml"]=23625
	["NL16_8.xml"]=57263
	["NL16_12.xml"]=92580
)

# You can get these from genJobScriptsAblation.sh
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

echo "Algorithm,LB,Obj" > $LOGFILE
echo "Algorithm,LB,Obj" > $LOGFILEMIN

# Swap to right cluster
module swap cluster/doduo

# Run all configurations
for c in "${!configs[@]}"
do
	echo $c
        for f in BRA24_12.xml BRA24_18.xml BRA24_6.xml CIRC40_10.xml CIRC40_20.xml CIRC40_30.xml CON40_10.xml CON40_20.xml CON40_30.xml GAL40_10.xml GAL40_20.xml GAL40_30.xml INCR40_10.xml INCR40_20.xml INCR40_30.xml LINE40_10.xml LINE40_20.xml LINE40_30.xml NFL32_16.xml NFL32_24.xml NFL32_8.xml NL16_12.xml NL16_4.xml NL16_8.xml
        do
		min=999999999
                for r in {1..5}
                do

			DIR="$VSC_DATA_VO_USER/iRR_new"
			STDOUT="${DIR}/Ablation/${c}-${f}-${r}.stdout"

			if [ ! -f "${STDOUT}" ]; then
				echo "ERROR. File ${STDOUT} does not exist!"
				#exit 1
			fi


			OBJ=-1
			if grep -q "Final cost" "$STDOUT"; then
    				OBJ=$(cat ${STDOUT} | grep -o -E 'Final cost = [0-9]+' | cut -d ' ' -f4 | tail -n 1)
			else
				echo "WARNING: 'Final cost' not found in $STDOUT"
				continue
			fi

			# Only append feasible sols!
			echo "$c,${bound[$f]},${OBJ}" >> $LOGFILE
			if [[ "$OBJ" -lt "$min" ]]; then
				min="$OBJ"
			fi

			# Compute gap and print if larger than 30%
			LB=${bound[$f]}
			if [ "$LB" != "0" ]; then
				GAP=$(( (OBJ - LB) * 100 / LB ))  # integer division for percentage
			 	if [ "$GAP" -gt 30 ]; then
					echo "LARGE GAP: Config $c, Instance $f, Bound ${bound[$f]}, OBJ=$OBJ, LB=$LB, Gap=${GAP}%"
			 	fi
			fi

		done
		echo "$c,${bound[$f]},${min}" >> $LOGFILEMIN
	done
done

