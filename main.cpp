#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <assert.h>
#include <chrono>
#include <sstream>
#include <filesystem>

#include "Input.h"
#include "AlgoSelection.h"

void setWeight(const unordered_map<Move,string>& Moves, unordered_map<Move, double>& InputWeights, string MoveName, double Weight){
    bool NameFound = false;
    for (auto& [move, name]: Moves){
        if (name == MoveName){
            NameFound = true;
            InputWeights[move] = Weight;
            if (InputWeights.at(move) < 0.0){
                std::cerr << MoveName << " should be positive!" << endl;
                std::abort();
            }
        }
    }
}

void ModifyWeights(const unordered_map<Move,string>& Moves, unordered_map<Move, double>& InputWeights){
    unordered_map<Move, double>InputWeightsCopy = InputWeights;
    double sum = 0.0;
    double sum_weights = 0.0;
    bool NoMoveSeen = true;
    for (const auto& [move, weight]: InputWeights){
        sum += InputWeights.at(move);
        NoMoveSeen = false;
    }
    for (const auto& [move, weight]: InputWeights){
        InputWeights.at(move) /= sum;
        sum_weights += InputWeights.at(move);
#ifdef PRINT
#if PRINT == 1
        cout << Moves.at(move) << ": " << InputWeights.at(move) << endl;
#endif 
#endif
    }
    if (NoMoveSeen){
        assert(sum == 0);
        for (const auto& [move, name]: Moves){
            sum += 1.0;
        }
        for (const auto& [move, name]: Moves){
            InputWeights[move] = 1.0 / sum;
            sum_weights += InputWeights.at(move);
#ifdef PRINT
#if PRINT == 1
            cout << Moves.at(move) << ": " << InputWeights.at(move) << endl;
#endif 
#endif
        }
    }
    if (sum_weights <= 0.99 || sum_weights >= 1.01){
        cout << "Sum of the weights should be equal to 1.0 but is now" << sum_weights << endl;
        std::abort();
    }
}

