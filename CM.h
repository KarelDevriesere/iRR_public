#ifndef CM_H  
#define CM_H

#include "Input.h"
#include "GurSolver.h"
#include "Heuristic_CM.h"
#include "Algo.h"

#include <filesystem>

namespace fs = std::filesystem;

void SolveHeuristic(Input& in, const int seed, const bool MinCostNB, const int HistoryLength, const int TimeLimit, vector<int>& TimeStamps, const string Instance){
    // Find initial solution with Vizing
    Solution sol(in);
    cout << "Solve Vizing" << endl;
    VizingConstruction(sol, seed);
    assert(sol.validate());
    const int obj = sol.ComputeCostGeneralMatrix();
    cout << "Cost initial solution = " << obj << endl;
    std::mt19937 gen(seed);
    Heuristic_CM algo(Moves, Weights, gen, HistoryLength, obj, MinCostNB);
    algo.setTimeLimit_meta(TimeLimit);
    algo.SetTimeStamps(TimeStamps);
    algo.solve(in, sol);
    string FilePath = "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel" + std::string(PATHSEP) + "0_100" + std::string(PATHSEP) + "Results" + std::string(PATHSEP) + "Heuristic" + std::string(PATHSEP);
    if (MinCostNB){
        FilePath += "MinCost";
    }
    else{
        FilePath += "NoMinCost";
    }
    FilePath += std::string(PATHSEP) + Instance + "_" + "s" + to_string(seed) + "_HL" + to_string(HistoryLength) + ".txt";
    const string config = to_string(seed) + ",Heuristic," + to_string(MinCostNB) + "," + to_string(HistoryLength);
    algo.SaveSolutionsTimeStamps(FilePath, config);
    assert(sol.validate());
    cout << "Final solution = " << sol.ComputeCostGeneralMatrix() << endl;
    cin.get();
    return;
}

void SolveIP(Input& in, const int seed, const int TimeLimit, vector<int>& TimeStamps, const string Instance){
    GurSolver gur(in);
    Solution sol(in);
    bool HA = true;
    bool relax_x = false;
    gur.build_base(HA, relax_x);
    // gur.build_HAP_constraints();
    gur.AddObjGeneralCosts();
    // gur.AddObjMinBreaks();
    gur.setTimeLimit(TimeLimit);
    gur.SetTimeStamps(TimeStamps);
    gur.solve();
    const string FilePath = "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel" + std::string(PATHSEP) + "0_100" + std::string(PATHSEP) + "Results" + std::string(PATHSEP) + "IP" + std::string(PATHSEP) + Instance + ".txt";
    const string config = to_string(seed) + ",IP";
    gur.SaveSolutionsTimeStamps(FilePath, config);
    cin.get();
    // gur.SaveSolution(sol);
    // sol.validate();
    // cin.get();
    return;
}

void TestCostMinimization(const int seed, const string Instance, const bool Heuristic, const bool MinCostNB, const int HistoryLength){

    vector<int>TimeStamps = {30, 60, 120, 300, 600, 1800, 3600, 7200, 14400, 28800};
    int TimeLimit = 65;

    const string FilePath = "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel" + std::string(PATHSEP) + "0_100" + std::string(PATHSEP) + Instance + ".txt";

    Input in;
    if (!in.read_CostMinimization(FilePath, InstanceSetCM::Karel)){
        cout << "Could not read " << FilePath << endl;
        return;
    }
    in.setHAP_requirements(false, false, false, true, in.getNrRounds());
    in.SRR = true;
    
    if (Heuristic){
        SolveHeuristic(in, seed, MinCostNB, HistoryLength, TimeLimit, TimeStamps, Instance);
    }
    else{
        SolveIP(in,seed,TimeLimit,TimeStamps, Instance);
    }
}

#endif