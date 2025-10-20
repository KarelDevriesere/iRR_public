#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>
#include "SA.h"

// #include "Graph.h"
#include "Solution.h"

using namespace std;

enum class move_name{TS, PTS, RS, PRS, M, BM, C, NC, Initial}; 
enum class failure{InfeasibleOpponents, HAPs, DRR, NoPathFound, MaxSameClub};
const std::unordered_map<failure, string> Failures = {{failure::InfeasibleOpponents, "InfeasibleOpponents"},{failure::HAPs, "HAPs"}, {failure::DRR, "DRR"}, 
    {failure::NoPathFound, "NoPathFound"}, {failure::MaxSameClub, "MaxSameClub"}};


class ILS: public SA<move_name>
{
    private:

        int NrSingleEdgePathChosen = 0;
        int NrMultiEdgePathChosen = 0;
        int NrSingleEdgePathSuccesful = 0; 
        int NrMultiEdgePathSuccesful = 0;

        int NrTimesRepairHapChosen = 0;
        int NrTimesRepairHapSuccesfull = 0;

        bool include_travel;
        bool include_HAP;

        void Move(Solution& sol);
        void repairHAPs(Solution& sol);
        void Perturbation(Solution& sol);
        // Moves for Miao and hockey:
        void SelectTS(Solution& sol);
        void SelectRS(Solution& sol);
        void SelectPTS(Solution& sol);
        void SelectPRS(Solution& sol);
        void SelectBipartiteMatching(const int l, Solution& sol);
        void SelectMatching(const int l, Solution& sol, const bool bipartite); 
        void SelectNegativeCycle(const int l, Solution& sol);
        void SelectBalancedCycle(const int l, Solution& sol);

        bool veto_haps(Solution& sol);
        bool RepairHAPs(Solution& sol);
        // Helpers:
        pair<int,int>SelectTwoTeamsSameLeague(Solution& sol);
        array<int,3>SelectTwoTeamsSameLeagueAndColor(Solution& sol);

        // Analysis of the moves:
        std::unordered_map<move_name, std::unordered_map<failure, int>> FailureReason;
        std::unordered_map<move_name, std::unordered_map<failure, int>> FailureReasonAll
            = {{move_name::TS, {{failure::InfeasibleOpponents, 0}, {failure::HAPs, 0}}},
               {move_name::PTS, {{failure::InfeasibleOpponents, 0}, {failure::HAPs, 0}, {failure::DRR, 0}, {failure::NoPathFound, 0}, {failure::MaxSameClub, 0}}},
               {move_name::PRS, {{failure::HAPs, 0}}},
               {move_name::M, {{failure::HAPs, 0}, {failure::NoPathFound, 0}}},
               {move_name::C, {{failure::DRR, 0}}}
            };

    public:
        ILS(const std::unordered_map<move_name, string>& moves, const std::unordered_map<move_name, double>& weights, std::mt19937& g);
        ~ILS();
        
        // Custom functions already declared in SA:
        void solve(Input& in, Solution& sol) override;

        void SaveResultsFailures(std::string file_path_results_base, int inst, int seed);
};

