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
#include "CM.h"
#include "InstanceGenerator.h"

int main(int argc, const char* argv[]){
    int case_ = 0;

    InputData data;

    if (case_ == 0){

        data.Instance = "Instances/TTP/NF16_4.xml";

        bool MinCostSpecified = false;
        bool M_chosen = false;
        bool BM_chosen = false;
        bool iPTS_chosen = false;
        double M_weight = 0.0;
        double BM_weight = 0.0;
        double iPTS_weight = 0.0;

        int ComputeBounds = 0;

        // Parse command-line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--Seed"){
                data.seed = std::stoi(argv[++i]);
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
                data.PerturbeIncrease = std::stoi(argv[++i]);
                if (data.PerturbeIncrease <= 0){
                    std::cerr << "PerturbeIncrease should be strictly positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--TTP"){
                data.TTP = std::stoi(argv[++i]);
                if (data.TTP != 0 && data.TTP != 1){
                    std::cerr << "TTP should be 0 or 1" << endl;
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
                data.Miao = false;
                data.Hockey = false;
            }
            else if (arg == "--Miao"){ // Miao
                data.Miao = std::stoi(argv[++i]);
                if (data.TTP != 0 && data.TTP != 1){
                    std::cerr << "TTP should be 0 or 1" << endl;
                    return 1;
                }
            }
            else if (arg == "--CapacitySetting"){ // Miao
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
                data.Miao = true;
                data.TTP = false;
                data.Hockey = false;
            }
            else if (arg == "--MaxNrBreaks"){ // Miao
                data.MaxNrBreaks = std::stoi(argv[++i]);
                data.Miao = true;
                data.TTP = false;
                data.Hockey = false;
                /*
                if (data.MaxNrBreaks != 0 && data.MaxNrBreaks != 1 && data.MaxNrBreaks != 2 && data.MaxNrBreaks != 3){
                    std::cerr << "MaxNrBreaks must be 0,1,2 or 3!!" << endl;
                    return 1;
                }
                    */
            }
            else if (arg == "--InstanceMiao"){ // Miao
                int miao_i = std::stoi(argv[++i]);
                data.Miao = true;
                data.TTP = false;
                data.Hockey = false;
                if (miao_i < 0 || miao_i > 6){
                    std::cerr << "MiaoInstance must be 1,2,3,4,5 or 6!!" << endl;
                    return 1;
                }
                else{
                    data.Instance = "i0" + to_string(miao_i);
                }
            }
            else if (arg == "--MiaoAlgo"){ // Miao
                data.RunMiaoAlgo = std::stoi(argv[++i]);
            }
            else if (arg == "--PercentageHAPs"){ // TTP
                data.PercentageHAPs = std::stoi(argv[++i]);
                if (data.PercentageHAPs < 0 || data.PercentageHAPs > 100){
                    std::cerr << "PercentageHAPs should be between 0 and 100" << endl;
                    return 1;
                }
            }
            else if (arg == "--InstanceHockey"){
                int hockey_i = std::stoi(argv[++i]);
                data.Miao = false;
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
                if (!NameFound){
                    if (MoveName == "M"){
                        M_chosen = true;
                        M_weight = Weight;
                    }
                    else if (MoveName == "BM"){
                        BM_chosen = true;
                        BM_weight = Weight;
                    }
                    else if (MoveName == "iPTS"){
                        iPTS_chosen = true;
                        iPTS_weight = Weight;
                    }
                    else{
                        cout << "Could not find " << MoveName << ", please choose one of the following moves: " << endl;
                        for (auto& [move, name]: data.Moves){
                            cout << name << endl;
                        }
                        cout << "BM" << endl;
                        cout << "M" << endl;
                        return 1;
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
            else if (arg == "--MinCost"){
                data.MinCost = std::stoi(argv[++i]);
                MinCostSpecified = true;
            }
            else if (arg == "--Base"){
                data.Base = std::stoi(argv[++i]);
            }
            else if (arg == "--Bounds"){
                ComputeBounds = std::stoi(argv[++i]);
                data.TTP = true;
            }
            else if (arg == "--OutputFolder"){
                data.OutputFolder = argv[++i];
                if (!std::filesystem::is_directory(data.OutputFolder)){
                    std::cerr << "The folder " << data.OutputFolder << " does not exist, please specify existing folder (or make one first)" << endl;
                    return 1;
                }

            }
            else if (arg == "--help"){
                cout << "Usage: " << argv[0] << "--Seed <int> --Heuristic <0/1> -- MinCostNB <0/1> --HistoryLength <+int> --TimeLimit <+int> --MaxIt <+int> --Weight <Move> <+int>" << endl;
                return 1;
            }
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return 1; // Exit with an error code
            }
        }

        if (ComputeBounds == 1){
            cout << "Compute TTP bound for " << data.Instance << "_" << data.NrRounds << " with TimeLimit " << data.TimeLimit << endl;
            BoundsTTP_OneInstance(data);
            return 1;
        }
        else if (ComputeBounds > 1){
            cout << "Compute TTP bound for all instances with TimeLimit " << data.TimeLimit << endl;
            BoundsTTP_All(data);
            return 1;
        }

        if (data.Base){
            cout << "Run base algorithm!!" << endl;
            cout << "Note: if all pairs of rounds are Hamiltonian cycles: never non-Hamiltonian cycles with base algo.." << endl;
        }
        // First, check if BM or M or PTS is chosen
        if (iPTS_chosen && !data.Base){
            if (MinCostSpecified && !data.MinCost){
                data.InputWeights[Move::iPTS_Random_PR] = iPTS_weight;
                data.InputWeights.erase(Move::iPTS_MinCost_PR);
            }
            else if (MinCostSpecified && data.MinCost){
                data.InputWeights[Move::iPTS_MinCost_PR] = iPTS_weight;
                data.InputWeights.erase(Move::iPTS_Random_PR);
            }
            else {
                cout << "distribute weight iPTS over " << data.Moves.at(Move::iPTS_MinCost_PR) << " and " << data.Moves.at(Move::iPTS_Random_PR) << endl;
                data.InputWeights[Move::iPTS_Random_PR] = iPTS_weight/2.0;
                data.InputWeights[Move::iPTS_MinCost_PR] = iPTS_weight/2.0;
            }
        }
        if (BM_chosen && !data.Base){
            if (MinCostSpecified && !data.MinCost){
                data.InputWeights[Move::Random_BM] = BM_weight;
                data.InputWeights.erase(Move::MinCost_BM);
            }
            else if (MinCostSpecified && data.MinCost){
                data.InputWeights[Move::MinCost_BM] = BM_weight;
                data.InputWeights.erase(Move::Random_BM);
            }
            else {
                cout << "distribute weight BM over " << data.Moves.at(Move::MinCost_BM) << " and " << data.Moves.at(Move::Random_BM) << endl;
                data.InputWeights[Move::Random_BM] = BM_weight/2.0;
                data.InputWeights[Move::MinCost_BM] = BM_weight/2.0;
            } 
        }
        if (M_chosen && !data.Base){
            if (MinCostSpecified && !data.MinCost){
                data.InputWeights[Move::Random_M_Random_PR] = M_weight;
                data.InputWeights.erase(Move::MinCost_M_MinCost_PR);
                data.InputWeights.erase(Move::MinCost_M_Random_PR);
                data.InputWeights.erase(Move::Random_M_MinCost_PR);
            }
            else if (MinCostSpecified && data.MinCost){
                data.InputWeights[Move::MinCost_M_MinCost_PR] = M_weight;
                data.InputWeights.erase(Move::Random_M_Random_PR);
                data.InputWeights.erase(Move::MinCost_M_Random_PR);
                data.InputWeights.erase(Move::Random_M_Random_PR);
            }
            else {
                cout << "distribute weight M over " << data.Moves.at(Move::MinCost_M_MinCost_PR) << " and " << data.Moves.at(Move::Random_M_Random_PR) << " and " << data.Moves.at(Move::Random_M_MinCost_PR) << " and " << data.Moves.at(Move::MinCost_M_Random_PR) << endl;
                data.InputWeights[Move::MinCost_M_MinCost_PR] = M_weight/4.0;
                data.InputWeights[Move::Random_M_MinCost_PR] = M_weight/4.0;
                data.InputWeights[Move::MinCost_M_Random_PR] = M_weight/4.0;
                data.InputWeights[Move::Random_M_Random_PR] = M_weight/4.0;
            }
        }   
    
        unordered_map<Move, double>InputWeightsCopy = data.InputWeights;

        double sum = 0.0;
        double sum_weights = 0.0;
        cout << "------ Weights ------" << endl;
        InputWeightsCopy = data.InputWeights;
        for (const auto& [move, weight]: InputWeightsCopy){
            if (data.Base && !data.IsMoveInBase.at(move)){
                data.InputWeights.erase(move);
                cout << "Base specified but " << data.Moves.at(move) << " not part of base so set this weight to 0!!" << endl;
            }
        }
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
                if (data.Base && !data.IsMoveInBase.at(move)){
                    continue;
                }
                else if (MinCostSpecified && data.MinCost && name.find("Random") != std::string::npos){
                    continue;
                }
                else if (MinCostSpecified && !data.MinCost && name.find("MinCost") != std::string::npos){
                    continue;
                }
                sum += 1.0;
            }
            for (const auto& [move, name]: data.Moves){
                if (data.Base && !data.IsMoveInBase.at(move)){
                    continue;
                }
                else if (MinCostSpecified && data.MinCost && name.find("Random") != std::string::npos){
                    continue;
                }
                else if (MinCostSpecified && !data.MinCost && name.find("MinCost") != std::string::npos){
                    continue;
                }
                else{
                    data.InputWeights[move] = 1.0 / sum;
                }
                sum_weights += data.InputWeights.at(move);
                cout << data.Moves.at(move) << ": " << data.InputWeights.at(move) << endl;
            }
        }
        if (sum_weights <= 0.99 || sum_weights >= 1.01){
            cout << "Sum of the weights should be equal to 1.0 but is now" << sum_weights << endl;
            return 1;
        }
        cout << "---------------------" << endl;

        cout << "Parameters" << endl;
        cout << "TimeLimit = " << data.TimeLimit << endl;
        cout << "Max iterations = " << data.MaxIt << endl;
        cout << "HistoryLength = " << data.HistoryLength << endl;
        TestCostMinimization(data);
        // GenerateCostMatrices(0);
        // cin.get();
    }
    else{
        // TODO: paper about algorithms for Hockey and Miao instances

    }
    return 1;
}
