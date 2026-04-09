#include "Solution.h"
#include <iostream>
#include <assert.h>
#include <cmath>

Solution::Solution(const Input& in) : Input(in) {

    int N = getNrTeams();
    int R = getNrRounds();

    MatchColor = vector<vector<int>>(N, vector<int>(N, -1)); // diagonals should stay -1, if a team does not play it is also -1
    TeamColorOpp = vector<vector<int>>(N, vector<int>(R, -1));
    Orientation = vector<vector<HA>>(N, vector<HA>(R, HA::BYE));

    if (IsBaseAlgo()){
        NrColouredRounds = getNrRoundsBaseAlgo();
    }
    else{
        NrColouredRounds = getNrRounds();
    }
}
Solution::~Solution(){}

void Solution::SetColorMatch(const int h, const int a, const int c){
    MatchColor[h][a] = c;
    if (SRR){
        MatchColor[a][h] = c;
    } 
    TeamColorOpp[h][c] = a;
    TeamColorOpp[a][c] = h;
    // sol.Orientation[h][c] = HA::H;
    // sol.Orientation[a][c] = HA::A;
}

void Solution::SetOneCostAllViolations(const int cost){
    NonEligibleCost = cost;
    CostCapacityViol = cost;
    CostSameClub = cost;
    HighCostHAPs = cost;

    Cost2RRSameHalf = cost;
    Cost2RRSameMode = cost;

    CostTTPViolation = cost;
    CostImbalance = cost;
}

void Solution::PrintAllRoundsLeague(const int l){
    for (int r = 0; r < NrColouredRounds; ++r){
        cout << "ROUND " << r << endl;
        cout << "--------" << endl;
        vector<bool>TeamSeen(getNrTeams(), false);
        for (int i_ = 0; i_ < getNrTeamsLeague(l); ++i_){
            int i = getTeamsLeague(l)[i_];
            if (!TeamSeen[i]){
                int j = TeamColorOpp[i][r];
                TeamSeen[j] = true;
                if (Orientation[i][r] == HA::H){
                    cout << i << "-" << j << endl;
                    assert(MatchColor[i][j] == r);
                }
                else{
                    assert((Orientation[i][r] == HA::A));
                    cout << j << "-" << i << endl;
                    if (MatchColor[j][i] != r){
                        cout << "color = " << r << endl;
                    }
                    assert(MatchColor[j][i] == r);
                }
            }
        }
        cout << "--------" << endl;
    }
}

int Solution::getNrBreaks(const int i){
    int NrBreaks = 0;
    for (int c = 1; c < (int)Orientation[i].size(); ++c){
        if (Orientation[i][c-1] == Orientation[i][c]){
            NrBreaks++;
        }
    }
    return NrBreaks;
}

int Solution::getNrHomeTeam(const int i){
    int nr_H = 0;
    for (int r = 0; r < NrColouredRounds; ++r){
        if (Orientation[i][r] == HA::H){
            nr_H++;
        }
    }
    return nr_H;
}

bool Solution::IsTeamBalanced(const int i){
    int nr_H = 0, nr_A = 0;
    for (int r = 0; r < NrColouredRounds; ++r){
        if (Orientation[i][r] == HA::H){
            nr_H++;
        }
        else{
            assert(Orientation[i][r] == HA::A);
            nr_A++;
        }
    }
    // For 2 rounds, the following assert can easily be violated:
    // assert(nr_H > 0);
    // assert(nr_A > 0);
    if (nr_H != nr_A){
        return false;
    }
    nr_H = 0, nr_A = 0;
    if (!SRR){
        for (int r = 0; r < NrColouredRounds; ++r){
            int j = TeamColorOpp[i][r];
            if (MatchColor[i][j] == r){
                assert(Orientation[i][r] == HA::H);
                assert(MatchColor[i][j] != MatchColor[j][i]);
                nr_H++;
            }
            else{
                assert(Orientation[i][r] == HA::A);
                assert(MatchColor[j][i] == r);
                nr_A++;
            }
        }
        // assert(nr_H > 0);
        // assert(nr_A > 0);
        if (nr_H == nr_A){
            return true;
        }
        else{
            return false;
        }
    }
    else{
        return true;
    }
}

