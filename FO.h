#ifndef FO_H  
#define FO_H

#include "GurSolver.h"
#include "SA.h"

// R1: 1 round free, R2: 2 rounds free, R3: 3 rounds free, T: free subset of teams
enum class FO_move{R1, R2C, R3C, R2NC, R3NC, T}; 
const unordered_map<FO_move, string>FO_name_string = {{FO_move::R1, "R1"}, {FO_move::R2C, "R2C"}, {FO_move::R3C, "R3C"}, 
    {FO_move::R2NC, "R2NC"}, {FO_move::R3NC, "R3NC"}, {FO_move::T, "T"}}; // C: consecutive, NC: non consecutive

class FO: public GurSolver, public SA<FO_move>
{
    private:
        vector<vector<vector<bool>>>x_value;
        vector<vector<vector<bool>>>x_fixed;

    public:
        FO(Input& in, const std::unordered_map<FO_move, string>& moves, const std::unordered_map<FO_move, double>& weights, const int seed);
        ~FO();

        double PercFreeTeams = 0.05;
        unordered_map<FO_move, double>PercentageHapsFixed = {{FO_move::R1, 0.0}, {FO_move::R2C, 0.25}, {FO_move::R3C, 0.50}, 
            {FO_move::R2NC, 0.25}, {FO_move::R3NC, 0.50}, {FO_move::T, 0.10}}; // Number of Haps we fix based on the number of rounds
        unordered_map<FO_move, double>TimeLimit = {{FO_move::R1, 60.0}, {FO_move::R2C, 60.0}, {FO_move::R3C, 60.0}, 
            {FO_move::R2NC, 60.0}, {FO_move::R3NC, 60.0}, {FO_move::T, 60.0}}; // in seconds!!
        
        void InitializeModel(Solution& sol);
        void FreeRounds(const int nr, const bool consecutive);
        void FreeTeams();
        void fix_all();
        void unfix_all();
        void FixVariables();
        void SetDualProbs();
        void UpdateSizeFixedVariables(const FO_move move, const bool optimal);
        void UpdateSelectionProbabilities();
        void FreeVariables();
        void Validate();
        void Set_x_value_from_sol(Solution& sol);
        void Store_x_value();

        void solve(Input& in, Solution& sol);
};

#endif