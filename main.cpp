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

#include "CM.h"
#include "InstanceGenerator.h"

int main(int argc, const char* argv[]){
    int case_ = 0;
    
    // BoundsTTP();
    // return 1;

    if (case_ == 0){

        int seed = 0;
        bool Heuristic = 1;
        bool MinCostNB = 0;
        int HistoryLength = 1;
        int NrTeams = 36;
        int NrRounds = 8;
        int k = 5;
        int inst = 0;
        int TL = 60;
        int MaxIt = 1000000;
        bool CM = true;
        bool TTP = false;
        string Instance;
        if (CM){
            Instance = to_string(NrTeams) + "_" + to_string(NrRounds) + "_" + "k" + to_string(k) + "_" + to_string(inst);
        }
        else {
            Instance = "N16.xml";
        }

        unordered_map<string, double>InputWeights = {{"TS", 0.0}, {"PTS", 0.0}, {"RS", 0.0}, {"PRS", 0.0},{"M", 0.0}, {"BM", 0.0}, {"C", 0.0}};
        unordered_map<string, bool>MoveSeen = {{"TS", false}, {"PTS", false}, {"RS", false}, {"PRS", false},{"M", false}, {"BM", false}, {"C", false}};

        // Parse command-line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--Seed"){
                seed = std::stoi(argv[++i]);
            }
            else if (arg == "--Heuristic"){
                Heuristic = std::stoi(argv[++i]);
                if (Heuristic != 0 && Heuristic != 1){
                    std::cerr << "Heuristic should be 0 or 1" << endl;
                    return 1;
                }
            }
            else if (arg == "--MinCostNB"){
                MinCostNB = std::stoi(argv[++i]);
                if (MinCostNB != 0 && MinCostNB != 1){
                    std::cerr << "MinCostNB should be 0 or 1" << endl;
                    return 1;
                }
            }
            else if (arg == "--HistoryLength"){
                HistoryLength = std::stoi(argv[++i]); 
                if (HistoryLength <= 0){
                    std::cerr << "HistoryLength should be strictly positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--CM"){
                CM = std::stoi(argv[++i]);
                if (CM != 0 && CM != 1){
                    std::cerr << "CM should be 0 or 1" << endl;
                    return 1;
                }
            }
            else if (arg == "--TTP"){
                TTP = std::stoi(argv[++i]);
                if (TTP != 0 && TTP != 1){
                    std::cerr << "TTP should be 0 or 1" << endl;
                    return 1;
                }
            }
            else if (arg == "--NrTeams"){ // CM
                NrTeams = std::stoi(argv[++i]);
                if (NrTeams != 36 && NrTeams != 100 && NrTeams != 250){
                    std::cerr << "NrTeams must be 36 or 100 or 250" << endl;
                    return 1;
                }
                if (NrTeams == 36){
                    NrRounds = 8;
                }
                else if (NrTeams == 100){
                    NrRounds = 16;
                }
                else{
                    NrRounds = 30;
                }
                CM = true;
                TTP = false;
            }
            else if (arg == "--k"){ // CM
                k = std::stoi(argv[++i]);
                if (k != 0 && k != 1 && k != 5 && k != 10){
                    std::cerr << "k must be 0, 1, 5 or 10!" << endl;
                    return 1;
                }
                CM = true;
                TTP = false;
            }
            else if (arg == "--i"){ // CM
                inst = std::stoi(argv[++i]);
                if (inst != 0 && inst != 1 && inst != 2 && inst != 3 && inst != 4){
                    std::cerr << "i must be 0,1,2,3 or 4" << endl;
                    return 1;
                }
                CM = true;
                TTP = false;
            }
            else if (arg == "--InstanceTTP"){ // TTP
                Instance = argv[++i];
                auto it = std::find(InstancesTTP.begin(), InstancesTTP.end(), Instance);
                if (it == InstancesTTP.end()){
                    std::cerr << "Incorrect TTP instance name" << endl;
                    return 1;
                }
                TTP = true;
                CM = false;
            }
            else if (arg == "--NrRounds"){ // TTP
                NrRounds = std::stoi(argv[++i]);
                if (NrRounds <= 0){
                    std::cerr << "NrRounds must be strictly positive!" << endl;
                    return 1;
                }
                TTP = true;
                CM = false;
            }
            else if (arg == "--TimeLimit"){
                TL = std::stoi(argv[++i]);
                if (TL < 0){
                    std::cerr << "TimeLimit should be positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--MaxIt"){
                MaxIt = std::stoi(argv[++i]);
                if (MaxIt < 0){
                    std::cerr << "Max no iterations should be positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--TSw"){
                InputWeights.at("TS") = std::stod(argv[++i]);
                MoveSeen.at("TS") = true;
                if (!(InputWeights.at("TS") >= 0.0 && InputWeights.at("TS") <= 1.0)){
                    std::cerr << "TSw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--PTSw"){
                InputWeights.at("PTS") = std::stod(argv[++i]);
                MoveSeen.at("PTS") = true;
                if (!(InputWeights.at("PTS") >= 0.0 && InputWeights.at("PTS") <= 1.0)){
                    std::cerr << "PTSw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--RSw"){
                InputWeights.at("RS") = std::stod(argv[++i]);
                MoveSeen.at("RS") = true;
                if (!(InputWeights.at("RS") >= 0.0 && InputWeights.at("RS") <= 1.0)){
                    std::cerr << "RSw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--PRSw"){
                InputWeights.at("PRS") = std::stod(argv[++i]);
                MoveSeen.at("PRS") = true;
                if (!(InputWeights.at("PRS") >= 0.0 && InputWeights.at("PRS") <= 1.0)){
                    std::cerr << "PRSw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--Mw"){
                InputWeights.at("M") = std::stod(argv[++i]);
                MoveSeen.at("M") = true;
                if (!(InputWeights.at("M") >= 0.0 && InputWeights.at("M") <= 1.0)){
                    std::cerr << "Mw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--BMw"){
                InputWeights.at("BM") = std::stod(argv[++i]);
                MoveSeen.at("BM") = true;
                if (!(InputWeights.at("BM") >= 0.0 && InputWeights.at("BM") <= 1.0)){
                    std::cerr << "BMw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--Cw"){
                InputWeights.at("C") = std::stod(argv[++i]);
                MoveSeen.at("C") = true;
                if (!(InputWeights.at("C") >= 0.0 && InputWeights.at("C") <= 1.0)){
                    std::cerr << "BMw should be between 0.0 and 1.0" << endl;
                    return 1;
                }
            }
            else if (arg == "--help"){
                cout << "Usage: " << argv[0] << "--Seed <int> --Heuristic <0/1> -- MinCostNB <0/1> --HistoryLength <+int> -- CM <0/1> --NrTeams <36/100>* --k <0/1/5/10>* --i <0/1/2/3/4>* --TimeLimit <+int> --MaxIt <+int> --TSw <[0,1]> --PTSw <[0,1]> --RSw <[0,1]> --PRSw <[0,1]> --Mw <[0,1]> --BMw <[0,1]> --Cw <[0,1]>" << endl;
                cout << "*: For CostMinimization instances" << endl;
                cout << "For TTP instances, specify --TTP 1 --InstanceTTP <BRA24/CIRC40/CON40/GAL40/INCR40/LINE40/N16/NFL32> --NrRounds<+int>" << endl;
                return 1;
            }
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return 1; // Exit with an error code
            }
        }

        if (CM == true && TTP == true){
            cout << "Choose either CM or TTP!!" << endl;
            return 1;
        }

        double sum = 0;
        cout << "------ Weights ------" << endl;
        bool NoMoveSeen = true;
        for (const auto& [move, weight]: InputWeights){
            if (MoveSeen.at(move)){
                sum += InputWeights.at(move);
                NoMoveSeen = false;
            }
            cout << move << ": " << InputWeights.at(move) << endl;
        }
        if (NoMoveSeen){
            for (const auto& [move, weight]: InputWeights){
                InputWeights.at(move) = 1.0 / (double)InputWeights.size();
                sum += InputWeights.at(move);
                cout << move << ": " << InputWeights.at(move) << endl;
            }
        }
        if (sum <= 0.99 || sum >= 1.01){
            cout << "Sum of the weights should be equal to 1.0 but is now" << sum << endl;
            return 1;
        }
        cout << "---------------------" << endl;

        cout << "Test cost minimization" << endl;
        cout << "MinCostNB = " << MinCostNB << endl;
        cout << "TimeLimit = " << TL << endl;
        cout << "Max iterations = " << MaxIt << endl;
        if (CM){
            Instance = to_string(NrTeams) + "_" + to_string(NrRounds) + "_" + "k" + to_string(k) + "_" + to_string(inst);
        }
        TestCostMinimization(seed, Instance, CM, TTP, Heuristic, MinCostNB, HistoryLength, TL, MaxIt, InputWeights, NrRounds);
        // GenerateCostMatrices(0);
        // cin.get();
    }
    else{
        // TODO: paper about algorithms for Hockey and Miao instances
    }
    return 1;
}