int Solution::NrThreeConsecutiveHA(const int i){
    int NrThreeConsecutive = 0;
    int NrConsecutive = 1;
    /*
    cout << "HAP of " << i << ": ";
    if (Orientation[i][0] == HA::H){
        cout << "H";
    }
    else if (Orientation[i][0] == HA::A){
        cout << "A";
    }
    else{
        cout << "B";
    }
        */
    for (int c = 1; c < Orientation[i].size(); ++c){
        /*
        if (Orientation[i][c] == HA::H){
            cout << "H";
        }
        else if (Orientation[i][c] == HA::A){
            cout << "A";
        }
        else{
            cout << "B";
        }
            */
        if (Orientation[i][c-1] == Orientation[i][c]){
            if (++NrConsecutive > 2){
                ++NrThreeConsecutive;
            }
        }
        else{
            NrConsecutive = 1;
        }
    }
    // cout << endl;
    return NrThreeConsecutive;
}

int Solution::getNrBreaksBeginningEnd(const int i){
    int NrBreaks = 0;
    if (Orientation[i][0] == Orientation[i][1]){
        NrBreaks++;
    }
    if (Orientation[i][NrColouredRounds-1] == Orientation[i][NrColouredRounds-2]){
        NrBreaks++;
    }
    return NrBreaks;
}

