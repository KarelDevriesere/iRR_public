To compile and run with CMake in debug mode:
1) cmake --build build --config Debug
2) .\build\Debug\test.exe "arguments"
To run in release mode, change Debug for Release

To compile and run with Make:
1) make
2) ./irr "Arguments"
To run in release, do "make release"
To print solutions, do "make PRINT=1"

Arguments:
--Seed: <+int> # seed (default is 0)
--Heuristic: <0/1> # 0: run IP, 1: run heuristic (default is 1)
--HistoryLength: <+int> # Lenght of the list in late acceptancy hill climbing, default is 1 (hill climbing)
--InstanceTTP <BRA24/CIRC40/CON40/GAL40/INCR40/LINE40/N16/NFL32/SUP12> # instance (default is N16)
--NrRounds <+int> # NrRounds of iRR TODO: set this information in the instance XML file (default is 4)
--TimeLimit <+int> # Absolute TimeLimit, default is 7200s
--MaxIt <+int> # Max nr of iterations, LAHC will stop if "it > MaxIt && it_idle > it*0.02" (default is 1 000 000)
--Weight <move> <+int> # Specify the weight of a move (see below*)
--ConstrViolationCost <+int> # cost for violating hard constraints (default is 100 000)
--MinCost <0/1> # whether the moves that can take costs into account do this yes (1) or no (0), (default is 0, but without flag both MinCost and Random moves will be included (see below*))
--Base <0/1> # 1 for running base algorithm, default is 0
--help # to print instructions on terminal

*The following moves are available:
TS,
PTS,
RS,
PRS,
C,
MinCost_BM, 
Random_BM,
PTS_MinCost_PR, 
PTS_Random_PR,
MinCost_M_MinCost_PR, 
MinCost_M_Random_PR, 
Random_M_MinCost_PR, 
Random_M_Random_PR

together with the shortcuts:
BM,
M

If no weights are specified:
-If Base is specified, TS,PTS,RS,PRS and C will be included
-If MinCost is specified to 1, all moves with "Random" will be excluded
-If MinCost is specified to 0, all moves with "MinCost" will be excluded
-All included moves will have uniform weight

If at least 1 weight is specified:
-Only the moves which have a specified weight will be included
-For the MinCost moves, BM,M and PTS can be specified together with the flag MinCost
-If BM,M or PTS is specified but the flag MinCost is not used, the weight will uniformly be distributed over the Random and MinCost variants

Tuning:
For now, tune the following parameters:
-Weight TS,
-Weight RS,
-Weight PRS,
-Weight PTS,
-Weight C,
-Weight BM,
-Weight M,
-MinCost (1 or 0)

