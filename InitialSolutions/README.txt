All solutions were obtained with Miao's method of first assigning the HAPs to the teams with IP and then doing 
a sequence of bipartite matchings, in chronological order. The seed value was set to 0. 
I also verified all the bounds with the ones reported in Miao her paper. All v_i did exactly match.
Initial solutions were found rather quick (few seconds) for all instances so far. 

For Tiny and VariableCapacity settings 1 and 2, I used the full IP of Miao to find feasible solutions,
this because the metaheuristic of Miao could not (or at leats not fast) find solutions because the HAP set with minimum
violations was too low for the matchings. All bounds found here were again exactly like in the paper.