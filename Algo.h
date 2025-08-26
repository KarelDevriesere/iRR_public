#ifndef ALGO_H  
#define ALGO_H

// #include "Graph.h"
#include "Solution.h"

void setHAofMatch(int& a, int& b, int& c, Solution& sol);

void setHaps(int& c1, int& c2, Solution& sol);

void RepairBalanceHA(Solution& sol);

void SetValueCircleMethod(const int a, const int b, const int c, Solution& sol);

void CircleMethod(vector<int>& Teams, Solution& sol);

void VizingConstruction(Solution& sol);

bool CyclicConstruction(Solution& sol);

int GreedyConstruction(const int l, Solution& sol);

void GreedyPerfectMatchings();

#endif
