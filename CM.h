#ifndef CM_H  
#define CM_H

#include "Input.h"
#include "GurSolver.h"
#include "Heuristic_CM.h"
#include "Algo.h"

#include <filesystem>

namespace fs = std::filesystem;

unordered_map<move_name_CM, double> WeightsMap(const unordered_map<string, double>& InputWeights){
    // Weights: see Heuristic_CM.h
    unordered_map<move_name_CM, double>Weights;
    for (const auto& [move, weight]: InputWeights){
        if (move == "TS" && weight > 0.0){
            Weights[move_name_CM::TS] = weight;
        }
        else if (move == "PTS" && weight > 0.0){
            Weights[move_name_CM::PTS] = weight;
        }
        else if (move == "RS" && weight > 0.0){
            Weights[move_name_CM::RS] = weight;
        }
        else if (move == "PRS" && weight > 0.0){
            Weights[move_name_CM::PRS] = weight;
        }
        else if (move == "M" && weight > 0.0){
            Weights[move_name_CM::M] = weight;
        }
        else if (move == "BM" && weight > 0.0){
            Weights[move_name_CM::BM] = weight;
        }
        else if (move == "C" && weight > 0.0){
            Weights[move_name_CM::C] = weight;
        }
    }
    return Weights;
}

void SolveHeuristic(Input& in, const int seed, const bool MinCostNB, const int HistoryLength, const int TimeLimit, const int MaxIt, vector<int>& TimeStamps, const string Instance, const unordered_map<string, double>& InputWeights){
    // Find initial solution with Vizing
    Solution sol(in);
    cout << "Solve Vizing" << endl;
    VizingConstruction(sol, seed);
    assert(sol.validate());
    const int obj = sol.ComputeCostGeneralMatrix();
    cout << "Cost initial solution = " << obj << endl;
    std::mt19937 gen(seed);
    unordered_map<move_name_CM, double>Weights = WeightsMap(InputWeights);
    Heuristic_CM algo(Moves, Weights, gen, HistoryLength, obj, MinCostNB);
    algo.setTimeLimit_meta(TimeLimit);
    algo.SetMaxIt(MaxIt);
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
    // gur.SaveSolution(sol);
    // sol.validate();
    // cin.get();
    return;
}

void TestCostMinimization(const int seed, const string Instance, const bool Heuristic, const bool MinCostNB, const int HistoryLength, const int TimeLimit, const int MaxIt, const unordered_map<string, double>& InputWeights){

    vector<int>TimeStamps;
    int TimeStamp = 30;
    int Incrementor = 30;
    while (TimeStamp <= TimeLimit){
        TimeStamps.push_back(TimeStamp);
        TimeStamp += Incrementor;
    }

    const string FilePath = "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel" + std::string(PATHSEP) + "0_100" + std::string(PATHSEP) + Instance + ".txt";

    Input in;
    if (!in.read_CostMinimization(FilePath, InstanceSetCM::Karel)){
        cout << "Could not read " << FilePath << endl;
        return;
    }
    in.setHAP_requirements(false, false, false, true, in.getNrRounds());
    in.SRR = true;
    
    if (Heuristic){
        SolveHeuristic(in, seed, MinCostNB, HistoryLength, TimeLimit, MaxIt, TimeStamps, Instance, InputWeights);
    }
    else{
        SolveIP(in,seed,TimeLimit,TimeStamps, Instance);
    }
}

#endif