# Compute the median and minimum costs for the base configuration

LOGFILEMEDIANBASE="MedianBase.csv"

echo "Instance,MedianBase" > $LOGFILEMEDIANBASE

for f in $(seq -w 1 30);
do

	# Min cost and array of all costs
	min_cost=9999999
	cost_array=()

	echo "i$f"
	for r in {1..10}
	do
		DIR="$VSC_DATA_VO_USER/IHTC"
		STDOUT="${DIR}/Ablation/Source-i${f}-${r}.stdout"

		if [ ! -f "$STDOUT" ]; then
			echo "ERROR. File ${STDOUT} does not exist!"
			exit 1
		fi


		if grep -q "Total violations" "$STDOUT"; then
			VIOL=$(grep "Total violations" "$STDOUT" | tail -n 1 | awk '{print $4}')
		else
			echo "WARNING: 'Total violations' not found in $STDOUT"
			exit 1
		fi

		if grep -q "Total cost" "$STDOUT"; then
			OBJ=$(grep "Total cost" "$STDOUT" | tail -n 1 | awk '{print $4}')
		else
			echo "WARNING: 'Total cost' not found in $STDOUT"
			exit 1
		fi


		
		echo "${VIOL} - ${OBJ}"
		if [ "$VIOL" == "0" ]; then

			# Add cost to the array
			cost_array+=("$OBJ")
		else
			echo "ERROR: Expected to always find a feasible solution"
			exit 1
		fi

	done

	# Count the overall number of feasible solutions found
	total_count=${#cost_array[@]}

	# We assume that a feasible solution is always found!
	if (( total_count < 1 )); then
		echo "ERROR: No feasible solution found for i$f"
		exit 1
	fi

	# Sort the cost array
	mapfile -t sorted < <(printf '%s\n' "${cost_array[@]}" | sort -n)

	# Compute the median
	if (( total_count % 2 == 1 )); then
		# odd: middle element (0-based index)
		idx=$(( total_count / 2 ))
		median_cost="${sorted[$idx]}"
	else
		# even: average of two middle elements
		idx1=$(( total_count / 2 - 1 ))
		idx2=$(( total_count / 2 ))
		a="${sorted[$idx1]}"
		b="${sorted[$idx2]}"
		median_cost=$(awk "BEGIN {print ($a + $b) / 2}")
	fi

	# Min cost is the first element in the array
	min_cost=${sorted[0]}

	echo "Num feas: $total_count"
	echo "Median Cost: $median_cost"
	echo "Minimum Cost: $min_cost"
	echo "i$f,$min_cost" >> $LOGFILEMEDIANBASE
done

