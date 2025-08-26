#ifndef SOLUTION_H  
#define SOLUTION_H

#include "Input.h"
// #include "Graph.h"
#include <climits>

using namespace std;

class Solution: public Input{
    private:

    public:

        bool ViolationHAP_allowed = false; // whether we allow solutions that violate the HAP constraints (at a high cost)
        bool ViolationEligibleOpponents_allowed = false; // whether we allow solutions where non-eligible opponents play vs each other (at a high cost)
        // We only allow this in TS and PTS because in the matchings wo do not create those edges

        Solution(const Input& in);
        ~Solution();

        vector<vector<int>>MatchColor;
        vector<vector<int>>TeamColorOpp;
        vector<vector<int>>WeightsBF; // weights for the Bellman Ford
        vector<vector<HA>>Orientation;

        int NonEligibleCost = 1000;
        int CostCapacityViol = 1000;
        int CostSameClub = 1000;
        int HighCostHAPs = 100;
        int TravelCost = 1;

        int Cost2RRSameHalf = 1000;
        int Cost2RRSameMode = 1000;

        void PrintAllRoundsLeague(const int l);
        bool IsTeamBalanced(const int i);
        int getNrHomeTeam(const int i);
        int getNrBreaks(const int i);
        int NrThreeConsecutiveHA(const int i); 
        int getNrBreaksBeginningEnd(const int i);
        int getImbalanceHalf(const int i);
        int CostCapacityClubHapSwitchTeam(const int i, const int r);

        bool IsMatchFeasible2RR(const int h, const int a, const int r);

        int ComputeTotalCost();
        int ComputeTotalHACost();
        int ComputeCapacityClubRound(const int c, const int r);
        int ComputeCostCapacities();
        int ComputeTravelCost();
        int ComputeCostNonEligibleOpponents();
        int ComputeCostSameClub();
        int ComputeCost2RRConstraint();
        
        int ComputeHACostTeam(const int i);

        int ComputeCostThreeConsecutive();
        int ComputeCostBreakLimit();
        int ComputeCostBreakBeginningEnd();
        int ComputeCostQuarterBalanced();

        int getNrSameClub(const int i);

        void clear();
        void validate();

        vector<vector<bool>>TeamsHAP; // HAP of the teams, this is for algorithm of Miao
};

#endif
