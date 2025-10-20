To compile and run with CMake in debug mode:
1) cmake --build build --config Debug
2) .\build\Debug\test.exe "arguments"
To run in release mode, change Debug for Release

To compile and run with Make:
1) make
2) ./irr "arguments"
To run in release, do "make release"
To print solutions, do "make PRINT=1"

Now, the argument list is as follows:
0 $seed $heuristic $MiNCostNB $HL $NrTeams $k $i

0 for Cost Minimization instances, 1 for instances of Miao or Hockey (do not do this)
seed = any positive integer
heuristic = 0 for running heuristic and 1 for running IP
MiNCostNB = 0 for running without considering costs (i.e. random matchings and paths) and 1 for with costs (minimum weight matchings, shortest paths)
HL = length of the list with historic values in LAHC (positive integer > 0)
NrTeams = 36 or 100
k = 0,1,5 or 10 (extra degree above the NrRounds)
i = 0,1,2,3,4 (each instance apperas 5x with different cost matrices)

Automatically, the instances with cost matrices with costs between 0-100 will be chosen