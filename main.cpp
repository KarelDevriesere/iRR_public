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
    if (case_ == 0){

        int seed = 0;
        bool Heuristic = 1;
        bool MinCostNB = 1;
        int HistoryLength = 1;
        int NrTeams = 36;
        int NrRounds = 8;
        int k = 5;
        int inst = 0;
        int TL = 60;
        string Instance = to_string(NrTeams) + "_" + to_string(NrRounds) + "_" + "k" + to_string(k) + "_" + to_string(inst);

        unordered_map<string, double>Weights = {{"TS", 1.0/Weights.size()}, {"PTS", 1.0/Weights.size()}, {"RS", 1.0/Weights.size()}, {"PRS", 1.0/Weights.size()},{"M", 1.0/Weights.size()}, {"BM", 1.0/Weights.size()}, {"C", 1.0/Weights.size()}};

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
                if (HistoryLength < 0){
                    std::cerr << "HistoryLength should be positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--NrTeams"){
                NrTeams = std::stoi(argv[++i]);
                if (NrTeams != 36 && NrTeams != 100){
                    std::cerr << "NrTeams must be 36 or 100" << endl;
                    return 1;
                }
                if (NrTeams == 36){
                    NrRounds = 8;
                }
                else{
                    NrRounds = 16;
                }
            }
            else if (arg == "--k"){
                k = std::stoi(argv[++i]);
                if (k != 0 && k != 1 && k != 5 && k != 10){
                    std::cerr << "k must be 0, 1, 5 or 10!" << endl;
                    return 1;
                }
            }
            else if (arg == "--i"){
                inst = std::stoi(argv[++i]);
                if (inst != 0 && inst != 1 && inst != 2 && inst != 3 && inst != 4){
                    std::cerr << "i must be 0,1,2,3 or 4" << endl;
                    return 1;
                }
            }
            else if (arg == "--TL"){
                TL = std::stoi(argv[++i]);
                if (TL < 0){
                    std::cerr << "TimeLimit should be positive" << endl;
                    return 1;
                }
            }
            else if (arg == "--help"){
                cout << "Usage: " << argv[0] << "--Seed <int> --Heuristic <0/1> -- MinCostNB <0/1> --HistoryLength <+int> --NrTeams <36/100> --k <0/1/5/10> --i <0/1/2/3/4> --TL <+int>" << endl;
                return 1;
            }
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return 1; // Exit with an error code
            }
        }

        cout << "Test cost minimization" << endl;
        cout << "MinCostNB = " << MinCostNB << endl;
        cout << "TimeLimit = " << TL << endl;
        Instance = to_string(NrTeams) + "_" + to_string(NrRounds) + "_" + "k" + to_string(k) + "_" + to_string(inst);
        cout << "Instance: " << Instance << endl;
        TestCostMinimization(seed, Instance, Heuristic, MinCostNB, HistoryLength, TL);
        // GenerateCostMatrices(0);
        // cin.get();
    }
    else{
        // TODO: paper about algorithms for Hockey and Miao instances
    }
    return 1;
}