#ifndef ALGO_H  
#define ALGO_H

// #include "Graph.h"
#include "Solution.h"

void setHAofMatch(int& a, int& b, int& c, Solution& sol);

void setHaps(const int c1, const int c2, Solution& sol);

void RepairBalanceHA(Solution& sol);

void SetValueCircleMethod(const int a, const int b, const int c, Solution& sol);

void CircleMethod(vector<int>& Teams, Solution& sol);

vector<vector<bool>>VizingRegularColorableGraph(const int N, const int R, const int seed);

void VizingConstruction(Solution& sol, const int seed);

bool CyclicConstruction(Solution& sol, const int seed);

int GreedyConstruction(const int l, Solution& sol);

void GreedyPerfectMatchings();

#endif
