#include "Solution.h"
#include <iostream>
#include <assert.h>

Solution::Solution(const Input& in) : Input(in) {

    int N = getNrTeams();
    int R = getNrRounds();

    MatchColor = vector<vector<int>>(N, vector<int>(N, -1)); // diagonals should stay -1, if a team does not play it is also -1
    TeamColorOpp = vector<vector<int>>(N, vector<int>(R, -1));
    WeightsBF = vector<vector<int>>(N+1, vector<int>(N+1, N+1)); // N+1 bc of extra source node!!
    Orientation = vector<vector<HA>>(N, vector<HA>(R, HA::BYE));
}
Solution::~Solution(){}

int Solution::getNrBreaks(const int i){
    int NrBreaks = 0;
    for (int c = 1; c < Orientation[i].size(); ++c){
        if (Orientation[i][c-1] == Orientation[i][c]){
            NrBreaks++;
        }
    }
    return NrBreaks;
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
    if (Orientation[i][getNrRounds()-1] == Orientation[i][getNrRounds()-2]){
        NrBreaks++;
    }
    return NrBreaks;
}

int Solution::getImbalanceHalf(const int i){
    int cost = 0;
    int NrH_half;
    const int Half = getNrRounds()/2;
    const vector<pair<int,int>>Halves = {{0, Half}, {Half, getNrRounds()}};
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

int Solution::getNrSameClub(const int i){
    int j;
    int nr = 0;
    for (int r = 0; r < getNrRounds(); ++r){
        j = TeamColorOpp[i][r];
        if (j != -1 && getTeamClub(j) == getTeamClub(i)){
            ++nr;
        }
    }
    return nr;
}

int Solution::ComputeCost2RRConstraint(){
    int H = getNrRounds()/2;
    int cost = 0;
    for (int i = 0; i < getNrTeams(); ++i){
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
        for (int r = H; r < getNrRounds(); ++r){
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
    }
    return cost;
}


int Solution::ComputeCostNonEligibleOpponents(){
    int cost = 0;
    for (int c = 0; c < getNrRounds(); c++){
        vector<bool>TeamSeen(getNrTeams(), false);
        for (int i = 0; i < getNrTeams(); i++){
            if (TeamSeen[i]){
                continue;
            }
            if (!isEligible(i, TeamColorOpp[i][c])){
                cost++;
            }
            TeamSeen[i] = true;
            TeamSeen[TeamColorOpp[i][c]] = true;
        }
    }
    return NonEligibleCost*cost;
}

int Solution::ComputeCostSameClub(){
    int NrSameClub;
    for (int i = 0; i < getNrTeams(); ++i){
        NrSameClub = getNrSameClub(i);
    }
    return CostSameClub*max(0, NrSameClub - getMaxSameClub());
}

int Solution::ComputeTravelCost(){
    int cost = 0;
    for (int c = 0; c < getNrRounds(); c++){
        /*
        cout << "------------" << endl;
        cout << "Round " << c << endl;
        cout << "------------" << endl;
        */
        vector<bool>TeamSeen(getNrTeams(), false);
        for (int i = 0; i < getNrTeams(); i++){
            if (TeamSeen[i]){
                continue;
            }
            cost += getDistanceTeams(i, TeamColorOpp[i][c]);
            TeamSeen[i] = true;
            TeamSeen[TeamColorOpp[i][c]] = true;
            /*
            if (G->Orientation[i][c] == HA::H){
                assert(G->Orientation[G->TeamColorOpp[i][c]][c] == HA::A);
                cout << i << ", " << G->TeamColorOpp[i][c] << " = " << getDistanceTeams(i, G->TeamColorOpp[i][c]) << endl;
            }
            else{
                assert(G->Orientation[G->TeamColorOpp[i][c]][c] == HA::H);
                cout << G->TeamColorOpp[i][c] << ", " << i << " = " << getDistanceTeams(i, G->TeamColorOpp[i][c]) << endl;
            }
                */
        }
    }
    return TravelCost*cost;
}

int Solution::ComputeCapacityClubRound(const int c, const int r){
    int cap = 0;
    int l;
    for (auto& i: getTeamsClub(c)){
        if (Orientation[i][r] == HA::H){
            cap++;
        }
    }
    return cap;
}

int Solution::ComputeCostCapacities(){
    int cost = 0;
    int cap;
    for (int c = 0; c < getNrClubs(); ++c){
        for (int r = 0; r < getNrRounds(); ++r){
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

    return CostCapacityViol*cost;
}

int Solution::ComputeHACostTeam(const int i){
    int cost = 0;
    if (getHAP_requirement(HAP_requirement_name::NoThreeConsecutive)){
        cost += NrThreeConsecutiveHA(i);
        // assert(NrThreeConsecutiveHA(i) == 0);
    }
    if (getHAP_requirement(HAP_requirement_name::NoBreakBeginningEnd)){
        cost += getNrBreaksBeginningEnd(i);
        // assert(getNrBreaksBeginningEnd(i) == 0);
    }
    if (getHAP_requirement(HAP_requirement_name::BreakLimit)){
        cost += max(0, getNrBreaks(i)-getBreakLimit());
        // assert(getNrBreaks(i) <= getBreakLimit());
    }
    if (getHAP_requirement(HAP_requirement_name::QuarterBalanced)){
        cost += getImbalanceHalf(i);
        // assert(getImbalanceHalf(i) == 0);
    }
    return HighCostHAPs*cost;
}

int Solution::ComputeTotalHACost(){
    int cost = ComputeCostCapacities();
    for (int i = 0; i < getNrTeams(); ++i){
        cost += ComputeHACostTeam(i);
    }
    return cost;
}

int Solution::ComputeTotalCost(){
    int travel_cost = ComputeTravelCost();
    // cout << "Travel cost = " << travel_cost << endl;
    int HA_cost = ComputeTotalHACost();
    // cout << "HAP cost = " << HA_cost << endl;
    int opp_cost = ComputeCostNonEligibleOpponents();
    // cout << "Non-eligible opponents cost = " << opp_cost << endl;
    int same_club_cost = ComputeCostSameClub();
    // cout << "Cost of playing too many times vs team same club = " << same_club_cost << endl;
    int DRR_cost = 0;
    if (!SRR){
        DRR_cost += ComputeCost2RRConstraint();
        // cout << "DRR cost = " << DRR_cost << endl;
    }
    return travel_cost + HA_cost + opp_cost + same_club_cost + DRR_cost;
}

void Solution::validate(){
    // cout << "Validate solution" << endl;
    int cap_viol = 0;
    vector<int>HomeGamesTeam(getNrTeams(), 0); // counts the nr of H games
    vector<vector<int>>Opponent(getNrTeams(), vector<int>(getNrRounds(), -1));
    for (int r = 0; r < getNrRounds(); ++r){
        vector<int>CapacityClub(getNrClubs(), 0);
        vector<bool>NodeSeen(getNrTeams(), false);
        for (int i = 0; i < getNrTeams(); ++i){
            if (NodeSeen[i]){
                continue;
            }
            int j = TeamColorOpp[i][r];
            if (!ViolationEligibleOpponents_allowed){
                assert(isEligible(i,j));
            }
            assert(TeamColorOpp[j][r] == i);
            Opponent[i][r] = j;
            Opponent[j][r] = i;
            if (Orientation[i][r] == HA::H){
                if (Orientation[j][r] != HA::A){
                    cout << j << endl;
                }
                if (MatchColor[i][j] != r){
                    cout << "MatchColor[" << i << "][" << j << "] should be " << r << " but is " << MatchColor[i][j] << endl;
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
                assert(MatchColor[j][i] == r);
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
                assert(cap_viol == 0);
            }
        }
    }
    for (int i = 0; i < getNrTeams(); ++i){
        vector<int>NrTeamSeen(getNrTeams(), 0);
        vector<int>NrTeamSeenH(getNrTeams(), 0);
        vector<int>NrTeamSeenA(getNrTeams(), 0);
        int nr_same_club = 0;
        int j;
        for (int r = 0; r < getNrRounds(); ++r){
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
        assert(nr_same_club <= getMaxSameClub());
    }

    for (int i = 0; i < getNrTeams(); ++i){
        // cout << "Home games of " <<i << " = " << HomeGamesTeam[i] << endl;
        if (HomeGamesTeam[i] != getNrRounds()/2){
            cout << i << " has " << HomeGamesTeam[i] << " home games" << endl;
        }
        assert(HomeGamesTeam[i] == getNrRounds()/2);
    }
    if (!ViolationHAP_allowed){
        assert(ComputeTotalHACost() == 0);
    }
}