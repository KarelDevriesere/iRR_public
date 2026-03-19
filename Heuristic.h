#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>

#include "Meta.h"
#include "Solution.h"

using namespace std;

class Heuristic: public LAHC<Move>
{
    private:
        array<int,3>SelectTwoTeamsAndColorMinCost(Solution& sol);
        array<int,3>SelectTwoTeamsAndColor(Solution& sol);
        pair<int,int>SelectTwoTeams(Solution& sol);
        pair<int,int>SelectTwoRounds(Solution& sol);
        // Moves for cost minimization
        void SelectTS(Solution& sol);
        void SelectiPTS(Solution& sol);
        void SelectRS(Solution& sol);
        void SelectPRS(Solution& sol);
        void SelectiPRS(Solution& sol, const bool bipartite);
        void SelectBalancedCycle(Solution& sol);

        bool MinCostPR = false; // MinCost Path
        bool MinCostAC = false; // MinCost Alternating Cycle
        bool MinCostC = false; // MinCost cycle

        // Helpers (construct these once to avoid overhead):
        vector<int>ColoredRoundsLantern; // iPTS
        vector<HA>OrientationCopy_i; // iPTS
        vector<HA>OrientationCopy_j; // iPTS

    public:
        Heuristic(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g, const int HistoryLength, const int obj);
        ~Heuristic();
        
        void DoMove(Solution& sol);

        // Custom functions already declared in LAHC:
        void solve(Input& in, Solution& sol) override;

        // void Perturbe(Solution& sol) override; // ILS

	void TeamSwapper(Input& in, Solution& sol);
};
