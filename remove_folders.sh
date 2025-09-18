for NrBreaksPerTeam in 0 1 2 3; do # 0 1 2 3
    for Capacity in 0 1; do # 0 1
        if [ $Capacity -eq 1 ]; then
            for Setting in 1 2; do
                rm -rf /Results/Miao/VariableCapacity/Setting$Setting/b$NrBreaksPerTeam/Failures/.*
                rm -rf /Results/Miao/VariableCapacity/Setting$Setting/b$NrBreaksPerTeam/Objectives/.*
            done
        else
            rm -rf /Results/Miao/ConstantCapacity/b$NrBreaksPerTeam/Failures/.*
            rm -rf /Results/Miao/ConstantCapacity/b$NrBreaksPerTeam/Objectives/.*
        fi
    done
done