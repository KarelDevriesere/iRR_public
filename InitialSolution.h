#ifndef ALGO_H  
#define ALGO_H

#include "Solution.h"

void setHaps(const int c1, const int c2, Solution& sol);

void RepairBalanceHA(Solution& sol);

void CircleMethod(vector<int>& Teams, Solution& sol);

vector<vector<bool>>VizingRegularColorableGraph(const int N, const int R, const int seed);

void VizingConstruction(Solution& sol, const int seed);

#endif
