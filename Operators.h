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

#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>

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
    bool InfeasibleOpponents = false;
    bool InfeasibleColor = false;
    map<int,int>c_; // this is needed for the alternative PTS->color bordering to the infeasible color
    map<int,int>fictive_nb;
    vector<vector<int>>paths; // vector with all the paths we found outside the lantarn

    bool PathReversalNeeded = false;
};

// Deltas:

int PTSCurrentTravelDelta(const vector<int>& SortedRoundsLantern, const int t, const Solution& sol);

int CostRoundSwapTeamiTTP(const int i, const int r, const int s, Solution& sol);

int DeltaPRS_TTP(Solution& sol, const int r, const int s, const int StartNode);

void PrintEdgeLantarn(const Solution& sol, const int i, const int k, const int j, const Lantarn& lantarn);

void PrintLantarn(const Solution& sol, const Lantarn& lantarn);

int CostTSTeamsTTP(const int i, const int j, const Solution& sol);

int CostTSTeamsYSTP(const int i, const int j, const Solution& sol);

int DeltaPRS_YSTP(Solution& sol, const int r, const int s, const int StartNode);

// int ComputeTotalCostHA(Solution& sol);

// void UpdateNrHA(Solution& sol, const int c_out, const int c_in, const int i, const int j, Solution& sol);

void setMatchColorR(Solution& sol, const int i, const int r, const int s);

void TS(Solution& sol, const int i, const int j);

void RS(Solution& sol, const int r, const int s);

void PRS(Solution& sol, const int r, const int s, const int StartNode);

vector<array<int,3>> NonIncreasingCycle(Solution& sol);

pair<vector<pair<int,int>>,vector<int>>MoveMWPMOneLeague(Solution& sol, const int r, std::mt19937& gen, const int l, const bool bipartite, const bool MinCostM);

vector<vector<array<int,3>>>EvaluateAlternatingCycleWithPaths(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r, const bool bipartite, int& delta, std::mt19937& gen, const bool MinCostP, bool NoPathDueTo2RRConstraint);

void GoBackToOldCycle(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r);

vector<array<int,3>> CycleBalanced(Solution& sol, std::mt19937& gen);

void ReplenishLantarn(Solution& sol, const int i, const int j, const int startColor, Lantarn& lantarn, int& delta);

Lantarn CreateLantarn(Solution& sol, const int i, const int j, const int startColor, int& delta);

void SwapColorsLantarn(Solution& sol, const Lantarn& lantarn, vector<HA>& OrientationCopy_i, vector<HA>& OrientationCopy_j);

int ReversePath(Solution& sol, const vector<array<int,3>> path, const bool PR, const bool ComputeDelta);

bool RepairOrientationsEdgesLantarn(Solution& sol, Lantarn& lantarn, vector<array<int,3>>& path, const bool MinCostP, int& delta, std::mt19937& gen);

vector<vector<pair<int,int>>>AlternatingCycleBM(Solution& sol, const int r, const bool bipartite, std::mt19937& gen);

bool FindNormalPathOneLeague(const int source, const int sink, Solution& sol, vector<array<int,3>>& path, int& delta, const bool MinCostP, std::mt19937& gen, const bool ComputeDelta);

bool FindPathLineGraphOneLeague(const int source, const int sink, Solution& sol, vector<array<int,3>>& path);

tuple<vector<Edge>,vector<Edge>,vector<int>> MakeLineGraph(Solution& sol, const int source, const int sink);

vector<pair<int,int>>ForbiddenPairNC(Solution& sol, const vector<array<int,3>> path);

vector<vector<pair<int,int>>>FindMinCostBalancedACycle(Solution& sol, const int r, std::mt19937& gen);

#endif
