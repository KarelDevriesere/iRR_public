#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>

#include "Meta.h"
#include "Solution.h"

using namespace std;

class Heuristic_CM: public LAHC<Move>
{
    private:
        array<int,3>SelectTwoTeamsAndColorMinCost(Solution& sol);
        array<int,3>SelectTwoTeamsAndColor(Solution& sol);
        pair<int,int>SelectTwoTeams(Solution& sol);
        pair<int,int>SelectTwoRounds(Solution& sol);
        // Moves for cost minimization
        void SelectTS(Solution& sol);
        void SelectPTS(Solution& sol);
        void SelectRS(Solution& sol);
        void SelectPRS(Solution& sol);
        void SelectMatching(Solution& sol, const bool bipartite);
        void SelectBalancedCycle(Solution& sol);

        bool MinCostPR = false; // MinCost Path
        bool MinCostM = false; // MinCost Matching
        bool MinCostC = false; // MinCost cycle

        bool CM = false;

    public:
        Heuristic_CM(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g, const int HistoryLength, const int obj);
        ~Heuristic_CM();
        
        void DoMove(Solution& sol);

        // Custom functions already declared in LACH:
        void solve(Input& in, Solution& sol) override;

	void TeamSwapper(Input& in, Solution& sol);
};
