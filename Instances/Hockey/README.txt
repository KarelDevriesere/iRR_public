---- General Info -----
# Number of teams # Number of leagues # Number of clubs # Number of rounds # 0: Constant capacity, 1: variable capacity
34 2 19 6 0
-----------------------
-------- Clubs --------
# Each row is a club with the following information: # Number of teams in club # Capacity of this club in each round (constant) an d # Team indices belonging to this club
# So the next club represents the first club which has 3 teams with indices 0,1 and 2 and has a constant capacity of 2 in each round
3 2 0 1 2 
1 1 3
1 1 4
2 1 5 22
1 1 6
4 2 7 8 23 24
2 1 9 25
1 1 10
1 1 11
3 2 12 13 27
2 1 14 28
3 2 15 16 29
1 1 17
2 1 18 30
2 1 19 31
1 1 20
2 1 21 32
1 1 26
1 1 33
-----------------------
--- Travel distance ---
# Travel distance matrix: cell (i,j) is the travel time between clubs i and j
0 43 36 69 96 19 80 74 91 82 87 86 115 48 58 78 81 77 0
43 0 24 48 72 32 51 46 63 54 58 58 88 20 30 51 75 56 0
36 24 0 48 75 27 61 65 77 63 72 80 110 41 53 73 88 56 0
69 48 48 0 40 59 17 23 37 21 26 38 69 33 25 32 88 14 0
96 72 75 40 0 85 32 38 26 20 32 30 67 58 49 42 102 29 0
19 32 27 59 85 0 68 63 79 71 75 74 105 36 47 68 62 66 0
80 51 61 17 32 68 0 11 23 13 17 31 63 38 28 26 81 20 0
74 46 65 23 38 63 11 0 25 20 22 26 57 32 22 20 76 24 0
91 63 77 37 26 79 23 25 0 23 16 10 47 49 39 21 85 32 0
82 54 63 21 20 71 13 20 23 0 12 28 65 40 30 28 84 12 0
87 58 72 26 32 75 17 22 16 12 0 21 58 45 35 33 88 20 0
86 58 80 38 30 74 31 26 10 28 21 0 41 44 35 15 80 37 0
115 88 110 69 67 105 63 57 47 65 58 41 0 75 65 45 89 69 0
48 20 41 33 58 36 38 32 49 40 45 44 75 0 17 38 71 43 0
58 30 53 25 49 47 28 22 39 30 35 35 65 17 0 28 68 34 0
78 51 73 32 42 68 26 20 21 28 33 15 45 38 28 0 75 31 0
81 75 88 88 102 62 81 76 85 84 88 80 89 71 68 75 0 89 0
77 56 56 14 29 66 20 24 32 12 20 37 69 43 34 31 89 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
-----------------------
------- Leagues -------
# Each row here contains elements equal to the number of teams in that leagues
# Each element is the strength of the team in that league 
# I.e. here, the first league contains 22 teams, all with strength 1, while the second league contains 12 teams, also all with strength 1
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
1 1 1 1 1 1 1 1 1 1 1 1
-----------------------