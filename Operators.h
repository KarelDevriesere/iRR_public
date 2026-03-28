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

const int INF = 1000000; // large enough?

struct EdgeNode{ // (a -> h)
    int a, h;
    EdgeNode() = default;
    EdgeNode(const int j, const int i){
        a = j;
        h = i;
    }
};

// We want one graph, do not delete edges and nodes but instead modify the weights
// i.e. if node u is forbidden, set a cost of INF to all edges u->v

struct LineGraph{
    int N,R;
    int SOURCE,SINK;
    vector<EdgeNode>Nodes;
    vector<vector<int>>Adj; // Adj[u] = {v,w} if u -> v and u -> w
    vector<vector<int>>Costs; // Real costs -> initialized only once per search!
    vector<vector<int>>Weights; // Weights on the graph -> updated during the search

    void init(const int n, const int r){
        // N = (n/2)*r
        N = (n/2)*r; // +2 because we need a dummy and sink
        R = r;
        // when constructing a new line graph, overwrite everything
        Nodes.resize(N);
        Adj.resize(N+2); // we do not know the size of Adj beforehand because teams can be unbalanced!
        Costs.assign(N+2, vector<int>(N+2));
        Weights.assign(N+2, vector<int>(N+2));
        SOURCE = N;
        SINK = N+1;
    }

    void reset(){
        for (auto& inner_vec: Adj){
            inner_vec.clear();
            inner_vec.reserve(R); // in worst case a team plays away in all rounds
        }
    }
};

struct Lantarn{
    int i;
    vector<int>middle;
    int j;
    bool InfeasibleOpponents = false;
    bool InfeasibleColor = false;
    bool PathReversalNeeded = false;
    vector<int>c_; // this is needed for the alternative PTS->color bordering to the infeasible color
    vector<int>up; // i <- k <- j
    vector<int>down; // i -> k -> j

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
        up.clear();
        down.clear();
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
        vector<vector<int>>Adj;
        vector<array<int,3>> path;
        vector<vector<array<int,3>>>PathsAC;
        vector<int>H_teamsAC;
        vector<int>A_teamsAC;
        vector<pair<int,int>> AlternatingCycle;
        vector<int>Cycle_AC; 
        vector<uint8_t>ClubSeen;
        vector<vector<int>>EdgeWeight;
        vector<vector<int>>dist;
        vector<vector<int>>predAC;
        vector<uint8_t>SwapColorLantarn; // This is for when we do cycle reversals in the lantern
        // If no: the middle team keeps its orientation in each round
        vector<vector<int>>distLG;
        vector<vector<int>>predLG;

        Lantarn lantarn;
        LineGraph LineGraph;

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

        void clearAdj_AC(){
            for (auto& inner_vec: Adj){
                inner_vec.clear();
                inner_vec.reserve(N);
            }
        }

        void clearFloydWarshall(){
            for (auto& inner_vec: EdgeWeight){
                std::fill(inner_vec.begin(), inner_vec.end(), INF);
            }
            for (auto& inner_vec: dist){
                std::fill(inner_vec.begin(), inner_vec.end(), 2*INF);
            }
            for (auto& inner_vec: predAC){
                std::fill(inner_vec.begin(), inner_vec.end(), -1);
            }
        }

        void clearFloydWarshallLineGraph(){
            for (auto& inner_vec: distLG){
                std::fill(inner_vec.begin(), inner_vec.end(), 2*INF);
            }
            for (auto& inner_vec: predLG){
                std::fill(inner_vec.begin(), inner_vec.end(), -1);
            }
        }

        void clearCycle_AC(){
            Cycle_AC.clear();
            Cycle_AC.reserve(N);
        }

        void clearSwapColorLantarn(){
            std::fill(SwapColorLantarn.begin(), SwapColorLantarn.end(), 1);
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
            Adj.resize(N);
            EdgeWeight.resize(N, vector<int>(N, INF));
            dist.resize(N, vector<int>(N, 2*INF));
            predAC.resize(N, vector<int>(N,-1));
            SwapColorLantarn.resize(N,1);
            LineGraph.init(N,R);
            distLG.resize(LineGraph.N+2, vector<int>(LineGraph.N+2, 2*INF));
            predLG.resize(LineGraph.N+2, vector<int>(LineGraph.N+2,-1));
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

        int DeltaiPTS_TS(const int k, int r, int s);

        void DeltaLantarn(int& delta);

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

        void FloydWarshall(vector<vector<int>>& dist, vector<vector<int>>& pred); // both for normal and alternating cycles -> set dist and pred accordingly

        //iPTS:
        void CreateLantarn(const int i, const int j, const int startColor);

        void SwapColorsLantarn(vector<HA>& OrientationCopy_i, vector<HA>& OrientationCopy_j);

        void ReplenishLantarn(const int i, const int j, const int startColor);

        bool RepairOrientationsEdgesLantarn(const bool MinCostP, int& delta);

        // Alternating cycles:

        // General purpose:
        void EvaluateAlternatingCycleWithPaths(const int r, const bool bipartite, int& delta, const bool MinCostP);

        // General purpose:
        void AlternatingCycleBM(const int l, const int r, const bool bipartite);

        // MinCost Alternating Cycle:
        int ComputeEdgeWeightM(const int i, const int j, const int c, const bool MinCostM, const bool bipartite);

        void PrepareFloydWarshallAlternatingCycle(const int r);

        void FindMinCostBalancedACycle(const int r);

        int StartNodeAlternatingCycleFloydWarshall(const int r);

        void RetrieveAlternatingCycleFloydWarshall(const int start_node, const int r);

        // Balanced Random Alternating Cycle:
        bool DFS_cycle(int u);

        // (Un)balanced Random Alternating Cycle:
        bool DFS_Modified(int u, const int l, const int r);

        // Paths:

        // Helper for DFS_path
        bool DFS_path_recursion(const int current, const int sink);

        bool DFS_path(const int source, const int sink);

        bool BFS_path(const int source, const int sink);

        bool FindNormalPathOneLeague(const int source, const int sink, int& delta, const bool ComputeDelta);

        // Helpers paths and cycles:

        void GoBackToOldCycle(const int r);

        int ReversePath(const bool PR, const bool ComputeDelta);

        // LineGraph:

        struct State{
            vector<int>ForbiddenNodes; // if node is forbidden: set cost of INF for going from this node to its neighbors
            // int LB;
            // bool operator>(const State& other)const{return LB > other.LB;} // when we want to find the most negative, we can use this
        };

        void RestoreWeights(const State& curr, const bool NC);

        void AddEdgeToLineGraph(const int a, const int h, const int weight);

        void MakeLineGraph(const int source, const int sink);

        bool ShortestPathLineGraph(const int source, const int sink, int& delta);

        bool SPFALineGraph(vector<int>& Pred, vector<int>& Cycle, int& delta);

        bool FindCycleLineGraph(const double& current_obj, const bool NC);

        bool CycleWithoutTTPViolations(const double& current_obj);

        bool RetrieveCycleLineGraphFloydWarshall(vector<int>& Cycle, int& delta);

};

// MinCost cycles and paths:

vector<array<int,3>> NonIncreasingCycle(Solution& sol);

vector<pair<int,int>>ForbiddenPairNC(Solution& sol, const vector<array<int,3>> path);

#endif
