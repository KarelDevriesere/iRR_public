This provides all the code for the following 2 manuscripts: 
-The incomplete Traveling Tournament Problem, available at https://arxiv.org/abs/2603.19754
-Novel neighborhood structures for incomplete round robin sports tournaments, available at https://arxiv.org/abs/2605.10599

Below, we describe first how to run the code. Then, we explain in detail how all the experiments in the papers were conducted. 

# Compiling and running the code

To compile and run with CMake in debug mode:
1) cmake --build build --config Debug
2) .\build\Debug\test.exe "arguments"
To run in release mode, change Debug for Release

To compile and run with Make:
1) make
2) ./irr "Arguments"
To run in release, do "make release"
To print solutions, do "make PRINT=1"

Gurobi should be loaded, on HPC: module load Gurobi/12.0.0-GCCcore-13.2.0

# Arguments
The following arguments can be passed to the executable: 
* --Seed [+int]: seed. std::mt19937 is used everywhere to generate random numbers
* --Heuristic [0,1]: whether heuristic is used or not to find solutions (if 0, then IP is solved)
* --InstanceTTP [string] : location of iTTP instance, provide full path
* --InstanceFootball [1,..,6]: instance class from the paper Li et al. (2025), 1=S, 2=U13, 3=U15, 4=U17, 5=U21, 6=Ti 
* --CapacitySetting [0,1,2] : capacity setting from the paper of Li et al. (2025) 0: Constant, 1: NC1, 2: NC2
* --MaxNrBreaks [3,100]: max number of breaks allowed per team. I always used either 3 or 100 (the latter corresponds to the setting where there is no limit on the number of breaks)
* --GM [0,1]: whether we run Greedy Matching heuristic or not
* --FO [0,1]: whether we run Fix and Optimize or not
* --Weight [+int]: weight given to the neighborhoods. Based on these weights, the selection probability is calculated
* --TimeLimit [+int]: strict time limit
* --MaxIt [+int]: maximum number of iterations in the algorithm
* --HC [0,1]: whether we use Hill Climbing as a metaheuristic or not
* --SA [0,1]: whether we use Simulated Annealing as a metaheuristic or not
* --T_begin [+double]: begin temperature in SA
* --T_end [+double]: final temperature in SA
* --CoolingRate [+double]: Rate with which we decrease the temperature in SA
* --I_temp [+int]: number of moves before cooling. If this enabled, we cool every time we reach I_temp consecutive iterations
* --I_accept [+int]: if enabled, we cool every time we accept I_accept solutions
* --Reheat [0,1]: whether we allow reheating in SA or not. If enabled, SA runs until the time limit
* --ILS [0,1]: whether we use Iterated Local Search as a metaheuristic
* --ItMaxPert [+int]: if we are in a local optimum and do perturbation, this parameters tells how many perturbation moves we should do
* --LAHC [0,1]: whether we use Late Acceptance Hill Climbing as a metaheuristic or not
* --HistoryLength [+int]: list length in LAHC, if not provided then this is set dynamically
* --PerturbeIncrease [+double] : Value with which we increase the list length if we are stuck in local optimum (default is 1.5)
* --PerturbeValueInitial [+double]: Everytime we reset the history length, we fill the list with values in range [best_obj, best_obj*PerturbeValue_INITIAL] (default is 1.005)
* --PerturbeIncrease [+double]: Number with which we increase PerturbeValue. PerturbeValue is by default initialized to 1.005. PerturbeIncrease is by default set to 0.005
* --VNS [0,1]: whether Variable Neighborhood Search is used as a metaheuristic
* --MAB [0,1]: whether moves are chosen based on Multi Armed Bandit
* --Bounds [0,1]: whether we run the code to generate lower bounds for iTTP instances
* --addMinTripConstraint [0,1]: whether we extend the formulation for the lower bound of iTTP with the MinTrip constraint
* --addColoringConstraint [0,1]: whether we extend the formulation for the lower bound of iTTP with the coloring (i.e. 1F) constraint
* --TripModel [0,1]: whether we solve the model with road trip variables in iTTP
* --TripModelHAPFixed [0,1]: whether we solve the model with the HAPs of teams fixed
* --OutputFolder [string]: every output has its own dedicated folder, if the output file should be stored in another folder, it can be specified here

# DETAILED GUIDE 

In the papers, we always used seeds in the list `{0, 11, 42, 154, 396, 588, 1217, 2486, 5003, 10000}`. 

Moreover, we next assume Make is used, resulting in the executable ./irr

## Computing ILB for iTTP instances

```bash
./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --Bounds 1
```

## Computing DLB for iTTP instances

```bash
./irr --InstanceTTP "Instances/TTP/BRA24_6.xml" --TimeLimit 172800 --Bounds 1 --DLB 1
```

## Computing DLB-1F for iTTP instances

```bash
./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --Bounds 1 --DLB 1 --addColoringConstraint 1
```

## Computing DLB-MinLeg for iTTP instances
./irr --InstanceTTP "Instances/TTP/NL16_12.xml" --TimeLimit 172800 --Bounds 1 --DLB 1 --addMinTripConstraint 1

Only for instances with r > n/2!

## Computing F1 for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --Heuristic 0

## Computing LP_F1 for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --Heuristic 0

Now, set "relax_x" to true in function iTTP() in GurSolver.cpp

## Computing F2 for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --TripModel 1 --Heuristic 0

## Computing LP_F2 for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --TripModel 1 --Heuristic 0

Now, in iTTP_TripModel() in GurSolver.cpp, set GRB_BINARY in definition of z_trs to GRB_CONTINUOUS

## Computing GM-c for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 43200 --GM 1 --Seed 0 --Constructive 1

## Computing GM-it for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 43200 --GM 1 --Seed 0

## Computing F2-HAP for iTTP instances

./irr --InstanceTTP "Instances/TTP/NL16_4.xml" --TimeLimit 172800 --TripModelHAPFixed 1 --Heuristic 0

This assumes solutions for this instance with GreedyMatching for all 10 seeds are available
If not, first compute solutions with GreedyMatching for this instance

## Computing Base for iTTP instances:

./irr --InstanceTTP  "Instances/TTP/NL16_4.xml" --Heuristic 1 --Seed 0 --TimeLimit $TL --Weight TS 1 --Weight PRS 1 --Weight PTS 1 --Weight C 1

## Computing All for iTTP instances:

./irr --InstanceTTP  "Instances/TTP/NL16_4.xml" --Heuristic 1 --Seed 0 --TimeLimit $TL --Weight Random_M_Random_PR 1 --Weight iPTS_Random_PR_CR 1 --Weight C 1 --Weight TS 1 --Weight PRS 1

If MAB: add --MAB 1 

## Computing LB and UB with IP for Football instances:

./irr --InstanceFootball 1 --CapacitySetting 0 --MaxNrBreaks 3 --TimeLimit 7200 --Heuristic 0

## Computing LB and IP for Hockey instances:

./irr --InstanceHockey 1 --TimeLimit 7200 --Heuristic 0