int Solution::getImbalanceHalf(const int i){
    int cost = 0;
    int NrH_half;
    const int Half = NrColouredRounds/2;
    const int lb = std::floor((double)Half/2.0);
    const int ub = lb+1;
    for (const auto&[Start, End]: {pair{0, Half}, pair{Half, NrColouredRounds}}){
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

int Solution::getNrSameClub(const int i){
    int j;
    int nr = 0;
    for (int r = 0; r < NrColouredRounds; ++r){
        j = TeamColorOpp[i][r];
        if (j != -1 && getTeamClub(j) == getTeamClub(i)){
            ++nr;
        }
    }
    return nr;
}

int Solution::ComputeCostReversingOrientationTeam(const int i, const int r1, const int r2){
    int cost = 0;
    if (getSetting() == Setting::TTP){
        cost -= (getCostTTPViolation()*ComputeTTPViolations(i,0,getNrRounds()-1) + ComputeTravelCostTeamTTP(i));
    }
    else if (getSetting() == Setting::Football || getSetting() == Setting::Hockey){
        cost -= ComputeHACostTeam(i); //  not possible to take capacity violations into account because multiple teams in same club!!
        // Also, then we get negative costs which is not allowed for dijkstra shortest paths
    }
    std::swap(Orientation[i][r1], Orientation[i][r2]);
    // cout << "swap orientation of " << i << " in " << r1 << " and " << r2 << endl;
    if (getSetting() == Setting::TTP){
        cost += (getCostTTPViolation()*ComputeTTPViolations(i,0,getNrRounds()-1) + ComputeTravelCostTeamTTP(i));
    }
    else if (getSetting() == Setting::Football || getSetting() == Setting::Hockey){
        cost += ComputeHACostTeam(i); //  not possible to take capacity violations into account because multiple teams in same club!!
        // Also, then we get negative costs which is not allowed for dijkstra shortest paths
    }
    std::swap(Orientation[i][r1], Orientation[i][r2]);
    // cout << "cost = " << cost << endl;
    return cost;
}

int Solution::ComputeCost2RRConstraintTeam(const int i){
    int H = NrColouredRounds/2;
    int cost = 0;

    vector<int>NodeSeenH1(getNrTeams(), false);
    vector<int>NodeSeenA1(getNrTeams(), false);
    vector<int>NodeSeenH2(getNrTeams(), false);
    vector<int>NodeSeenA2(getNrTeams(), false);
    for (int r = 0; r < H; ++r){
        if (Orientation[i][r] == HA::H){
            NodeSeenH1[TeamColorOpp[i][r]]++;
        }
        else{
            NodeSeenA1[TeamColorOpp[i][r]]++;
        }
    }
    for (int r = H; r < NrColouredRounds; ++r){
        if (Orientation[i][r] == HA::H){
            NodeSeenH2[TeamColorOpp[i][r]]++;
        }
        else{
            NodeSeenA2[TeamColorOpp[i][r]]++;
        }
    }
    for (int j = 0; j < getNrTeams(); ++j){
        assert(NodeSeenH1[j] + NodeSeenA1[j] + NodeSeenH2[j] + NodeSeenA2[j] <= 2);
        // cost for seeing an opponent twice in one 1 half
        cost += Cost2RRSameHalf*max(0, NodeSeenH1[j]+NodeSeenA1[j]-1);
        cost += Cost2RRSameHalf*max(0, NodeSeenH2[j]+NodeSeenA2[j]-1);
        // cost for seeing an opponent twice at H or twice A over the halfs
        cost += Cost2RRSameMode*(max(0, NodeSeenH1[j]+NodeSeenH2[j]-1));
        cost += Cost2RRSameMode*(max(0, NodeSeenA1[j]+NodeSeenA2[j]-1));
    }

    return cost;
}

int Solution::ComputeCost2RRConstraint(){
    int H = NrColouredRounds/2;
    int cost = 0;
    for (int i = 0; i < getNrTeams(); ++i){
        cost += ComputeCost2RRConstraintTeam(i);
    }
    return cost;
}


int Solution::ComputeCostNonEligibleOpponents(){
    int cost = 0;
    int j;
    for (int c = 0; c < NrColouredRounds; c++){
        for (int i = 0; i < getNrTeams(); i++){
            j = TeamColorOpp[i][c];
            if (i < j){
                continue;
            }
            if (!isEligible(i, j)){
                cost++;
            }
        }
    }
    return NonEligibleCost*cost;
}

int Solution::ComputeCostSameClub(){
    int NrSameClub = 0;
    for (int i = 0; i < getNrTeams(); ++i){
        NrSameClub += getNrSameClub(i);
    }
    return CostSameClub*max(0, NrSameClub - getMaxSameClub());
}

int Solution::ComputeTravelCostTeam(const int i){
    int cost = 0;
    for (int c = 0; c < NrColouredRounds; c++){
        cost += getDistanceTeams(i, TeamColorOpp[i][c]);
    }
    return cost;
}

int Solution::ComputeTravelCost(){
    int cost = 0;
    int j;
    for (int c = 0; c < NrColouredRounds; c++){
        /*
        cout << "------------" << endl;
        cout << "Round " << c << endl;
        cout << "------------" << endl;
        */
        for (int i = 0; i < getNrTeams(); i++){
            j = TeamColorOpp[i][c];
            if (i < j){
                continue;
            }
            // cout << "Opponent of " << i << ":" << endl;
            // cout << TeamColorOpp[i][c] << endl;
            cost += getDistanceTeams(i, j);
        }
    }
    return TravelCost*cost;
}

int Solution::ComputeCapacityClubRound(const int c, const int r)const{
    int cap = 0;
    for (auto& i: getTeamsClub(c)){
        if (Orientation[i][r] == HA::H){
            cap++;
        }
    }
    return cap;
}

int Solution::CostCapacityClubHapSwitchTeam(const int i, const int r){
    // Calculate the impact of switching the mode of i in round r on the capacity of the club of i
    // Optimal if one team per club
    const int c = getTeamClub(i);
    const int cap = ComputeCapacityClubRound(c, r);
    int cost_before = max(0, cap - getCapacityClub(c,r));
    int cost_after = 0;
    if (Orientation[i][r] == HA::H){
        cost_after = max(0, cap - 1 - getCapacityClub(c,r));
    }
    else if (Orientation[i][r] == HA::A){
        cost_after = max(0, cap + 1 - getCapacityClub(c,r));
    }
    return (cost_after-cost_before)*CostCapacityViol;
}

int Solution::ComputeCostCapacities(){
    int cost = 0;
    int cap;
    for (int c = 0; c < getNrClubs(); ++c){
        for (int r = 0; r < NrColouredRounds; ++r){
            cap = ComputeCapacityClubRound(c, r);
            cost += max(0, cap - getCapacityClub(c,r));
            /*
            if (cap > getCapacityClub(c,r)){
                cout << "club " << c << " violated in round " << r << endl;
                for (auto t: getTeamsClub(c)){
                    if (Orientation[t][r] == HA::H){
                        cout << t << " plays H in round " << r << endl; 
                    }
                }
            }
                */
        }
    }
    int final_cost = max(0, cost-getAllowedNrCapacityViolations());

    return CostCapacityViol*final_cost;
}

int Solution::ComputeCostThreeConsecutive(){
    int cost = 0;
    if (!getHAP_requirement(HAP_requirement_name::NoThreeConsecutive)){
        return 0;
    }
    for (int i = 0; i < getNrTeams(); ++i){
        cost += NrThreeConsecutiveHA(i);
    }
    return HighCostHAPs*cost;
}

int Solution::ComputeCostBreakBeginningEnd(){
    if (!getHAP_requirement(HAP_requirement_name::NoBreakBeginningEnd)){
        return 0;
    }
    int cost = 0;
    for (int i = 0; i < getNrTeams(); ++i){
        cost += getNrBreaksBeginningEnd(i);
    }
    return HighCostHAPs*cost;
}

int Solution::ComputeCostBreakLimit(){
    if (!getHAP_requirement(HAP_requirement_name::BreakLimit)){
        return 0;
    }
    int cost = 0;
    for (int i = 0; i < getNrTeams(); ++i){
        cost += max(0, getNrBreaks(i)-getBreakLimit());
    }
    return HighCostHAPs*cost;
}

int Solution::ComputeCostQuarterBalanced(){
    if (!getHAP_requirement(HAP_requirement_name::QuarterBalanced)){
        return 0;
    }
    int cost = 0;
    for (int i = 0; i < getNrTeams(); ++i){
        cost += getImbalanceHalf(i);
    }
    return HighCostHAPs*cost;
}

int Solution::ComputeHACostTeam(const int i){
    int cost = 0;
    if (getHAP_requirement(HAP_requirement_name::NoThreeConsecutive)){
        cost += NrThreeConsecutiveHA(i);
        if (NrThreeConsecutiveHA(i) > 0){
            // cout << "NrThreeConsecutiveHA violated for " << i << endl;
        }
        // assert(NrThreeConsecutiveHA(i) == 0);
    }
    if (getHAP_requirement(HAP_requirement_name::NoBreakBeginningEnd)){
        cost += getNrBreaksBeginningEnd(i);
        if (getNrBreaksBeginningEnd(i) > 0){
            // cout << "NrBreaksBeginningEnd violated for " << i << endl;
        }
        // assert(getNrBreaksBeginningEnd(i) == 0);
    }
    if (getHAP_requirement(HAP_requirement_name::BreakLimit)){
        cost += max(0, getNrBreaks(i)-getBreakLimit());
        if (max(0, getNrBreaks(i)-getBreakLimit()) > 0){
            // cout << "BreakLimit violated for " << i << endl;
        }
        // assert(getNrBreaks(i) <= getBreakLimit());
    }
    if (getHAP_requirement(HAP_requirement_name::QuarterBalanced)){
        cost += getImbalanceHalf(i);
        if (getImbalanceHalf(i) > 0){
            // cout << "NrBreaksBeginningEnd violated for " << i << endl;
        }
        // assert(getImbalanceHalf(i) == 0);
    }
    if (!IsTeamBalanced(i)){
        // cout << i << "Not balanced!!" << endl;
        // cin.get();
        cost++;
    }
    return HighCostHAPs*cost;
}

int Solution::ComputeTotalHACost(){
    int cost = ComputeCostCapacities();
    // cout << "Cost capacities = " << cost << endl;
    for (int i = 0; i < getNrTeams(); ++i){
        cost += ComputeHACostTeam(i);
    }
    return cost;
}

int Solution::ComputeTotalCostYSTP(){
    // cout << "Compute total cost" << endl;
    int travel_cost = ComputeTravelCost();
    // cout << "Travel cost = " << travel_cost << endl;
    int HA_cost = ComputeTotalHACost();
    // cout << "HAP cost = " << HA_cost << endl;
    int opp_cost = ComputeCostNonEligibleOpponents();
    // cout << "Non-eligible opponents cost = " << opp_cost << endl;
    // int same_club_cost = ComputeCostSameClub();
    // cout << "Cost of playing too many times vs team same club = " << same_club_cost << endl;
    int DRR_cost = 0;
    if (!SRR){
        DRR_cost += ComputeCost2RRConstraint();
        // cout << "DRR cost = " << DRR_cost << endl;
    }
    return travel_cost + HA_cost + opp_cost + DRR_cost;
}

int Solution::ComputeTTPViolations(const int i, const int min_round, const int max_round){
    int cost = 0;
    int NrH = 0, NrA = 0;
    const auto& RowOrientation = Orientation[i];
    for (int k = min_round; k <= max_round; ++k){
        if (RowOrientation[k] == HA::H){
            if (++NrH > 3){
                if (ConstraintViolationAllowed){
                    ++cost;
                }
                else{
                    return 1;
                }
            }
            NrA = 0;
        }
        else if (RowOrientation[k] == HA::A){
            if (++NrA > 3){
                if (ConstraintViolationAllowed){
                    ++cost;
                }
                else{
                    return 1;
                }
            }
            NrH = 0;
        }
    }
    if (ConstraintViolationAllowed){
        return cost;
    }
    else{
        return 0;
    }
}

int Solution::ComputeTotalCostTTPViolations(){
    int sum = 0;
    for (int i = 0; i < getNrTeams(); ++i){
        sum += ComputeTTPViolations(i,0,getNrRounds()-1);
    }
    return sum*getCostTTPViolation();
}


int Solution::ComputeTravelCostTeamTTP(const int t)const{
    int CostOfTrips = 0;
    const auto& TeamRow = TeamColorOpp[t];
    const auto& OrientationRow = Orientation[t];

    int PrevOpponent = TeamRow[0];
    HA PrevOrientation = OrientationRow[0];

    if (PrevOrientation == HA::A){
        assert(PrevOpponent != -1);
        CostOfTrips += getDistanceTeams(t,PrevOpponent); // if it plays A in first round, it must also travel to that team (not accounted for in sum above)
        // cout << t << " -> " << TeamColorOpp[t][0] << ": " << getDistanceTeams(t,TeamColorOpp[t][0]) << endl;
    }
    
    int CurrOpponent;
    HA CurrOrientation;
    for (int r = 1; r < NrColouredRounds; ++r){
        CurrOpponent = TeamRow[r];
        CurrOrientation = OrientationRow[r];
        if (PrevOrientation == HA::H && CurrOrientation == HA::A){
            CostOfTrips += getDistanceTeams(t,CurrOpponent);
            // cout << t << " -> " << j << ": " << getDistanceTeams(t,j) << endl;
        }
        else if (PrevOrientation == HA::A && CurrOrientation == HA::H){
            CostOfTrips += getDistanceTeams(t,PrevOpponent);
            // cout << i << " -> " << t << ": " << getDistanceTeams(i,t) << endl;
        }
        else if (PrevOrientation == HA::A && CurrOrientation == HA::A){
            CostOfTrips += getDistanceTeams(PrevOpponent,CurrOpponent);
            // cout << i << " -> " << j << ": " << getDistanceTeams(i,j) << endl;
        }
        PrevOpponent = CurrOpponent;
        PrevOrientation = CurrOrientation;
    }
    
    if (PrevOrientation == HA::A){
        assert(PrevOpponent != -1);
       //  if ((NrColouredRounds == getNrRounds()) || (NrColouredRounds >= 3 && Orientation[t][NrColouredRounds-1] == HA::A && Orientation[t][NrColouredRounds-2] == HA::A && Orientation[t][NrColouredRounds-3] == HA::A)){
            CostOfTrips += getDistanceTeams(t,PrevOpponent); // similar for last round
            // cout << TeamColorOpp[t][NrColouredRounds-1] << " -> " << t << ": " << getDistanceTeams(t,TeamColorOpp[t][NrColouredRounds-1]) << endl;
        // }
    }
    return CostOfTrips;
}

int Solution::ComputeTravelCostTTP(){
    int CostOfTrips = 0;
    for (int t = 0; t < getNrTeams(); ++t){
        // cout << "Cost of trips of " << t << ": " << endl;
        CostOfTrips += ComputeTravelCostTeamTTP(t);
        // cin.get();
    }
    return CostOfTrips;
}

int Solution::ComputeTotalCostTeamTTP(const int i){
    int delta = 0;
    delta += getCostTTPViolation()*ComputeTTPViolations(i,0,getNrRounds()-1);
    if (!ConstraintViolationAllowed && delta >= getCostTTPViolation()){
        return delta;
    }
    else{
        return delta + ComputeTravelCostTeamTTP(i);
    }
}

int Solution::ComputeTotalCostTTP(){
    // cout << "Travel cost = " << ComputeTravelCostTTP() << endl;
    // cout << "TTP cost = " << ComputeTotalCostTTPViolations() << endl;
    return ComputeTravelCostTTP()+ComputeTotalCostTTPViolations();
}

int Solution::ComputeCostGeneralMatrix(){
    int cost = 0;
    int j;
    for (int r = 0; r < NrColouredRounds; ++r){
        vector<bool>NodeSeen(getNrTeams(), false);
        for (int i = 0; i < getNrTeams(); ++i){
            if (!NodeSeen[i]){
                j = TeamColorOpp[i][r];
                if (Orientation[i][r] == HA::H){
                    cost += getCostMatchRound(i,j,r);
                }
                else{
                    cost += getCostMatchRound(j,i,r);
                }
                NodeSeen[j] = true;
            }
        }
    }
    return cost;
}

int Solution::ComputeTotalCost(){
    int cost;
    if (getSetting() == Setting::TTP){
        cost = ComputeTotalCostTTP();
    }
    else if (getSetting() == Setting::Football || getSetting() == Setting::Hockey){
        cost = ComputeTotalCostYSTP();
    }
    else{
        std::cerr << "Unknown setting" << std::endl;
        return -1;
    }
    if (IsBaseAlgo()){
        for (int i = 0; i < getNrTeams(); ++i){
            if (!IsTeamBalanced(i)){
                cost += getCostImbalance();
            }
        }
    }
    return cost;
}

bool Solution::validate(){
    // cout << "Validate solution" << endl;
    int cap_viol = 0;
    vector<int>HomeGamesTeam(getNrTeams(), 0); // counts the nr of H games
    vector<vector<int>>Opponent(getNrTeams(), vector<int>(NrColouredRounds, -1));
    for (int r = 0; r < NrColouredRounds; ++r){
        vector<int>CapacityClub(getNrClubs(), 0);
        vector<bool>NodeSeen(getNrTeams(), false);
        for (int i = 0; i < getNrTeams(); ++i){
            if (NodeSeen[i]){
                continue;
            }
            int j = TeamColorOpp[i][r];
            if (!ViolationEligibleOpponents_allowed){
                if (!isEligible(i,j)){
                    cout << i << " and " << j << " cannot play against each other" << endl;
                    assert(isEligible(i,j));
                }
            }
            assert(TeamColorOpp[j][r] == i);
            Opponent[i][r] = j;
            Opponent[j][r] = i;
            assert(Orientation[j][r] != Orientation[i][r]);
            if (Orientation[i][r] == HA::H){
                if (Orientation[j][r] != HA::A){
                    cout << j << endl;
                }
                if (MatchColor[i][j] != r){
                    cout << "MatchColor[" << i << "][" << j << "] should be " << r << " but is " << MatchColor[i][j] << endl;
                }
                if (SRR){
                   assert(MatchColor[j][i] == r); 
                }
                assert(MatchColor[i][j] == r);
                assert(Orientation[j][r] == HA::A);
                HomeGamesTeam[i]++;
                CapacityClub[getTeamClub(i)]++;
            }
            else{
                if (MatchColor[j][i] != r){
                    cout << "MatchColor[" << j << "][" << i << "] should be " << r << " but is " << MatchColor[j][i] << endl;
                }
                if (SRR){
                   assert(MatchColor[i][j] == r); 
                }
                assert(MatchColor[j][i] == r);
                if (Orientation[j][r] != HA::H){
                    cout << "error for " << i << " vs " << j << endl;
                }
                assert(Orientation[j][r] == HA::H);
                assert(Orientation[i][r] == HA::A);
                HomeGamesTeam[j]++;
                CapacityClub[getTeamClub(j)]++;
            }
            NodeSeen[j] = true;
        }
        for (int c = 0; c < getNrClubs(); ++c){
            // assert(CapacityClub[c] <= getCapacityClub(c));
            cap_viol += max(0, CapacityClub[c] - getCapacityClub(c,r));
            if (!ViolationHAP_allowed){
                assert(cap_viol <= getAllowedNrCapacityViolations());
            }
        }
    }
    for (int i = 0; i < getNrTeams(); ++i){
        vector<int>NrTeamSeen(getNrTeams(), 0);
        vector<int>NrTeamSeenH(getNrTeams(), 0);
        vector<int>NrTeamSeenA(getNrTeams(), 0);
        int nr_same_club = 0;
        int j;
        for (int r = 0; r < NrColouredRounds; ++r){
            j = Opponent[i][r];
            assert(j != -1); // each team plays once in every slot
            assert(j != i);
            if (SRR){
                assert(NrTeamSeen[j] < 1); // each team plays at most once vs the same opponent
            }
            else{
                assert(NrTeamSeen[j] < 2); // case of DRR
            }
            NrTeamSeen[j]++;
            if (Orientation[i][r] == HA::H){
                // Each opponents is seen at most once at H
                assert(NrTeamSeenH[j] < 1);
                NrTeamSeenH[j]++;
            }
            else{
                // Each opponents is seen at most once at A
                assert(NrTeamSeenA[j] < 1);
                NrTeamSeenA[j]++;
            }
            if (getTeamClub(i) == getTeamClub(j)){
                nr_same_club++;
            }
        }
        if (!ViolationSameClubAllowed){
            if (nr_same_club > getMaxSameClub()){
            // cout << i << " plays more against same club than allowed!!" << endl;
                assert(nr_same_club <= getMaxSameClub());
            }
        }
    }

    for (int i = 0; i < getNrTeams(); ++i){
        // cout << "Home games of " <<i << " = " << HomeGamesTeam[i] << endl;
        if (HomeGamesTeam[i] != NrColouredRounds/2){
            cout << i << " has " << HomeGamesTeam[i] << " home games" << endl;
        }
        assert(HomeGamesTeam[i] == NrColouredRounds/2);
    }
    if (!ViolationHAP_allowed){
        assert(ComputeTotalHACost() == 0);
    }

    return true;
}


void Solution::clear(){
    for (int i = 0; i < getNrTeams(); ++i){
        for (int j = i; j < getNrTeams(); ++j){
            MatchColor[i][j] = -1;
            MatchColor[j][i] = -1;
        }
        for (int r = 0; r < getNrRounds(); ++r){
            TeamColorOpp[i][r] = -1;
            Orientation[i][r] = HA::BYE;
        }
    }
}
