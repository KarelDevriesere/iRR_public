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

        bool ViolationHAP_allowed = false; // whether we allow solutions that violate the HAP constraints (at a high cost)
        bool ViolationEligibleOpponents_allowed = false; // whether we allow solutions where non-eligible opponents play vs each other (at a high cost)
        // We only allow this in TS and PTS because in the matchings wo do not create those edges
        bool ViolationSameClubAllowed = false;
        bool ConstraintViolationAllowed = false;

        Solution(const Input& in);
        ~Solution();

        vector<vector<int>>MatchColor;
        vector<vector<vector<int>>>MatchColor2RR; // In 2RR, we can have (i,j) with colour c and (i,j) with colour d!!
        vector<vector<int>>TeamColorOpp;
        vector<vector<int>>WeightsBF; // weights for the Bellman Ford
        vector<vector<HA>>Orientation;

        int NonEligibleCost = 1000000;
        int CostCapacityViol = 1000000;
        int CostSameClub = 1000000;
        int HighCostHAPs = 1000000;
        int TravelCost = 1;

        int Cost2RRSameHalf = 1000000;
        int Cost2RRSameMode = 1000000;

        int CostTTPViolation = 1000000;
        int CostImbalance = 1000000;

        void SetColorMatch(const int h, const int a, const int c);

        int getCostTTPViolation()const{return CostTTPViolation;};
        int getCostImbalance()const{return CostImbalance;};

        void SetOneCostAllViolations(const int cost);

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
        int ComputeTotalCostYSTP();
        int ComputeTotalHACost();
        int ComputeCapacityClubRound(const int c, const int r)const;
        int ComputeCostCapacities();
        int ComputeTravelCostTeam(const int i);
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

        int ComputeTTPViolations(const int i, const int min_round, const int max_round);
        int ComputeTotalCostTTP();
        int ComputeTotalCostTTPViolations();
        int ComputeCostMatchRoundTTP(const int h, const int a, const int r);
        int ComputeTravelCostTeamTTP(const int t)const;
        int ComputeTravelCostTTP();
        int ComputeTotalCostTeamTTP(const int i);

        int ComputeCostGeneralMatrix();

        int getNrSameClub(const int i);
        int getNrColouredRounds()const{return NrColouredRounds;};

        void clear();
        bool validate();

        vector<vector<bool>>TeamsHAP; // HAP of the teams, this is for algorithm of Miao
};

#endif
