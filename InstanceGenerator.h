#include <iostream>
#include <array>
#include <vector>
#include <algorithm> 
#include <random>  
#include <string>
#include <fstream>
#include <unordered_set>

#include "Algo.h"

using namespace std;

enum class CostMatrixType{ZeroOne, ZeroHundred, Uthus};

const array<pair<int,int>,2>TeamsRounds{{{36,8},{100,16}}};
const array<double,5>Rho{0.5,0.6,0.7,0.8,0.9};
const int NrInstancesPerCombination = 5;

vector<vector<bool>> GenerateInstance(const int NrTeams, const int NrRounds, const int k, const int seed){
    // for iRR: first generate an r-regular subgraph that is r-colorable, with Vizing
    vector<vector<bool>>A = VizingRegularColorableGraph(NrTeams, NrRounds, seed);
    // k = nr of opponents on top of the NrRounds opponents already in A;
    // for this, we need the degrees of vertices
    if (k > 0){
        // cout << "add to each vertex k edges" << endl;
        int MaxDegree = NrRounds+k;
        vector<int>VertexDegree(NrTeams, NrRounds);
        int NrUnusedEdges = (NrTeams/2)*(NrTeams-1-NrRounds);
        vector<pair<int,int>>AvailablePairs(NrUnusedEdges);
        int p = 0;
        for (int i = 0; i < NrTeams; ++i){
            for (int j = i+1; j < NrTeams; ++j){
                if (A[i][j] == 0){
                    AvailablePairs[p++] = {i,j};
                }
                /*
                else{
                    cout << i << "-" << j << "in orginal graph" << endl;
                }
                */
            }
        }
        int i,j;
        while (!AvailablePairs.empty()){
            p = rand()%AvailablePairs.size();
            i = AvailablePairs[p].first, j = AvailablePairs[p].second;
            A[i][j] = 1, A[j][i] = 1;
            // cout << "add " << i << "-" << j << " to the graph" << endl;
            AvailablePairs[p] = AvailablePairs.back();
            AvailablePairs.pop_back();
            for (int h: {i,j}){
                if (++VertexDegree[h] >= MaxDegree){
                    p = 0;
                    while (p < AvailablePairs.size()){
                        if (AvailablePairs[p].first == h || AvailablePairs[p].second == h){
                            AvailablePairs[p] = AvailablePairs.back();
                            AvailablePairs.pop_back();
                        }
                        else{
                            ++p;
                        }
                    }
                }
            }
        }
    }
    return A;
}

vector<vector<vector<int>>> Generate01CostMatrix(const int NrTeams, const int NrRounds, const double rho, const int seed, vector<vector<bool>>A){
    // this exactly how Jasper does it in his paper of "Integer programming models for round robin tournaments"
    vector<array<int,3>>MatchRoundTriples((NrTeams)*(NrTeams-1)*NrRounds);
    int index=0;
    int i,j,r;
    for (i = 0; i < NrTeams; ++i){
        for (j = 0; j < NrTeams; ++j){
            if (i != j && A[i][j] == 1){
                for (r = 0; r < NrRounds; ++r){
                    // cout << "triple at index " << index << " = " << "{" << i << "," << j << "," << r << "}" << endl;
                    MatchRoundTriples.at(index++) = {i,j,r};
                }
            }
        }
    }
    // first, pick a set of match-rounds uniformly at random (but with a given seed)
    std::mt19937 gen(seed);
    std::shuffle(MatchRoundTriples.begin(), MatchRoundTriples.end(), gen);
    const int NrTriples1 = rho*(double)MatchRoundTriples.size();

    vector<vector<vector<int>>>CostMatrix(NrTeams,vector<vector<int>>(NrTeams,vector<int>(NrRounds, 0)));
    int s;
    for (s = 0; s < NrTriples1; ++s){
        i = MatchRoundTriples.at(s).at(0);
        j = MatchRoundTriples.at(s).at(1);
        r = MatchRoundTriples.at(s).at(2);
        CostMatrix.at(i).at(j).at(r) = 1;
    }
    const int BigM = (NrTeams/2)*NrRounds+1; // so that it is always better to not plan this game
    for (i = 0; i < NrTeams; ++i){
        for (j = 0; j < NrTeams; ++j){
            if (A[i][j] == 0){
                for (r = 0; r < NrRounds; ++r){
                    CostMatrix.at(i).at(j).at(r) = BigM;
                }
            }
        }
    }
    return CostMatrix;
}

