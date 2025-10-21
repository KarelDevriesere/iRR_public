#ifndef OPERATORS_H  
#define OPERATORS_H 

// #include "Graph.h"
#include "Input.h"
#include "Solution.h"
#include <map>
#include <random>
#include <assert.h>
#include <iostream>
#include <string>
#include <array>
#include "GurSolver.h"
#include "Algo.h"
#include <algorithm>

#include <boost/config.hpp>
#include <fstream>
#include <iomanip>
#include <boost/graph/edge_coloring.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>

// TODO Overwrite maximum weighted matching with the newest boost file.
// See https://github.com/boostorg/graph/issues/399
//#include <boost/graph/maximum_weighted_matching.hpp>
#include "maximum_weighted_matching.hpp"
// #include <boost/graph/max_cardinality_matchinsol.hpp>


#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <boost/graph/graphviz.hpp> // for the dot file

typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property <boost::edge_weight_t, int>>BGraph;
// shortest path needs listS?
// typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, boost::no_property, boost::property< boost::edge_weight_t, int > >BGraph;
typedef pair<int, int>Edge;
typedef boost::property_map<BGraph, boost::edge_weight_t>::type WeightMap;
typedef boost::graph_traits<BGraph>::vertex_descriptor Vertex;

struct Lantarn{
    int i;
    vector<int>middle;
    vector<bool>Swapped; // vector with whether we swapped the direction of i-k-j
    int j;
    map<int,int>NrH;
    bool InfeasibleOpponents = false;
    bool InfeasibleColor = false;
    bool Infeasible2RRMatch = false;
    bool MaxSameClubViolated = false;
    map<int,int>c_; // this is needed for the alternative PTS->color bordering to the infeasible color
    map<int,int>fictive_nb;
    vector<vector<int>>paths; // vector with all the paths we found outside the lantarn

    // the following 2 are solely for DRR:

    vector<pair<bool, bool>>MatchAlreadyPresent; // this is for DRR, it can happen that team i plays 2x vs the same middle team k but
    // in different colors, so then we should take a different matchcolor from k to j
    // So this k measures whether k plays already plays H vs j or A
    unordered_map<int, vector<pair<int,int>>>EdgesMatch; // whether edge {i,j} contains match (i,j) or match (j,i)
};

bool MaxSameClubViolated(Solution& sol, Lantarn& lantarn);

void PrintEdgeLantarn(Solution& sol, const int i, const int k, const int j, Lantarn& lantarn);

void PrintLantarn(Solution& sol, Lantarn& lantarn);

// int ComputeTotalCostHA(Solution& sol);

// void UpdateNrHA(Solution& sol, const int c_out, const int c_in, const int i, const int j, Solution& sol);

void setMatchColorR(Solution& sol, const int i, const int r, const int s);

bool TS_feasible(Solution& sol, const int i, const int j);

void TS(Solution& sol, const int i, const int j);

void RS(Solution& sol, const int r, const int s);

void PRS(Solution& sol, const int r, const int s, const int StartNode);

void PRS_same_direction(Solution& sol, const int r, const int s, const int startNode);

void PTS_default(Solution& sol, const int i, const int j, const int startColor);

int cost_PTS_infeasible_coloring_default(Solution& sol, const int i, const int j, const int startColor);

int CostEvaluationTS_travel(Solution& sol, const int i, const int j);

int CostEvaluationRS_travel(Solution& sol, const int r, const int s);

int CostEvaluationPTS_travel(Solution& sol, const int i, const int j, const int startColor);

int CostEvaluationPRS_travel(Solution& sol, const int i, const int j, const int startNode);

int CostEvaluationTS_HA(Solution& sol, const int i, const int j);

int CostEvaluationRS_HA(Solution& sol, const int r, const int s);

int CostEvaluationPRS_HA(Solution& sol, const int r, const int s, const int startNode);

int CostEvaluationPTS_HA(Solution& sol, const int i, const int j, const int startColor);

