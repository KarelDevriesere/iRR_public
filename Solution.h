#ifndef SOLUTION_H  
#define SOLUTION_H

#include "Input.h"
// #include "Graph.h"
#include <climits>

using namespace std;

class Solution: public Input{
    private:

    public:

        int NrColouredRounds;

        bool ViolationHAP_allowed = true; // whether we allow solutions that violate the HAP constraints (at a high cost)
        bool ViolationEligibleOpponents_allowed = true; // whether we allow solutions where non-eligible opponents play vs each other (at a high cost)
        // We only allow this in TS and PTS because in the matchings wo do not create those edges
        bool ViolationSameClubAllowed = true;

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
        int ComputeTotalCostMiaoHockey();
        int ComputeTotalHACost();
        int ComputeCapacityClubRound(const int c, const int r);
        int ComputeCostCapacities();
        int ComputeTravelCost();
        int ComputeCostNonEligibleOpponents();
        int ComputeCostSameClub();
        int ComputeCost2RRConstraintTeam(const int i);
        int ComputeCost2RRConstraint();
        int ComputeCostReversingOrientationTeam(const int i, const int r1, const int r2);
        
        int ComputeHACostTeam(const int i);

        int ComputeCostThreeConsecutive();
        int ComputeCostBreakLimit();
        int ComputeCostBreakBeginningEnd();
        int ComputeCostQuarterBalanced();

        int ComputeTTPViolations(const int i);
        int ComputeTotalCostTTP();
        int ComputeTotalCostTTPViolations();
        int ComputeCostMatchRoundTTP(const int h, const int a, const int r);
        int ComputeTravelCostTeamTTP(const int t);
        int ComputeTravelCostTTP();

        int ComputeCostGeneralMatrix();

        int getNrSameClub(const int i);

        void clear();
        bool validate();

        vector<vector<bool>>TeamsHAP; // HAP of the teams, this is for algorithm of Miao
};

#endif
