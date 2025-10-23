#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>

#include "SA.h"
#include "Solution.h"

using namespace std;

enum class move_name_CM{TS, PTS, RS, PRS, M, BM, C, NC, Initial}; 
const unordered_map<move_name_CM, string>Moves = {{move_name_CM::TS, "TS"}, {move_name_CM::PTS, "PTS"}, {move_name_CM::RS, "RS"}, {move_name_CM::PRS, "PRS"}, {move_name_CM::M, "M"}, {move_name_CM::BM, "BM"}, {move_name_CM::C, "C"}}; 
// Cycle gives problems!!

class Heuristic_CM: public LAHC<move_name_CM>
{
    private:
        array<int,3>SelectTwoTeamsAndColorMinCost(Solution& sol);
        array<int,3>SelectTwoTeamsAndColor(Solution& sol);
        pair<int,int>SelectTwoTeams(Solution& sol);
        pair<int,int>SelectTwoRounds(Solution& sol);
        // Moves for cost minimization
        void SelectTS_CM(Solution& sol);
        void SelectPTS_CM(Solution& sol);
        void SelectRS_CM(Solution& sol);
        void SelectPRS_CM(Solution& sol);
        void SelectMatching_CM(Solution& sol, const bool bipartite);
        void SelectBalancedCycle_CM(Solution& sol);

        bool MinCostP = false; // MinCost Path
        bool MinCostM = false; // MinCost Matching
        bool MinCostC = false; // MinCost cycle
        bool MinCostPTS = false;

    public:
        Heuristic_CM(const std::unordered_map<move_name_CM, string>& moves, const std::unordered_map<move_name_CM, double>& weights, std::mt19937& g, const int HistoryLength, const int obj, const bool MinCostNB);
        ~Heuristic_CM();
        
        void Move(Solution& sol);

        // Custom functions already declared in LACH:
        void solve(Input& in, Solution& sol) override;
};