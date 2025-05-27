#ifndef GRAPH_H  
#define GRAPH_H

#include <vector>
#include <array>

using namespace std;

enum class HA{H, A, BYE};

class Graph{

    private:
        bool directed = true;
        int index;

    public:

        int NrNodes;
        int NrColors;

        vector<vector<int>>MatchColor;
        vector<vector<int>>TeamColorOpp;
        vector<vector<int>>WeightsBF; // weights for the Bellman Ford
        vector<vector<HA>>Orientation;

        Graph(const int index, const int N, const int R); // N: Nr of teams (=nodes), R: Nr of rounds (=colors)
        ~Graph();

        int getNrBreaks(const int i);
        int NrThreeConsecutiveHA(const int i); 
        int getNrBreaksBeginningEnd(const int i);
        int getImbalanceHalf(const int i);

        int getIndex()const{return index;};
        void print_round(const int c);
        void print_opponents(const int i);
        void print_2rounds(const int r, const int s);
        void print_all_rounds();
        void print_opponents_2teams(const int i, const int j);
        void print_opponents_all_teams();
        bool isDirected()const{return directed;};

        bool IsMatchFeasible2RR(const int h, const int a, const int r);
};

#endif