int CostEvaluationTS(Solution& sol, const int i, const int j, const bool travel, const bool HA);

int CostEvaluationRS(Solution& sol, const int r, const int s, const bool travel, const bool HA);

int CostEvaluationPRS(Solution& sol, const int r, const int s, const int startNode, const bool travel, const bool HA);

int CostEvaluationPTS(Solution& sol, const int i, const int j, const int startColor, const bool travel, const bool HA);

void setAllWeightsBF(Solution& sol);

void updateWeightBF(Solution& sol, const int i, const int j);

int UpdateCycleBF(vector<int>& Cycle, Solution& sol);

int MinimizeBreaksCycleBoost(Solution& sol);

int NegativeCycleBoost(Solution& sol);

void SwapMatchings(Solution& sol, vector<pair<int,int>>Matching, const int l, const int r, const bool bipartite);

pair<vector<pair<int,int>>,vector<int>>MoveMWPM(Solution& sol, const int l, const int r, const bool bipartite, const bool includeHAPs, const bool CM, std::mt19937& gen, const bool MinCostM);

vector<vector<pair<int,int>>>iPRS(Solution& sol, const int l, const int r, const bool bipartite, const bool includeHAPs, const bool CM, std::mt19937& gen, const bool MinCostM);

vector<vector<array<int,3>>>EvaluateAlternatingCycleWithPaths(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r, const bool bipartite, const bool CM, int& delta, std::mt19937& gen, const bool MinCostP);

void GoBackToOldCycle(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r);

int PTS_infeasible_coloring(Solution& sol, int &i, const int j, const int startColor, bool& evaluate_cost, const bool swap);

int NegativeCycle(Solution& sol);

void ReverseCycle(vector<int>& Cycle, Solution& sol);

vector<array<int,3>> CycleBalanced(Solution& sol, std::mt19937& gen);

int PTS_infeasible_coloring_with_infeasible_color(Solution& sol, int &i, const int j, const int startColor, const bool evaluate_cost, const bool swap);

void ReplenishLantarn(Solution& sol, const int i, const int j, const int startColor, Lantarn& lantarn);

int CostEvaluationTravelLantarn(Solution& sol, Lantarn& lantarn);

int CostEvaluationHAPsLantarnTraditional(Solution& sol, Lantarn& lantarn);

int CostEvaluationHAPsLantarnAlternative(Solution& sol, Lantarn& lantarn);

Lantarn CreateLantarn(Solution& sol, const int i, const int j, const int startColor);

void SwapColorsLantarn(Solution& sol, Lantarn& lantarn);

vector<int>PathBoostLantarn(Solution& sol, Lantarn& lantarn);

void SwapPathBoostLantarn(Solution& sol, vector<int>& path);

bool LantarnContainsOneTeamPerClubPerColor(Solution& sol, Lantarn& lantarn);

bool OptimizeCyclesLantarn(Solution& sol, Lantarn& lantarn);

void OptimizeOrientationsCyclesLantarn(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy);

void ReversePath(Solution& sol, const vector<array<int,3>> path);

vector<vector<int>> ReversePathsMatching(Solution& sol, const vector<pair<int,int>> Matching, const int l, const int r);

void KeepOrientationsAllEdgesLantarn(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy);

bool RepairOrientationsEdgesLantarn_CM(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy, vector<array<int,3>>& path, const bool MinCostP, const bool CM);

bool SetOrientationsEdgesLantarn(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy, vector<array<int,3>>& path, const bool MinCostP, const bool CM);

bool RepairHAPsWithNegativeCycles(Solution& sol, const int l);

bool FindNormalPathOneLeague(const int source, const int sink, Solution& sol, vector<array<int,3>>& path, int& delta, const bool MinCostP);

void ReversePathLineGraph(Solution& sol, const vector<int>& path, const vector<Edge>& Nodes);

tuple<vector<Edge>,vector<Edge>,vector<int>> MakeLineGraphOnlyZeroWeightEdges(Solution& sol, const int source, const int sink, const int source_nb, const int sink_nb);

#endif
