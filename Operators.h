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

typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property <boost::edge_weight_t, int>>BGraph;
// shortest path needs listS?
// typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, boost::no_property, boost::property< boost::edge_weight_t, int > >BGraph;
typedef pair<int, int>Edge;
typedef boost::property_map<BGraph, boost::edge_weight_t>::type WeightMap;
typedef boost::graph_traits<BGraph>::vertex_descriptor Vertex;

struct Lantarn{
    int i;
    vector<int>middle;
    int j;
    bool InfeasibleOpponents = false;
    bool InfeasibleColor = false;
    bool PathReversalNeeded = false;
    vector<int>c_; // this is needed for the alternative PTS->color bordering to the infeasible color

    void init(const int N){
        c_.resize(N,-1);
    }

    void reset(const int new_i, const int new_j){
        i = new_i;
        j = new_j;
        InfeasibleOpponents = false;
        InfeasibleColor = false;
        PathReversalNeeded = false;
        middle.clear();
        // c_ will automatically be overwritten
    }
};

class Operator{
    protected:
        Solution& sol;
        const int N;
        const int R;
        std::mt19937& gen;

        // Helpers:
        vector<uint8_t>Visited;
        vector<uint8_t>Stack;
        vector<int>Count;
        vector<int>Pred;
        vector<int>Parent;
        vector<int>Queue;
        vector<array<int,3>> path;
        vector<vector<array<int,3>>>PathsAC;
        vector<int>H_teamsAC;
        vector<int>A_teamsAC;
        vector<pair<int,int>> AlternatingCycle;
        vector<uint8_t>ClubSeen;

        Lantarn lantarn;

        void clearVisited(){
            std::fill(Visited.begin(), Visited.end(), 0);
        }

        void clearStack(){
            std::fill(Stack.begin(), Stack.end(), 0);
        }

        void clearCount(){
            std::fill(Count.begin(), Count.end(), -1);
        }

        void clearPred(){
            std::fill(Pred.begin(), Pred.end(), -1);
        }

        void clearParent(){
            std::fill(Parent.begin(), Parent.end(), -1);
        }

        void clearPath(){
            path.clear();
            path.reserve(N);
        }

        void clearPathsAC(){
            PathsAC.clear();
            PathsAC.reserve(N/2);
        }

        void clearHA_teamsAC(){
            H_teamsAC.clear();
            A_teamsAC.clear();
            H_teamsAC.reserve(N/2);
            A_teamsAC.reserve(N/2);
        }

        void clearAlternatingCycle(){
            AlternatingCycle.clear();
            AlternatingCycle.reserve(N);
        }

        void clearClubSeen(){
            std::fill(ClubSeen.begin(), ClubSeen.end(), 0);
        }


    public:
        Operator(Solution& current_sol, std::mt19937& current_gen): sol(current_sol), N(sol.getNrTeams()), R(sol.getNrRounds()), gen(current_gen){
            Visited.resize(N, 0);
            Stack.resize(N,0);
            Count.resize(N,-1);
            Pred.resize(N,-1);
            Parent.resize(N,-1);
            Queue.resize(N,0);
            lantarn.init(N);
            ClubSeen.resize(sol.getNrClubs(),0);
        };

        // Deltas:

        int getLocSwapped(const int i, const int round);

        int getLoc(const int i, const int round);

        int DeltaTravelOrientationSwapTeam(const int i, const int r, const int s);

        int DeltaTravelRoundSwapTeam(const int i, const int r, const int s);

        int CostNrConsecutiveHA(const int i, const int min_round, const int max_round);

        int DeltaHACostOrientationSwapTeam(const int i, const int r, const int s);

        int CostOrientationSwapTeamiTTP(const int i, const int r, const int s);

        int PTSCurrentTravelDelta(const vector<int>& SortedRoundsLantern, const int t);

        int CostRoundSwapTeamiTTP(const int i, const int r, const int s);

        int CostUncoloredRoundSwapTeamiTTP(const int i, const int UncoloredOpponent_i, const int r, const int s);

        int DeltaPRS_TTP(const int r, const int s, const int StartNode);

        int CostTSTeamsTTP(const int i, const int j);

        int CostTSTeamsYSTP(const int i, const int j);

        int DeltaPRS_YSTP(const int r, const int s, const int StartNode);

        // Debugging:
        void PrintEdgeLantarn(const int i, const int k, const int j);

        void PrintLantarn();

        // Operators:

        // TeamSwap:
        void TS(const int i, const int j);

        // RoundSwap:
        void RS(const int r, const int s);

        void setMatchColorR(const int i, const int r, const int s);

        // PartialRoundSwap:
        void PRS(const int r, const int s, const int StartNode);

        // CycleReversal:
        void CycleBalanced();

        //iPTS:
        void CreateLantarn(const int i, const int j, const int startColor, int& delta);

        void SwapColorsLantarn(vector<HA>& OrientationCopy_i, vector<HA>& OrientationCopy_j);

        void ReplenishLantarn(const int i, const int j, const int startColor, int& delta);

        bool RepairOrientationsEdgesLantarn(const bool MinCostP, int& delta);

        // Alternating cycles:

        // General purpose:
        void EvaluateAlternatingCycleWithPaths(const int r, const bool bipartite, int& delta, const bool MinCostP, bool NoPathDueTo2RRConstraint);

        // General purpose:
        void AlternatingCycleBM(const int r, const bool bipartite);

        // MinCost Alternating Cycle:
        int ComputeEdgeWeightM(const int i, const int j, const int c, const bool MinCostM, const bool bipartite);

        void FindMinCostBalancedACycle(const int r);

        // Balanced Random Alternating Cycle:
        bool DFS_cycle(int u, vector<int>& Cycle, const vector<vector<int>>& Adj);

        // (Un)balanced Random Alternating Cycle:
        bool DFS_Modified(int u, vector<int>& Cycle, vector<int>& AdjC, vector<vector<int>>& Adj);

        // Paths:

        // Helper for DFS_path
        bool DFS_path_recursion(const int current, const int sink);

        bool DFS_path(const int source, const int sink);

        bool BFS_path(const int source, const int sink);

        bool FindNormalPathOneLeague(const int source, const int sink, int& delta, const bool MinCostP, const bool ComputeDelta);

        // Helpers paths and cycles:

        void GoBackToOldCycle(const int r);

        int ReversePath(const bool PR, const bool ComputeDelta);

};

// MinCost cycles and paths:

vector<array<int,3>> NonIncreasingCycle(Solution& sol);

tuple<vector<Edge>,vector<Edge>,vector<int>> MakeLineGraph(Solution& sol, const int source, const int sink);

vector<pair<int,int>>ForbiddenPairNC(Solution& sol, const vector<array<int,3>> path);

#endif
