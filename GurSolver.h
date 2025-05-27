#ifndef GURSOLVER_H  
#define GURSOLVER_H

#include "Input.h"
#include "Solution.h"
#include "gurobi_c++.h"
#include <cmath>
#include <chrono>
#include <random>
#include <array>

// R1: 1 round free, R2: 2 rounds free, R3: 3 rounds free, T: free subset of teams
enum class nb_name{R1, R2C, R3C, R2NC, R3NC, T}; 
const unordered_map<nb_name, string>nb_name_string = {{nb_name::R1, "R1"}, {nb_name::R2C, "R2C"}, {nb_name::R3C, "R3C"}, 
    {nb_name::R2NC, "R2NC"}, {nb_name::R3NC, "R3NC"}, {nb_name::T, "T"}}; // C: consecutive, NC: non consecutive

class GurSolver : public Input
{
    private:

        // const GRBEnv env;
	    // GRBModel model = GRBModel(env);
        GRBEnv env;
        GRBModel model;
        GRBLinExpr Objective;
        vector<vector<vector<GRBVar>>> x;
        vector<vector<GRBVar>>v;
        vector<vector<GRBVar>>b;
        vector<vector<GRBVar>>y;
        vector<vector<HA>>Orientation;
        vector<vector<vector<bool>>>x_value;
        vector<vector<vector<bool>>>x_fixed;

        vector<vector<vector<GRBConstr>>>Constraints_fixed_variables; // per team per round

        GRBModel createModel(GRBEnv& env); // this way, we can set GRB_IntParam_OutputFlag to 0 before building the model

        // Fix and Optimize:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point current_time;
        int TimeLimitSubIp = 30.0;
        int TimeLimitFO = 600;
        int Consecutive_w = 0.3;

        double PercFreeTeams = 0.05;
        unordered_map<nb_name, double>CumulWeights = {{nb_name::R1, 0.35}, {nb_name::R2C, 0.45}, {nb_name::R3C, 0.50}, 
            {nb_name::R2NC, 0.60}, {nb_name::R3NC, 0.65}, {nb_name::T, 1.0}};

        unordered_map<nb_name, double>Weights = {{nb_name::R1, 0.35}, {nb_name::R2C, 0.10}, {nb_name::R3C, 0.05}, 
            {nb_name::R2NC, 0.10}, {nb_name::R3NC, 0.05}, {nb_name::T, 0.35}}; // Whether we do something related to rounds or teams

        unordered_map<nb_name, double>PercentageHapsFixed = {{nb_name::R1, 0.0}, {nb_name::R2C, 0.25}, {nb_name::R3C, 0.50}, 
            {nb_name::R2NC, 0.25}, {nb_name::R3NC, 0.50}, {nb_name::T, 0.10}}; // Number of Haps we fix based on the number of rounds

        array<nb_name, 6>Moves = {nb_name::R1, nb_name::R2C, nb_name::R3C, nb_name::R2NC, nb_name::R3NC, nb_name::T};

        unordered_map<nb_name, double>Reward = {{nb_name::R1, 0.0}, {nb_name::R2C, 0.0}, {nb_name::R3C, 0.0}, {nb_name::R2NC, 0.0}, {nb_name::R3NC, 0.0}, {nb_name::T, 0.0}};
        unordered_map<nb_name, int>NrChosen = {{nb_name::R1, 0}, {nb_name::R2C, 0}, {nb_name::R3C, 0}, {nb_name::R2NC, 0}, {nb_name::R3NC, 0}, {nb_name::T, 0}};

        double MinWeight = 0.01;
        double lambda = 0.093;

    public:

        GurSolver(const Input& in);
        ~GurSolver();

        vector<bool>HapFixed; // whether the hap of team i is fixed

        void build_league(const bool HA, const bool relax_x);
        int build_all(const bool HA, const bool relax_x);
        void AddObj(const bool min_travel, const bool min_capacity_violations);
        void setTimeLimit(const int time_limit);
        void setBoundCapacityViolations();
        int solve();
        double getMipGap();
        int getBestBound();
        void SaveSolution(Solution& sol);
        void Store_x_value();
        int ComputeObjValue();
        void Validate();

        void BuildMiaoFormulation(const bool relax_x);
        void BuildPatternFormulation();
        void Fix_y_Patterns(const Solution& sol);
        void AssignHAPsToTeams(Solution& sol); // For Miao's algorithm
        void StoreHAPs(Solution& sol);

        // Fix and optimize:
        void FixHAP(Solution& sol);
        void FreeRounds(const int nr, const bool consecutive);
        void FreeTeams();
        void FO(Solution& sol);
        void fix_all();
        void unfix_all();
        void FixVariables();
        void SetDualProbs();
        void UpdateSizeFixedVariables(const nb_name move, const bool optimal);
        void UpdateSelectionProbabilities();
        void FreeVariables();

        vector<vector<pair<int,int>>> EdgeColoring(int& C, vector<vector<bool>>& ForbiddenEdge, int& NrColorsUsed);

        int PerfectMatching(vector<vector<bool>>& GameForbidden);
};

#endif
