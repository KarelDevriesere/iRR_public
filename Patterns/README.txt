To generate patterns, use 
NL4 -> 6 rounds
NL5 -> 8 rounds
NL6 -> 10 rounds
NL7 -> 12 rounds
NL8 -> 14 rounds
NL11 -> 20 rounds
NL13 -> 24 rounds
NL16 -> 30 rounds

Next, in the RobinX code (under Projects/JPL), set usePatterns = true; in source/interface/main.cpp. 
Also uncomment the outfile pieces of the code. To generate patterns, adapt the file name into 0,1,2,3 for the number of breaks. In all_patterns, we only use the constraints of no HHH and AAA.
In the makefile, disable JPL
Recompile
$ make
Run
$ ./robinx --instanceFile ./PatternsKarel/NL4.xml



which will print all patterns on the screen. Write these to a file.
