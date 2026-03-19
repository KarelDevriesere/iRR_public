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

int main(int argc, const char* argv[]){

    InputData data;

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
        }
        else if (arg == "--HistoryLength"){ // if not provided: history length is dynamic!!
            data.HistoryLength = std::stoi(argv[++i]); 
            if (data.HistoryLength <= 0){
                std::cerr << "HistoryLength should be strictly positive" << endl;
                return 1;
            }
            data.HistoryLengthProvided = true;
        }
        else if (arg == "PerturbeIncrease"){ // default is 100
            data.PerturbeIncrease = std::stod(argv[++i]);
            if (data.PerturbeIncrease <= 0){
                std::cerr << "PerturbeIncrease should be strictly positive" << endl;
                return 1;
            }
        }
        else if (arg == "--InstanceTTP"){ // TTP
            data.Instance = argv[++i];
            //auto it = std::find(InstancesTTP.begin(), InstancesTTP.end(), data.Instance);
            //if (it == InstancesTTP.end()){
            //    std::cerr << "Incorrect TTP instance name, name given = " << data.Instance << endl;
            //    return 1;
            //}
            data.TTP = true;
    std::cout << "Set data TTP true" << std::endl;
            data.Football = false;
            data.Hockey = false;
        }
        else if (arg == "--InstanceFootball"){ // Instances from paper Li et al. (YSTP, football)
            int i_ = std::stoi(argv[++i]);
            data.Football = true;
            data.TTP = false;
            data.Hockey = false;
            if (i_ < 0 || i_ > 6){
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
        else if (arg == "--TimeLimit"){
            data.TimeLimit = std::stoi(argv[++i]);
            if (data.TimeLimit < 0){
                std::cerr << "TimeLimit should be positive" << endl;
                return 1;
            }
        }
        else if (arg == "--MaxIt"){
            data.MaxIt = std::stoi(argv[++i]);
            if (data.MaxIt < 0){
                std::cerr << "Max no iterations should be positive" << endl;
                return 1;
            }
        }
        else if (arg == "--Weight"){
            string MoveName = argv[++i];
            double Weight = std::stod(argv[++i]);
            bool NameFound = false;
            for (auto& [move, name]: data.Moves){
                if (name == MoveName){
                    NameFound = true;
                    data.InputWeights[move] = Weight;
                    if (data.InputWeights.at(move) < 0.0){
                        std::cerr << MoveName << " should be positive!" << endl;
                        return 1;
                    }
                }
            }
        }
        else if (arg == "--ConstrViolationCost"){
            // 1 cost for violating all types of hard constraints
            data.ConstrViolationCost = std::stol(argv[++i]);
            if (data.ConstrViolationCost < 0){
                std::cerr << "ConstrViolationCost should be positive!" << endl;
                return 1;
            }

        }
        else if (arg == "--Bounds"){ // TTP
            ComputeBounds = std::stoi(argv[++i]);
            data.TTP = true;
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

if(teamSwapper == 1){
    if(data.startSol.empty()){
        std::cout << "Team swapper requires to set --startSol" << std::endl;
    }
    std::cout << "Perform the team swapper for " << data.Instance << " and solution " << data.startSol << std::endl;

    // Read the instance
    std::cout << "Read the instance" << std::endl;
    Input in;
    if (data.TTP && !in.read_TTP(data.Instance)){
        cout << "could not read TTP path " << data.Instance << endl;
        return -1;
    }
    else if ((data.Football || data.Hockey) && !in.read_YSTP(data.Instance, data.Football)){
        cout << "could not read Football or Hockey path " << data.Instance << endl;
        return -1;
    }

    // Read the starting solution
    Solution sol(in);
        sol.SetOneCostAllViolations(data.ConstrViolationCost);
    ReadSolutionXML(data.startSol, sol);
        sol.validate();

    // Perform the team swapper
    //sol.SetOneCostAllViolations(data.ConstrViolationCost);
    int obj = sol.ComputeTotalCost();
    std::mt19937 gen(data.seed);
    int HL = 1;
    if (data.HistoryLengthProvided){
        HL = data.HistoryLength;
    }	
        Heuristic algo(data.Moves, data.InputWeights, gen, HL, obj, sol);
        algo.TeamSwapper();

    return 0;
}

    if (ComputeBounds == 1){
        cout << "Compute TTP bound for " << data.Instance << "_" << data.NrRounds << " with TimeLimit " << data.TimeLimit << endl;
        BoundsTTP_OneInstance(data);
        return 1;
    }

    unordered_map<Move, double>InputWeightsCopy = data.InputWeights;

    double sum = 0.0;
    double sum_weights = 0.0;
    cout << "------ Weights ------" << endl;
    InputWeightsCopy = data.InputWeights;
    bool NoMoveSeen = true;
    for (const auto& [move, weight]: data.InputWeights){
        sum += data.InputWeights.at(move);
        NoMoveSeen = false;
    }
    for (const auto& [move, weight]: data.InputWeights){
        data.InputWeights.at(move) /= sum;
        sum_weights += data.InputWeights.at(move);
        cout << data.Moves.at(move) << ": " << data.InputWeights.at(move) << endl;
    }
    if (NoMoveSeen){
        assert(sum == 0);
        for (const auto& [move, name]: data.Moves){
            sum += 1.0;
        }
        for (const auto& [move, name]: data.Moves){
            data.InputWeights[move] = 1.0 / sum;
            sum_weights += data.InputWeights.at(move);
            cout << data.Moves.at(move) << ": " << data.InputWeights.at(move) << endl;
        }
    }
    if (sum_weights <= 0.99 || sum_weights >= 1.01){
        cout << "Sum of the weights should be equal to 1.0 but is now" << sum_weights << endl;
        return 1;
    }
    cout << "---------------------" << endl;
    cout << "Instance = " << data.Instance << endl;
    cout << "Parameters specified:" << endl;
    cout << "TimeLimit = " << data.TimeLimit << endl;
    cout << "Max iterations = " << data.MaxIt << endl;
    cout << "HistoryLength = " << data.HistoryLength << endl;
    SelectAlgo(data);

    return 1;
}
