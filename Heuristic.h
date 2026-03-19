#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>

#include "Meta.h"
#include "Solution.h"
#include "Operators.h"

using namespace std;

class Heuristic: public LAHC<Move>, public Operator
{
    private:
        array<int,3>SelectTwoTeamsAndColorMinCost();
        array<int,3>SelectTwoTeamsAndColor();
        pair<int,int>SelectTwoTeams();
        pair<int,int>SelectTwoRounds();
        // Moves for cost minimization
        void SelectTS();
        void SelectiPTS();
        void SelectRS();
        void SelectPRS();
        void SelectiPRS(const bool bipartite);
        void SelectBalancedCycle();

        bool MinCostPR = false; // MinCost Path
        bool MinCostAC = false; // MinCost Alternating Cycle
        bool MinCostC = false; // MinCost cycle

        // Helpers (construct these once to avoid overhead):
        pair<int,int>PairOfTeams;
        pair<int,int>PairOfRounds;
        array<int,3>PairOfTeamsAndColor;
        vector<int>ColoredRoundsLantern; // iPTS
        vector<HA>OrientationCopy_i; // iPTS
        vector<HA>OrientationCopy_j; // iPTS
        vector<int>CostBeforeTTPTeams; // Unbalanced iPRS

    public:
        Heuristic(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g, const int HistoryLength, const int obj, Solution& current_sol);
        ~Heuristic();
        
        void DoMove();

        // Custom functions already declared in LAHC:
        void solve(Input& in, Solution& sol) override;

        // void Perturbe(Solution& sol) override; // ILS

	    void TeamSwapper();
};