int main(int argc, const char* argv[]){

    InputData data; // Concerned with the input instance
    ParameterValues param; // Concerned with parameter values for metaheuristics

    data.Instance = "Instances/TTP/NL16_4.xml";

    int ComputeBounds = 0;
    bool teamSwapper = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--Seed"){
            data.seed = std::stoi(argv[++i]);
        } else if (arg == "--TeamSwapper"){
        teamSwapper = std::stoi(argv[++i]);
        } else if (arg == "--StartSol"){
        data.startSol = argv[++i];
    }
        else if (arg == "--Heuristic"){
            data.Heuristic = std::stoi(argv[++i]);
            if (data.Heuristic != 0 && data.Heuristic != 1){
                std::cerr << "Heuristic should be 0 or 1" << endl;
                return 1;
            }
            data.RunGM = 0;
            data.FO = 0;
        }
        else if (arg == "--InstanceTTP"){ // TTP
            data.Instance = argv[++i];
            //auto it = std::find(InstancesTTP.begin(), InstancesTTP.end(), data.Instance);
            //if (it == InstancesTTP.end()){
            //    std::cerr << "Incorrect TTP instance name, name given = " << data.Instance << endl;
            //    return 1;
            //}
            data.TTP = true;
            data.Football = false;
            data.Hockey = false;
        }
        else if (arg == "--InstanceFootball"){ // Instances from paper Li et al. (YSTP, football)
            int i_ = std::stoi(argv[++i]);
            data.Football = true;
            data.TTP = false;
            data.Hockey = false;
            if (i_ <= 0 || i_ > 6){
                std::cerr << "Football instance must be 1,2,3,4,5 or 6!!" << endl;
                return 1;
            }
            else{
                data.Instance = "i0" + to_string(i_);
            }
        }
        else if (arg == "--CapacitySetting"){ // YSTP, football
            int setting = std::stoi(argv[++i]);
            if (setting == 0){
                data.ConstantCapacity = true;
            }
            else if (setting == 1 || setting == 2){
                data.ConstantCapacity = false;
            }
            else{
                std::cerr << "Capacity setting must be 0 (constant), 1 or 2!" << endl;
                return 1;
            }
            data.CapacitySetting = setting;
            data.Football = true;
            data.TTP = false;
            data.Hockey = false;
        }
        else if (arg == "--MaxNrBreaks"){ // YSTP, football
            data.MaxNrBreaks = std::stoi(argv[++i]);
            data.Football = true;
            data.TTP = false;
            data.Hockey = false;
            /*
            if (data.MaxNrBreaks != 0 && data.MaxNrBreaks != 1 && data.MaxNrBreaks != 2 && data.MaxNrBreaks != 3){
                std::cerr << "MaxNrBreaks must be 0,1,2 or 3!!" << endl;
                return 1;
            }
                */
        }
        else if (arg == "--GM"){ // YSTP, football
            data.RunGM = std::stoi(argv[++i]);
            data.Heuristic = 0;
            data.FO = 0;
        }
        else if (arg == "--Constructive"){
            data.GM_Constructive = std::stoi(argv[++i]);
        }
        else if (arg == "--FO"){ // YSTP, football, hockey
            data.FO = std::stoi(argv[++i]);
            data.Heuristic = 0;
            data.RunGM = 0;
        }
        else if (arg == "--InstanceHockey"){
            int hockey_i = std::stoi(argv[++i]);
            data.Football = false;
            data.TTP = false;
            data.Hockey = true;
            if (hockey_i < 0 || hockey_i > 6){
                std::cerr << "Hockey instance must be 1,2,3,4,5 or 6!!" << endl;
                return 1;
            }
            else{
                data.Instance = "i0" + to_string(hockey_i);
            }
        }
        else if (arg == "--Weight"){ // Selection probability of each neighborhood
            string MoveName = argv[++i];
            double Weight = std::stod(argv[++i]);
            setWeight(data.Moves, data.InputWeights, MoveName, Weight);
        }
        else if (arg == "--WeightPerturb"){ // Selection probability of each neighborhood when perturbing (ILS)
            string MoveName = argv[++i];
            double Weight = std::stod(argv[++i]);
            setWeight(data.Moves, data.InputWeightsPerturb, MoveName, Weight);
        }
        else if (arg == "--TimeLimit"){ // Generic parameters
            param.TIME_LIMIT = std::stoi(argv[++i]);
            if (param.TIME_LIMIT < 0){
                std::cerr << "TimeLimit should be positive" << endl;
                return 1;
            }
        }
        else if (arg == "--MaxIt"){ // Generic parameters
            param.MAX_IT = std::stoi(argv[++i]);
            if (param.MAX_IT < 0){
                std::cerr << "Max no iterations should be positive" << endl;
                return 1;
            }
        }
        else if (arg == "--HC"){
            if (!std::stoi(argv[++i])){
                continue;
            }
            param.HC = true;
        }
        else if (arg == "--SA"){
            if (!std::stoi(argv[++i])){
                continue;
            }
            param.SA = true;
        }
        else if (arg == "--T_begin"){ // SA
            param.T_begin = std::stod(argv[++i]);
            if (param.T_begin < 0){
                std::cerr << "T_begin should be positive" << endl;
                return 1;
            }
            param.SA = true;
        }
        else if (arg == "--T_end"){ // SA
            param.T_end = std::stod(argv[++i]);
            if (param.T_end < 0){
                std::cerr << "T_end should be positive" << endl;
                return 1;
            }
            param.SA = true;
        }
        else if (arg == "--CoolingRate"){ // SA
            param.cooling_rate = std::stod(argv[++i]);
            if (param.cooling_rate < 0){
                std::cerr << "CoolingRate should be positive" << endl;
                return 1;
            }
            param.SA = true;
        }
        else if (arg == "--I_temp"){ // SA
            param.I_temp = std::stoi(argv[++i]);
            if (param.I_temp < 0){
                std::cerr << "I_temp should be positive" << endl;
                return 1;
            }
            param.SA = true;
        }
        else if (arg == "--I_accept"){ // SA
            param.I_accept = std::stoi(argv[++i]);
            if (param.I_accept < 0){
                std::cerr << "I_accept should be positive" << endl;
                return 1;
            }
            param.SA = true;
        }
        else if (arg == "--Reheat"){ // SA
            if (std::stoi(argv[++i]) > 0){
                param.IncludeReheating = true;
            }
        }
        else if (arg == "--ILS"){ // Iterated Local Search
            if (!std::stoi(argv[++i])){
                continue;
            }
            param.ILS = true;
        }
        else if (arg == "--ItMaxPert"){ // ILS
            param.IT_MAX_PERT = std::stoi(argv[++i]);
            if (param.IT_MAX_PERT < 0){
                std::cerr << "ItMaxPert should be positive" << endl;
                return 1;
            }
        }
        else if (arg == "--LAHC"){ // Iterated Local Search
            if (!std::stoi(argv[++i])){
                continue;
            }
            param.LAHC = true;
        }
        else if (arg == "--HistoryLength"){ // if not provided: history length is dynamic!!
            param.HistoryLength = std::stoi(argv[++i]); 
            if (param.HistoryLength <= 0){
                std::cerr << "HistoryLength should be strictly positive" << endl;
                return 1;
            }
            param.HistoryLengthProvided = true;
            param.LAHC = true;
        }
        else if (arg == "--PerturbeIncrease"){ // default is 100
            param.PerturbeIncrease = std::stod(argv[++i]);
            if (param.PerturbeIncrease <= 0){
                std::cerr << "PerturbeIncrease should be strictly positive" << endl;
                return 1;
            }
            param.LAHC = true;
        }
        else if (arg == "--HistoryMultiplier"){ // default is 100
            param.HistoryMultiplier = std::stod(argv[++i]);
            if (param.HistoryMultiplier <= 0){
                std::cerr << "HistoryMultiplier should be strictly positive" << endl;
                return 1;
            }
            param.LAHC = true;
        }
        else if (arg == "--PerturbeValueInitial"){ // default is 100
            param.PerturbeValue_INITIAL = std::stod(argv[++i]);
            if (param.PerturbeValue_INITIAL < 0){
                std::cerr << "PerturbeValueInitial should be positive" << endl;
                return 1;
            }
            param.LAHC = true;
        }
        else if (arg == "--VNS"){ // Variable Neighborhood Search
            if (!std::stoi(argv[++i])){
                continue;
            }
            param.VNS = true;
        }
        else if (arg == "--MAB"){
            if (!std::stoi(argv[++i])){
                continue;
            }
            param.MAB = true;
        }
        else if (arg == "--ConstrViolationAllowed"){
            if (std::stol(argv[++i]) > 0){
                data.ConstraintViolationAllowed = true;
            }
        }
        else if (arg == "--ConstrViolationCost"){ // Not used in paper
            // 1 cost for violating all types of hard constraints
            data.ConstrViolationCost = std::stol(argv[++i]);
            if (data.ConstrViolationCost < 0){
                std::cerr << "ConstrViolationCost should be positive!" << endl;
                return 1;
            }
            param.LAHC = true;
        }
        else if (arg == "--Bounds"){ // TTP
            ComputeBounds = std::stoi(argv[++i]);
            data.TTP = true;
        }
        else if (arg == "--DLB"){ // iTTP
            data.DLB = std::stoi(argv[++i]);
        }
        else if (arg == "--addMinTripConstraint"){ // TTP
            data.addMinTripConstraint = std::stoi(argv[++i]);
        }
        else if (arg == "--addColoringConstraint"){ // TTP
            data.addColoringConstraint = std::stoi(argv[++i]);
        }
        else if (arg == "--TripModel"){ // TTP
            data.SolveTripModel = std::stoi(argv[++i]);
        }
        else if (arg == "--TripModelHAPFixed"){ // TTP
            data.TripModelHAP_Fixed = std::stoi(argv[++i]);
        }
        else if (arg == "--OutputFolder"){
            data.OutputFolder = argv[++i];
            if (!std::filesystem::is_directory(data.OutputFolder)){
                std::cerr << "The folder " << data.OutputFolder << " does not exist, please specify existing folder (or make one first)" << endl;
                return 1;
            }

        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1; // Exit with an error code
        }
    }

    int count_meta = 0;
    if (param.HC){
        ++count_meta;
    }
    if (param.SA){
        ++count_meta;
    }
    if (param.ILS){
        ++count_meta;
    }
    if (param.LAHC){
        ++count_meta;
    }
    if (count_meta > 1){
        cout << "Choose only one metaheuristic!!" << endl;
        std::abort();
    }

    if (ComputeBounds == 1){
        cout << "Compute TTP bound for " << data.Instance << "_" << data.NrRounds << " with TimeLimit " << param.TIME_LIMIT << endl;
        BoundsTTP_OneInstance(data, param);
        return 1;
    }

    if (data.Heuristic){
#ifdef PRINT
#if PRINT == 1
    cout << "------ Weights ------" << endl;
#endif
#endif
    ModifyWeights(data.Moves, data.InputWeights);
    ModifyWeights(data.Moves, data.InputWeightsPerturb);
#ifdef PRINT
#if PRINT == 1
    cout << "---------------------" << endl;
#endif
#endif
    }
#ifdef PRINT
#if PRINT == 1
    cout << "---------------------" << endl;
    cout << "Instance = " << data.Instance << endl;
    cout << "Parameters specified:" << endl;
    cout << "TimeLimit = " << param.TIME_LIMIT << endl;
    cout << "Max iterations = " << param.MAX_IT << endl;
    cout << "HistoryLength = " << param.HistoryLength << endl;
#endif
#endif
    SelectAlgo(data, param);

    return 1;
}
