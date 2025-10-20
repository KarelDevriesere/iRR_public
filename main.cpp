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

int main(int argc, char** argv){
    int case_ = std::stoi(argv[1]);
    if (case_ == 0){
        int seed = std::stoi(argv[2]);
        bool Heuristic = std::stoi(argv[3]);
        bool MinCostNB = std::stoi(argv[4]); // whether we always choose the move of min cost or random move
        int HistoryLength = std::stoi(argv[5]);
        if (HistoryLength < 0){
            throw std::runtime_error("HistoryLength should be positive");
        }
        const int NrTeams = std::stoi(argv[6]);
        if (NrTeams != 36 && NrTeams != 100){
            throw std::runtime_error("NrTeams must be 36 or 100");
        }
        int NrRounds;
        if (NrTeams == 36){
            NrRounds = 8;
        }
        else{
            NrRounds = 16;
        }
        const int k = std::stoi(argv[7]);
        if (k != 0 && k != 1 && k != 5 && k != 10){
            throw std::runtime_error("k must be 0, 1, 5 or 10!");
        }
        const int i = std::stoi(argv[8]);
        if (i != 0 && i != 1 && i != 2 && i != 3 && i != 4){
            throw std::runtime_error("i must be 0,1,2,3 or 4");
        }
        const int TL = std::stoi(argv[9]);
        if (TL < 0){
            throw std::runtime_error("TimeLimit should be positive");
        }
        cout << "Test cost minimization" << endl;
        cout << "MinCostNB = " << MinCostNB << endl;
        cout << "TimeLimit = " << TL << endl;
        const string Instance = to_string(NrTeams) + "_" + to_string(NrRounds) + "_" + "k" + to_string(k) + "_" + to_string(i);
        TestCostMinimization(seed, Instance, Heuristic, MinCostNB, HistoryLength, TL);
        // GenerateCostMatrices(0);
        // cin.get();
    }
    else{
        // TODO: paper about algorithms for Hockey and Miao instances
    }
    return 1;
}