vector<vector<vector<int>>> GenerateUniformRangeCostMatrix(const int NrTeams, const int NrRounds, const int seed, vector<vector<bool>>A, const int UB){
    // this exactly how Jasper does it in his paper of "Integer programming models for round robin tournaments"

    std::mt19937 gen(seed);
    uniform_int_distribution<>distr(0,UB);

    vector<vector<vector<int>>>CostMatrix(NrTeams,vector<vector<int>>(NrTeams,vector<int>(NrRounds, 0)));

    const int BigM = UB*(NrTeams/2)*NrRounds+1; // so that it is always better to not plan this game
    int i,j,r;
    for (i = 0; i < NrTeams; ++i){
        for (j = 0; j < NrTeams; ++j){
            if (A[i][j] == 0){
                for (r = 0; r < NrRounds; ++r){
                    CostMatrix.at(i).at(j).at(r) = BigM;
                }
            }
            else{
                for (r = 0; r < NrRounds; ++r){
                    CostMatrix.at(i).at(j).at(r) = distr(gen);
                }
            }
        }
    }
    return CostMatrix;
}


void StoreCostMatrix(const int NrTeams, const int NrRounds, const int k, const int inst, vector<vector<vector<int>>>& CostMatrix, CostMatrixType type_){

    string path = "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel";
    if (type_ == CostMatrixType::ZeroOne){
        path += (std::string(PATHSEP) + "0_1");
    }
    else if (type_ == CostMatrixType::ZeroHundred){
        path += (std::string(PATHSEP) + "0_100");
    }
    else{
        path += (std::string(PATHSEP) + "Uthus");
    }
    path += (std::string(PATHSEP) + to_string(NrTeams) + "_" + to_string(NrRounds) + "_k" + to_string(k) + "_" + to_string(inst) + ".txt");
    cout << "Save cost matrix in file " << path << endl;
    std::ofstream output_file(path);
    output_file << NrTeams << "," << NrRounds << "\n";
    int i,j,r;
    for (i = 0; i < NrTeams; ++i){
        for (j = 0; j < NrTeams; ++j){
            for (r = 0; r < NrRounds; ++r){
                output_file << i << "," << j << "," << r << "," << CostMatrix.at(i).at(j).at(r) << "\n";
            }
        }
    }
    output_file.close();
}

int GenerateCostMatrices(const int global_seed){
    cout << "The instances created are too easy.." << endl;
    std::mt19937 gen(global_seed);
    uniform_int_distribution<>distr(0,1000);
    int NrTeams, NrRounds, seed;
    for (auto& tuple: TeamsRounds){
        NrTeams = tuple.first;
        NrRounds = tuple.second;
        for (int inst = 0; inst < NrInstancesPerCombination; ++inst){
            cout << "Create cost matrix for " << NrTeams << ", " << NrRounds << ", " << inst << endl;
            seed = distr(gen);

            for (int k: {0,5,10}){
                vector<vector<bool>>A = GenerateInstance(NrTeams, NrRounds, k, seed);

                vector<vector<vector<int>>>CostMatrix01 = Generate01CostMatrix(NrTeams,NrRounds,0.70,seed,A); // rho=0.70
                StoreCostMatrix(NrTeams, NrRounds, k, inst, CostMatrix01, CostMatrixType::ZeroOne);

                int UB = 100;
                vector<vector<vector<int>>>CostMatrix0100 = GenerateUniformRangeCostMatrix(NrTeams,NrRounds,seed,A,UB); 
                StoreCostMatrix(NrTeams, NrRounds, k, inst, CostMatrix0100, CostMatrixType::ZeroHundred);

                UB = NrTeams*NrTeams;
                vector<vector<vector<int>>>CostMatrixUthus = GenerateUniformRangeCostMatrix(NrTeams,NrRounds,seed,A,UB); 
                StoreCostMatrix(NrTeams, NrRounds, k, inst, CostMatrixUthus, CostMatrixType::Uthus);
            }
        }
    }
    return 0;
}