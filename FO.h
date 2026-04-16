#ifndef FO_H  
#define FO_H

#include "GurSolver.h"
#include "Meta.h"
#include "Input.h"

class FO: public GurSolver, public MoveExecutor
{
    private:
        std::unique_ptr<MetaBase<FO_move>> MetaH;

        vector<vector<vector<bool>>>x_value;
        vector<vector<vector<bool>>>x_fixed;
        vector<bool>LeagueFree;

        std::unique_ptr<Randomizer<double>>DisFreeTeams;
        std::unique_ptr<Randomizer<double>>DisFreeHAPs;

        int MaxNrRandomIterations = 20; // maximum number of iterations where we sample uniformly from the neighborhoods before doing multi armed bandit
        double obj_prev_pert = INT_MAX; // objective before the previous perturb
        // If nothing happened: increase computation time and percentage of free variables
        // TODO


    public:
        FO(Input& in, std::unique_ptr<MetaBase<FO_move>> strategy);
        ~FO();

        double TimeLimitPerturb = 60;
        double PercFreeVariables = 0.50;
        double PercFreeTeams = 0.15;
        unordered_map<FO_move, double>PercentageHapsFixed = {{FO_move::R1, 0.0}, {FO_move::R2C, 0.0}, {FO_move::R3C, 0.05}, 
            {FO_move::R2NC, 0.0}, {FO_move::R3NC, 0.0}, {FO_move::T, 0.05}}; // Number of Haps we fix based on the number of rounds
        unordered_map<FO_move, double>TimeLimit = {{FO_move::R1, 10.0}, {FO_move::R2C, 10.0}, {FO_move::R3C, 10.0}, 
            {FO_move::R2NC, 10.0}, {FO_move::R3NC, 10.0}, {FO_move::T, 10.0}}; // in seconds!!

        unordered_map<FO_move, int>NrLeaguesFree = {{FO_move::R1, 1}, {FO_move::R2C, 1}, {FO_move::R3C, 1}, 
            {FO_move::R2NC, 1}, {FO_move::R3NC, 1}, {FO_move::T, 1}};
        
        void InitializeModel(Solution& sol, const InputData& data);
        void FreeLeagues();
        void FreeRounds(const int nr, const bool consecutive);
        void FreeTeams();
        void fix_all();
        void unfix_all();
        void FixVariables();
        void SetDualProbs();
        void UpdateSizeFixedVariables(const FO_move move, const bool optimal);
        void UpdateSizeFreeLeagues(const FO_move move, const bool optimal);
        void FreeVariables();
        void FreeVariablesPerturb();
        void Validate();
        void Set_x_value_from_sol(Solution& sol);
        void Store_x_value();

        void DoMove() override;
        void solve(Input& in, Solution& sol, const InputData& data);
        int Perturb(Solution& sol);
};

#endif
