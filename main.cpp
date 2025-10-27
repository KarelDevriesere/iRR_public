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
    
    // BoundsTTP();
    // return 1;

    InputData data;

    if (case_ == 0){
        if (data.CM){
            data.Instance = to_string(data.NrTeams) + "_" + to_string(data.NrRounds) + "_" + "k" + to_string(data.k) + "_" + to_string(data.inst);
        }
        else {
            data.Instance = "N16.xml";
        }

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
            else if (arg == "--HistoryLength"){
                data.HistoryLength = std::stoi(argv[++i]); 
                if (data.HistoryLength <= 0){
                    std::cerr << "HistoryLength should be strictly positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--CM"){
                data.CM = std::stoi(argv[++i]);
                if (data.CM != 0 && data.CM != 1){
                    std::cerr << "CM should be 0 or 1" << endl;
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
            else if (arg == "--NrTeams"){ // CM
                data.NrTeams = std::stoi(argv[++i]);
                if (data.NrTeams != 36 && data.NrTeams != 100 && data.NrTeams != 250){
                    std::cerr << "NrTeams must be 36 or 100 or 250" << endl;
                    return 1;
                }
                if (data.NrTeams == 36){
                    data.NrRounds = 8;
                }
                else if (data.NrTeams == 100){
                    data.NrRounds = 16;
                }
                else{
                    data.NrRounds = 30;
                }
                data.CM = true;
                data.TTP = false;
            }
            else if (arg == "--k"){ // CM
                data.k = std::stoi(argv[++i]);
                if (data.k != 0 && data.k != 1 && data.k != 5 && data.k != 10){
                    std::cerr << "k must be 0, 1, 5 or 10!" << endl;
                    return 1;
                }
                data.CM = true;
                data.TTP = false;
            }
            else if (arg == "--i"){ // CM
                data.inst = std::stoi(argv[++i]);
                if (data.inst != 0 && data.inst != 1 && data.inst != 2 && data.inst != 3 && data.inst != 4){
                    std::cerr << "i must be 0,1,2,3 or 4" << endl;
                    return 1;
                }
                data.CM = true;
                data.TTP = false;
            }
            else if (arg == "--InstanceTTP"){ // TTP
                data.Instance = argv[++i];
                auto it = std::find(InstancesTTP.begin(), InstancesTTP.end(), data.Instance);
                if (it == InstancesTTP.end()){
                    std::cerr << "Incorrect TTP instance name, name given = " << data.Instance << endl;
                    return 1;
                }
                data.TTP = true;
                data.CM = false;
            }
            else if (arg == "--NrRounds"){ // TTP
                data.NrRounds = std::stoi(argv[++i]);
                if (data.NrRounds <= 0){
                    std::cerr << "NrRounds must be strictly positive!" << endl;
                    return 1;
                }
                if (data.NrRounds %2 != 0){
                    std::cerr << "NrRounds must be even!" << endl;
                    return 1;
                }
                data.TTP = true;
                data.CM = false;
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
                bool NameFound = false;
                for (auto& [move, name]: data.Moves){
                    if (name == MoveName){
                        NameFound = true;
                        data.InputWeights[move] = std::stod(argv[++i]);
                        if (data.InputWeights.at(move) < 0.0){
                            std::cerr << MoveName << " should be positive!" << endl;
                            return 1;
                        }
                    }
                }
                if (!NameFound){
                    cout << "Could not find " << MoveName << ", please choose one of the following moves: " << endl;
                    for (auto& [move, name]: data.Moves){
                        cout << name << endl;
                    }
                    return 1;
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
            else if (arg == "--Base"){
                data.Base = true;
            }
            else if (arg == "--help"){
                cout << "Usage: " << argv[0] << "--Seed <int> --Heuristic <0/1> -- MinCostNB <0/1> --HistoryLength <+int> -- CM <0/1> --NrTeams <36/100>* --k <0/1/5/10>* --i <0/1/2/3/4>* --TimeLimit <+int> --MaxIt <+int> --Weight <Move> <+int>" << endl;
                cout << "*: For CostMinimization instances" << endl;
                cout << "For TTP instances, specify --TTP 1 --InstanceTTP <BRA24/CIRC40/CON40/GAL40/INCR40/LINE40/N16/NFL32> --NrRounds<+int>" << endl;
                return 1;
            }
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return 1; // Exit with an error code
            }
        }

        if (data.CM == true && data.TTP == true){
            cout << "Choose either CM or TTP!!" << endl;
            return 1;
        }
        if (data.Base){
            cout << "Run base algorithm!!" << endl;
            cout << "Note: if all pairs of rounds are Hamiltonian cycles: never non-Hamiltonian cycles with base algo.." << endl;
        }

        double sum = 0;
        double sum_weights = 0;
        cout << "------ Weights ------" << endl;
        unordered_map<Move, double>InputWeightsCopy = data.InputWeights;
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
                sum += 1.0;
            }
            for (const auto& [move, name]: data.Moves){
                if (data.Base && !data.IsMoveInBase.at(move)){
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

        cout << "Test cost minimization" << endl;
        cout << "MinCostNB = " << data.MinCostNB << endl;
        cout << "TimeLimit = " << data.TimeLimit << endl;
        cout << "Max iterations = " << data.MaxIt << endl;
        if (data.CM){
            data.Instance = to_string(data.NrTeams) + "_" + to_string(data.NrRounds) + "_" + "k" + to_string(data.k) + "_" + to_string(data.inst);
        }
        cin.get();
        TestCostMinimization(data);
        // GenerateCostMatrices(0);
        // cin.get();
    }
    else{
        // TODO: paper about algorithms for Hockey and Miao instances
    }
    return 1;
}
