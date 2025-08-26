#include "Graph.h"
#include <iostream>
#include <assert.h>
#include <map>
#include <cmath>

Graph::Graph(const int index, const int N, const int R){
    this->index = index;
    NrNodes = N;
    NrColors = R;
    MatchColor = vector<vector<int>>(N, vector<int>(N, -1)); // diagonals should stay -1, if a team does not play it is also -1
    TeamColorOpp = vector<vector<int>>(N, vector<int>(R, -1));
    WeightsBF = vector<vector<int>>(N+1, vector<int>(N+1, N+1)); // N+1 bc of extra source node!!
    Orientation = vector<vector<HA>>(N+1, vector<HA>(R, HA::BYE));
}

Graph::~Graph(){}

int Graph::getNrBreaks(const int i){
    int NrBreaks = 0;
    for (int c = 1; c < Orientation[i].size(); ++c){
        if (Orientation[i][c-1] == Orientation[i][c]){
            NrBreaks++;
        }
    }
    return NrBreaks;
}

int Graph::NrThreeConsecutiveHA(const int i){
    int NrThreeConsecutive = 0;
    int NrConsecutive = 1;
    for (int c = 1; c < Orientation[i].size(); ++c){
        if (Orientation[i][c-1] == Orientation[i][c]){
            if (++NrConsecutive > 2){
                ++NrThreeConsecutive;
            }
        }
        else{
            NrConsecutive = 1;
        }
    }
    return NrThreeConsecutive;
}

int Graph::getNrBreaksBeginningEnd(const int i){
    int NrBreaks = 0;
    if (Orientation[i][0] == Orientation[i][1]){
        NrBreaks++;
    }
    if (Orientation[i][NrColors-1] == Orientation[i][NrColors-2]){
        NrBreaks++;
    }
    return NrBreaks;
}

int Graph::getImbalanceHalf(const int i){
    int cost = 0;
    int NrH_half;
    const int Half = NrColors/2;
    const vector<pair<int,int>>Halves = {{0, Half}, {Half, NrColors}};
    const int lb = floor((double)Half/2.0);
    const int ub = lb+1;
    for (const auto&[Start, End]: Halves){
        NrH_half = 0;
        for (int c = Start; c < End; ++c){
            if (Orientation[i][c] == HA::H){
                NrH_half++;
            }
        }
        if (NrH_half < lb){
            cost += (lb - NrH_half);
        }
        else if (NrH_half > ub){
            cost += (NrH_half - ub);
        }
    }
    return cost;
}

void Graph::print_round(const int c){
    assert(c >= 0 && c < NrColors);
    std::cout << "ROUND " << c << std::endl;
    vector<bool>NodeSeen(NrNodes, false);
    for (int i = 0; i < NrNodes; i++){
        if (!NodeSeen[i]){
            int j = TeamColorOpp[i][c];
            assert(i != j);
            if (Orientation[i][c] == HA::H){
                std::cout << i << " - " << TeamColorOpp[i][c] << std::endl;
            }
            else {
                std::cout << TeamColorOpp[i][c] << " - " << i << std::endl;
            }
            NodeSeen[i] = true;
            NodeSeen[TeamColorOpp[i][c]] = true;
        }
    }
}

void Graph::print_all_rounds(){
    for (int c = 0; c < NrColors; ++c){
        print_round(c);
    }
}

void Graph::print_2rounds(const int r, const int s){
    print_round(r);
    print_round(s);
}

void Graph::print_opponents(const int i){
    std::cout << "TEAM " << i << std::endl;
    for (int c = 0; c < NrColors; c++){
        std::cout << TeamColorOpp[i][c] << " ";
    }
    std::cout << std::endl;
}

void Graph::print_opponents_all_teams(){
    for (int i = 0; i < NrNodes; i++){
        print_opponents(i);
    }
}

void Graph::print_opponents_2teams(const int i, const int j){
    print_opponents(i);
    print_opponents(j);
}

bool Graph::IsMatchFeasible2RR(const int h, const int a, const int r){
    const int Half = NrColors/2;
    if (MatchColor[h][a] != -1 && MatchColor[h][a] != r){
        // we cannot have twice the same game!
        return false;
    }
    if (r < Half){
        if (MatchColor[a][h] < Half){
            return false;
        }
    }
    else{
        if (MatchColor[a][h] >= Half){
            return false;
        }
    }
    return false;
}
