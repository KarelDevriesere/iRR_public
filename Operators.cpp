#include "Operators.h"
#include <queue> // for DFS

using namespace std;

// Deltas:
// ********************************************************************************************************************************* //

// ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP //

int Operator::getLocSwapped(const int i, const int round) {
    if (round < 0 || round >= sol.getNrRounds()){
        // Dist[i][i] = 0!
        return i; 
    }
    else if (sol.Orientation[i][round] == HA::H){
        return sol.TeamColorOpp[i][round];
        // Orientation swap so if it plays home, now it plays away 
    }
    else{
        return i;
    }
};

// Helper to get location of team i in any round (handles home city)
int Operator::getLoc(const int i, const int round) {
    if (round < 0 || round >= sol.getNrRounds() || sol.Orientation[i][round] == HA::H){
        // Dist[i][i] = 0!
        return i; 
    }
    else{
        return sol.TeamColorOpp[i][round];
    }
};

int Operator::DeltaTravelOrientationSwapTeam(const int i, const int r, const int s) {
    // Ensure r is the earlier round for easier logic
    int MinRound = std::min(r, s);
    int MaxRound = std::max(r, s);
    
    int delta = 0;

    int L_r_prev = getLoc(i, MinRound - 1);
    int L_r      = getLoc(i, MinRound);
    int L_r_next = getLoc(i, MinRound + 1);

    int L_s_prev = getLoc(i, MaxRound - 1);
    int L_s      = getLoc(i, MaxRound);
    int L_s_next = getLoc(i, MaxRound + 1);

    int L_r_swp = getLocSwapped(i, MinRound);
    int L_s_swp = getLocSwapped(i, MaxRound);

    if (MaxRound == MinRound + 1) {
        // --- CASE 1: ADJACENT SWAP ---
        // Old legs
        delta -= sol.getDistanceTeams(L_r_prev, L_r);
        delta -= sol.getDistanceTeams(L_r, L_s);
        delta -= sol.getDistanceTeams(L_s, L_s_next);
        // New legs
        delta += sol.getDistanceTeams(L_r_prev, L_r_swp);
        delta += sol.getDistanceTeams(L_r_swp, L_s_swp);
        delta += sol.getDistanceTeams(L_s_swp, L_s_next);
    } 
    else {
        // --- CASE 2: NON-ADJACENT SWAP ---
        // Change around Round R
        delta -= sol.getDistanceTeams(L_r_prev, L_r);
        delta -= sol.getDistanceTeams(L_r, L_r_next);
        delta += sol.getDistanceTeams(L_r_prev, L_r_swp);
        delta += sol.getDistanceTeams(L_r_swp, L_r_next);

        // Change around Round S
        delta -= sol.getDistanceTeams(L_s_prev, L_s);
        delta -= sol.getDistanceTeams(L_s, L_s_next);
        delta += sol.getDistanceTeams(L_s_prev, L_s_swp);
        delta += sol.getDistanceTeams(L_s_swp, L_s_next);
    }

    return delta;
}

int Operator::DeltaTravelRoundSwapTeam(const int i, const int r, const int s) {
    // Ensure r is the earlier round for easier logic
    int MinRound = std::min(r, s);
    int MaxRound = std::max(r, s);
    
    int delta = 0;

    int L_r_prev = getLoc(i, MinRound - 1);
    int L_r      = getLoc(i, MinRound);
    int L_r_next = getLoc(i, MinRound + 1);

    int L_s_prev = getLoc(i, MaxRound - 1);
    int L_s      = getLoc(i, MaxRound);
    int L_s_next = getLoc(i, MaxRound + 1);

    if (MaxRound == MinRound + 1) {
        // --- CASE 1: ADJACENT SWAP ---
        // Old legs
        delta -= sol.getDistanceTeams(L_r_prev, L_r);
        delta -= sol.getDistanceTeams(L_r, L_s);
        delta -= sol.getDistanceTeams(L_s, L_s_next);
        // New legs
        delta += sol.getDistanceTeams(L_r_prev, L_s);
        delta += sol.getDistanceTeams(L_s, L_r);
        delta += sol.getDistanceTeams(L_r, L_s_next);
    } 
    else {
        // --- CASE 2: NON-ADJACENT SWAP ---
        // Change around Round R
        delta -= sol.getDistanceTeams(L_r_prev, L_r);
        delta -= sol.getDistanceTeams(L_r, L_r_next);
        delta += sol.getDistanceTeams(L_r_prev, L_s);
        delta += sol.getDistanceTeams(L_s, L_r_next);

        // Change around Round S
        delta -= sol.getDistanceTeams(L_s_prev, L_s);
        delta -= sol.getDistanceTeams(L_s, L_s_next);
        delta += sol.getDistanceTeams(L_s_prev, L_r);
        delta += sol.getDistanceTeams(L_r, L_s_next);
    }

    return delta;
}


int Operator::CostNrConsecutiveHA(const int i, const int min_round, const int max_round){
    int cost = 0;
    int NrH = 0, NrA = 0;
    for (int k = min_round; k <= max_round; ++k){
        if (sol.Orientation[i][k] == HA::H){
            NrH++;
            NrA = 0;
        }
        else if (sol.Orientation[i][k] == HA::A){
            NrA++;
            NrH = 0;
        }
        if (NrH > 3 || NrA > 3){
            cost += sol.getCostTTPViolation();
        }
    }
    return cost;
}

int Operator::DeltaHACostOrientationSwapTeam(const int i, const int r, const int s){
    int delta = 0;
    int min_round, max_round;

    if (sol.Orientation[i][r] == sol.Orientation[i][s]){
        return delta;
    }

    for (int q = 0; q < 2; ++q){
        if (abs(r-s) >= 4){
            for (int z: {r,s}){
                min_round = std::max(0, z-3);
                max_round = std::min(R-1, z+3);
                if (q == 0){
                    delta -= CostNrConsecutiveHA(i, min_round, max_round);
                }
                else{
                    delta += CostNrConsecutiveHA(i, min_round, max_round);
                }
            }
        }
        else{
            min_round = std::min(r,s);
            min_round = std::max(0, min_round-3);
            max_round = std::max(r,s);
            max_round = std::min(R-1, max_round+3);
            if (q == 0){
                delta -= CostNrConsecutiveHA(i, min_round, max_round);
            }
            else{
                delta += CostNrConsecutiveHA(i, min_round, max_round);
            }
        }
        std::swap(sol.Orientation[i][r], sol.Orientation[i][s]);
    }
    return delta;
}

int Operator::CostOrientationSwapTeamiTTP(const int i, const int r, const int s){

    assert(r > -1 && s > -1);

    // Use this for CR and PR

    // Travel distance:
    
    int delta_travel = DeltaTravelOrientationSwapTeam(i, r, s);

    // HA cost:

    int delta_HA = DeltaHACostOrientationSwapTeam(i, r, s);
    

    // cout << "delta_HA = " << delta_HA << ", delta_travel = " << delta_travel << endl;

    return delta_travel + delta_HA;
}

int Operator::CostRoundSwapTeamiTTP(const int i, const int r, const int s){

    assert(r > -1 && s > -1);

    // Use this for RS, PRS, TS

    // Travel distance:
    
    int delta_travel = DeltaTravelRoundSwapTeam(i, r, s);

    // HA cost:

    int delta_HA = DeltaHACostOrientationSwapTeam(i, r, s);

    // cout << "delta_HA = " << delta_HA << ", delta_travel = " << delta_travel << endl;
    // cout << "New cost team " << i << " : " << sol.ComputeTotalCostTeamTTP(i)+delta_HA+delta_travel << endl;

    return delta_travel+delta_HA;
}

int Operator::CostUncoloredRoundSwapTeamiTTP(const int i, const int UncoloredOpponent_i, const int r, const int s){
    int delta = 0;
    // Exactly one must be uncolored and one must be colored!

    assert((r == -1 || s == -1) && !(r == -1 && s == -1));

    int f,c;
    if (r == -1){
        f=r;
        c=s;
    }
    else{
        f=s;
        c=r;
    }

    if (sol.Orientation[i][c] == HA::H){
        return delta;
    }

    int L_c_prev = getLoc(i, c - 1);
    int L_c = getLoc(i, c);
    int L_c_next = getLoc(i, c + 1);

    delta -= (sol.getDistanceTeams(L_c_prev, L_c) + sol.getDistanceTeams(L_c, L_c_next));
    // cout << "initial travel for team " << i << ": " << L_c_prev << " -> (" << sol.getDistanceTeams(L_c_prev, L_c) << ")" << L_c << " -> (" << sol.getDistanceTeams(L_c, L_c_next) << ")" << L_c_next << endl;
    delta += (sol.getDistanceTeams(L_c_prev, UncoloredOpponent_i) + sol.getDistanceTeams(UncoloredOpponent_i, L_c_next));
    // cout << "new travel for team " << i << ": " << L_c_prev << " -> (" << sol.getDistanceTeams(L_c_prev, UncoloredOpponent_i) << ")" << UncoloredOpponent_i << " -> (" << sol.getDistanceTeams(UncoloredOpponent_i, L_c_next) << ")" << L_c_next << endl;

    // No TTP violations in case of one uncolored edge

    return delta; 
}

int Operator::CostTSTeamsTTP(const int i, const int j){
    // cout << "Costs after delta computation" << endl;
    // cout << "i = " << i << ", j = " << j << endl;
    int delta = 0;
    int k,k2;
    for (int t: {i,j}){
        for (int r = 0; r < R; ++r){
            int delta_k = 0;
            k = sol.TeamColorOpp[t][r];
            if (t == j && sol.MatchColor[k][i] > -1){
                continue;
            }
            if (k == i || k == j){
                // cout << i << " and " << j << " face each other!!" << endl;
                continue;
            }
            else if (t ==i && sol.MatchColor[k][j] > -1){
                /*
                This is unique for TS: the HAPs are changed with the colors!!
                This ensues that we do not break anything for the middle teams
                i receives the HAP of j and visa versa, so also here nothing breaks
                */
                int s = sol.MatchColor[k][j];
                if (abs(r-s) > 1){
                    if (sol.Orientation[k][r] == HA::A){
                        int L_r_prev = getLoc(k,r-1);
                        int L_r = getLoc(k,r);
                        int L_r_next = getLoc(k,r+1);

                        delta_k -= (sol.getDistanceTeams(L_r_prev, L_r) + sol.getDistanceTeams(L_r, L_r_next));
                        delta_k += (sol.getDistanceTeams(L_r_prev, j) + sol.getDistanceTeams(j, L_r_next));
                    }

                    if (sol.Orientation[k][s] == HA::A){
                        int L_s_prev = getLoc(k,s-1);
                        int L_s = getLoc(k,s);
                        int L_s_next = getLoc(k,s+1);

                        delta_k -= (sol.getDistanceTeams(L_s_prev, L_s) + sol.getDistanceTeams(L_s, L_s_next));
                        delta_k += (sol.getDistanceTeams(L_s_prev, i) + sol.getDistanceTeams(i, L_s_next));
                    }
                }
                else{
                    int min_round = std::min(r,s);
                    int min_team, max_team;
                    if (min_round == r){
                        min_team = i, max_team = j;
                    }
                    else{
                        min_team = j, max_team = i;
                    }
                    int L_r_prev = getLoc(k,min_round-1);
                    int L_r = getLoc(k,min_round);
                    int L_r_next = getLoc(k,min_round+1);
                    int L_r_next2 = getLoc(k,min_round+2);

                    delta_k -= (sol.getDistanceTeams(L_r_prev, L_r) + sol.getDistanceTeams(L_r, L_r_next) + sol.getDistanceTeams(L_r_next, L_r_next2));

                    if (sol.Orientation[k][min_round] == HA::A){
                        L_r = max_team;
                    }
                    if (sol.Orientation[k][min_round+1] == HA::A){
                        L_r_next = min_team;
                    }
                    delta_k += (sol.getDistanceTeams(L_r_prev, L_r) + sol.getDistanceTeams(L_r, L_r_next) + sol.getDistanceTeams(L_r_next, L_r_next2));
                }
                // cout << "New travel cost of " << k << " (adjacent to 2 colored edges) = " << sol.ComputeTravelCostTeamTTP(k)+delta_k << endl;
            }
            else if (t == i){
                delta_k += CostUncoloredRoundSwapTeamiTTP(k, j, r, sol.MatchColor[k][j]);
                // cout << "New travel cost of " << k << " (adjacent to one uncolored edge) = " << sol.ComputeTravelCostTeamTTP(k)+delta_k << endl;
            }
            else if (t == j && sol.MatchColor[k][i] == -1){
                delta_k += CostUncoloredRoundSwapTeamiTTP(k, i, r, sol.MatchColor[k][i]);
                // cout << "New travel cost of " << k << " (adjacent to one uncolored edge)  = " << sol.ComputeTravelCostTeamTTP(k)+delta_k << endl;
            }
            delta += delta_k;
        }
    }
    // subtract costs for teams i and j:
    delta -= (sol.ComputeTravelCostTeamTTP(i) + sol.ComputeTravelCostTeamTTP(j));

    // deltas i and j:
    array<int,2>pair = {i,j};
    int t_opp;
    for (int t = 0; t < 2; ++t){
        t_opp = (t+1)%2;
        k = getLoc(pair[t_opp],0);
        if (k == pair[t_opp]){
            k = pair[t];
        }
        else if (k == pair[t]){
            k = pair[t_opp];
        }
        delta += sol.getDistanceTeams(pair[t],k);
        for (int r = 1; r < R; ++r){
            k2 = getLoc(pair[t_opp],r);
            if (k2 == pair[t_opp]){
                k2 = pair[t];
            }
            else if (k2 == pair[t]){
                k2 = pair[t_opp];
            }
            delta += sol.getDistanceTeams(k,k2); 
            k = k2;
        }
        delta += sol.getDistanceTeams(k, pair[t]);
    }

    return delta;
}

int Operator::PTSCurrentTravelDelta(const vector<int>& SortedRoundsLantern, const int t){
    int delta = 0;
    bool skip = false;
    int L, L_next;
    for (int i = 0; i < SortedRoundsLantern.size(); ++i){
        L = getLoc(t,SortedRoundsLantern[i]);
        if (!skip){
            delta += (sol.getDistanceTeams(getLoc(t,SortedRoundsLantern[i]-1),L));
        }
        if (i < SortedRoundsLantern.size()-1 && SortedRoundsLantern[i+1] == SortedRoundsLantern[i]+1){
            L_next = getLoc(t,SortedRoundsLantern[i+1]);
            skip = true;
        }
        else{
            L_next = getLoc(t,SortedRoundsLantern[i]+1);
            skip = false;
        }
        delta += (sol.getDistanceTeams(L,L_next));
    }
    return delta;
}

int Operator::DeltaPRS_TTP(const int r, const int s, const int StartNode){
    // std::cout << "Partially swap rounds " << r << " and " << s << std::endl;
    // r and s are always real colors!!
    // The only way that something changes for PRS when doing 2RR is when a node goes back to another node before completing the cycle
    // But since any previous node except for the StartNode already has 2 neighbors with the colors r and s, this is not possible
    // What is possible is if we get a cycle directly in the beginning -> we don't need to change anything in the code
    // This move can be imagined as a cycle in which the colors of the edges are swapped but the orientations remain the same
    // This guarantees that the HAPs stay balanced. However, the HAPs of all the teams involved may break
    int next = StartNode;
    int delta = 0;
    do{
        delta += CostRoundSwapTeamiTTP(next, r, s);
        // std::cout << "t: " << next << std::endl;
        next = sol.TeamColorOpp[next][s];
        delta += CostRoundSwapTeamiTTP(next, r, s);
        next = sol.TeamColorOpp[next][r];
    }
    while (next != StartNode);
    return delta;
}

// ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP ----- TTP //


// ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP //

int Operator::DeltaPRS_YSTP(const int r, const int s, const int StartNode){
    int next = StartNode;
    int delta = 0;
    const int C = sol.getNrClubs();
    clearClubSeen();
    do{
        if (sol.Orientation[next][r] != sol.Orientation[next][s]){
            std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
            delta += sol.ComputeHACostTeam(next);
            ClubSeen[sol.getTeamClub(next)] = 1;
        }
        next = sol.TeamColorOpp[next][s];
        if (sol.Orientation[next][r] != sol.Orientation[next][s]){
            std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
            delta += sol.ComputeHACostTeam(next);
            ClubSeen[sol.getTeamClub(next)] = 1;
        }
        next = sol.TeamColorOpp[next][r];
    }
    while (next != StartNode);

    for (int c = 0; c < C; ++c){
        if (ClubSeen[c]){
            if (sol.ComputeCapacityClubRound(c,r) > sol.getCapacityClub(c,r)){
                delta += sol.CostCapacityViol;
            }
            if (sol.ComputeCapacityClubRound(c,s) > sol.getCapacityClub(c,s)){
                delta += sol.CostCapacityViol;
            }
        }
    }

    do{
        if (sol.Orientation[next][r] != sol.Orientation[next][s]){
            std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
        }
        next = sol.TeamColorOpp[next][s];
        if (sol.Orientation[next][r] != sol.Orientation[next][s]){
            std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
        }
        next = sol.TeamColorOpp[next][r];
    }
    while (next != StartNode);

    return delta;
}

int Operator::CostTSTeamsYSTP(const int i, const int j){
    // Travel part:
    int delta_travel = 0;
    int r, opp_i, opp_j;
    int R = sol.getNrRounds();
    for (r = 0; r < R; ++r){
        opp_i = sol.TeamColorOpp[i][r], opp_j = sol.TeamColorOpp[j][r];
        delta_travel -= (sol.getDistanceTeams(i, opp_i) + sol.getDistanceTeams(j, opp_j));
        delta_travel += (sol.getDistanceTeams(i, opp_j) + sol.getDistanceTeams(j, opp_i));
        if (!sol.isEligible(i,opp_j)){
            delta_travel += sol.NonEligibleCost;
        }
        if (!sol.isEligible(j,opp_i)){
            delta_travel += sol.NonEligibleCost;
        }
    }

    // HA part:
    int delta_HA = 0;
    int c_i = sol.getTeamClub(i), c_j = sol.getTeamClub(j);
    if (c_i != c_j){
        for (r = 0; r < R; ++r){
            if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A){
                if (sol.ComputeCapacityClubRound(c_j,r) + 1 > sol.getCapacityClub(c_j,r)){
                    delta_HA += sol.CostCapacityViol;
                }
            }
            else if (sol.Orientation[i][r] == HA::A && sol.Orientation[j][r] == HA::H){
                if (sol.ComputeCapacityClubRound(c_i,r) + 1 > sol.getCapacityClub(c_i,r)){
                    delta_HA += sol.CostCapacityViol;
                }
            }
        }
    }
    return delta_travel + delta_HA;
}

// ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP ----- YSTP //

// ********************************************************************************************************************************* //
// end Deltas


void Operator::PrintEdgeLantarn(const int i, const int k, const int j){
    // k: position in middle, not the team itself!!!
    int c_i = sol.MatchColor[i][k];
    int c_j = sol.MatchColor[j][k];
    cout << i;
    if (c_i < 0){
        cout << "-" << c_i << "-";
    }
    else if (sol.Orientation[i][c_i] == HA::H){
        cout << "<-" << c_i << "-";
    }
    else{
        cout << "-" << c_i << "->";
    }
    cout << lantarn.middle[k];
    if (c_j < 0){
        cout << "-" << c_j << "-";
    }
    else if (sol.Orientation[j][c_j] == HA::H){
        cout << "-" << c_j << "->";
    }
    else{
        cout << "<-" << c_j << "-";
    }
    cout << j << endl;
}

void Operator::PrintLantarn(){
    int i = lantarn.i;
    int j = lantarn.j;
    for (int k = 0; k < lantarn.middle.size(); ++k){
        PrintEdgeLantarn(i, k, j);
    }
}

// Another option is to swap all the colors: the HAPs of the middle teams do not change, only of i and j
// Next, we find paths to restore the balance but we use the trick 

void Operator::setMatchColorR(const int i, const int r, const int s){
    // must always go before swap!!
    int opp_r = sol.TeamColorOpp[i][r];
    int opp_s = sol.TeamColorOpp[i][s];
    if (!sol.SRR){
        if (sol.Orientation[i][r] == HA::H){
            sol.MatchColor[i][opp_r] = s;
        }
        else{
            sol.MatchColor[opp_r][i] = s;
        }
        if (sol.Orientation[i][s] == HA::H){
            sol.MatchColor[i][opp_s] = r;
        }
        else{
            sol.MatchColor[opp_s][i] = r;
        }
    }
    else{
        sol.MatchColor[i][opp_r] = s;
        sol.MatchColor[i][opp_s] = r;
        sol.MatchColor[opp_r][i] = s;
        sol.MatchColor[opp_s][i] = r;
    }   
}


void Operator::TS(const int i, const int j){
    // Swaps 2 teams
    // std::cout << "Swap teams " << i << " and " << j << std::endl;
    // ALWAYS KEEPS THE BALANCE!!
    // This moves works by swapping the colors on both sides. The orienations are such that the middle teams
    // keep their orientations in each color, hence only the HAPs of i and j change!!
    // Because we swap all the colors and orientations, including i and j
    // the resulting HAPs of i and j will still be balanced
    // However, the HAPs of i and j might cause conflicts with capacities

    int c, opp_i, opp_j;
    for (c = 0; c < R; ++c){
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);

        if (sol.TeamColorOpp[i][c] != j){

            opp_i = sol.TeamColorOpp[i][c];
            opp_j = sol.TeamColorOpp[j][c];
            std::swap(sol.TeamColorOpp[i][c], sol.TeamColorOpp[j][c]);

            sol.TeamColorOpp[opp_i][c] = j; // opp of i now has j as opp in c
            sol.TeamColorOpp[opp_j][c] = i;
        }

        /*
        string dir_i = "H";
        if (sol.Orientation[i][c] == HA::A){
            dir_i = "A";
        }
        string dir_j = "H";
        if (sol.Orientation[j][c] == HA::A){
            dir_j = "A";
        }
        cout << i << " plays " << dir_i << " in " << c << " vs " << sol.TeamColorOpp[i][c] << endl;
        cout << j << " plays " << dir_j << " in " << c << " vs " << sol.TeamColorOpp[j][c]  << endl;
        */
    }

    for (int k = 0; k < N; ++k){
        if (i == k || j == k){
            continue;
        }
        std::swap(sol.MatchColor[i][k], sol.MatchColor[j][k]);
        std::swap(sol.MatchColor[k][i], sol.MatchColor[k][j]);
        if (sol.getSetting() != Setting::Football){
            assert(sol.ComputeHACostTeam(k) == 0); // test assumption that the HAPs of the middle teams do not change
            // For Miao instances, we do allow HAP violations
        }
    }
    std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]); // do this to make the HAPs of i and j balanced!
}

void Operator::RS(const int r, const int s){
    // std::cout << "Partially swap rounds " << r << " and " << s << std::endl;
    // r and s are always real colors!!
    for (int i = 0; i < N; ++i){
        if (!sol.SRR){
            setMatchColorR(i, r, s);
        }
        else{
            // otherwise we count double!!
            // i.e. in setMatchColorR we do both (i,j) and (j,i)
            std::swap(sol.MatchColor[i][sol.TeamColorOpp[i][r]], sol.MatchColor[i][sol.TeamColorOpp[i][s]]);
        }
        std::swap(sol.TeamColorOpp[i][r], sol.TeamColorOpp[i][s]);
        std::swap(sol.Orientation[i][r], sol.Orientation[i][s]);
    }
}

void Operator::PRS(const int r, const int s, const int StartNode){
    // std::cout << "Partially swap rounds " << r << " and " << s << std::endl;
    // r and s are always real colors!!
    // The only way that something changes for PRS when doing 2RR is when a node goes back to another node before completing the cycle
    // But since any previous node except for the StartNode already has 2 neighbors with the colors r and s, this is not possible
    // What is possible is if we get a cycle directly in the beginning -> we don't need to change anything in the code
    // This move can be imagined as a cycle in which the colors of the edges are swapped but the orientations remain the same
    // This guarantees that the HAPs stay balanced. However, the HAPs of all the teams involved may break
    int next = StartNode;
    do{
        // std::cout << "t: " << next << std::endl;
        setMatchColorR(next, r, s);
        std::swap(sol.TeamColorOpp[next][r], sol.TeamColorOpp[next][s]);
        std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
        next = sol.TeamColorOpp[next][s];
        // std::cout << "t2: " << next << std::endl;
        setMatchColorR(next, r, s);
        std::swap(sol.TeamColorOpp[next][s], sol.TeamColorOpp[next][r]);
        std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
        next = sol.TeamColorOpp[next][r];
    }
    while (next != StartNode);
}

void Operator::SwapColorsLantarn(vector<HA>& OrientationCopy_i, vector<HA>& OrientationCopy_j){

    std::fill(OrientationCopy_i.begin(), OrientationCopy_i.end(), HA::BYE);
    std::fill(OrientationCopy_j.begin(), OrientationCopy_j.end(), HA::BYE);

    int i = lantarn.i;
    int j = lantarn.j;
    for (int k = 0; k < lantarn.middle.size(); ++k){

        int k_ = lantarn.middle[k];

        int c_i = sol.MatchColor[i][k_];
        int c_j = sol.MatchColor[j][k_];

        if (c_i > -1 && c_j > -1){
            std::swap(sol.Orientation[k_][c_i], sol.Orientation[k_][c_j]);
            OrientationCopy_i[c_j] = sol.Orientation[i][c_i];
            OrientationCopy_j[c_i] = sol.Orientation[j][c_j];
        }
        else if (c_j < 0){
            OrientationCopy_j[c_i] = sol.Orientation[i][c_i];
        }
        else {
            OrientationCopy_i[c_j] = sol.Orientation[j][c_j];
        }
        
        sol.MatchColor[i][k_] = c_j;
        sol.MatchColor[k_][i] = c_j;
        sol.MatchColor[j][k_] = c_i;
        sol.MatchColor[k_][j] = c_i;

        if (c_i >= 0){
            std::swap(sol.TeamColorOpp[i][c_i], sol.TeamColorOpp[j][c_i]);
        }

        if (c_i < 0){
            sol.TeamColorOpp[k_][c_j] = i;
        }
        else if (c_j < 0){
            sol.TeamColorOpp[k_][c_i] = j;
        }
        else{
            std::swap(sol.TeamColorOpp[k_][c_i], sol.TeamColorOpp[k_][c_j]);
        }  
        // PrintEdgeLantarn(G, i, k, j);
    }

    for (int r = 0; r < R; ++r){
        if (OrientationCopy_i[r] != HA::BYE){
            sol.Orientation[i][r] = OrientationCopy_i[r];
        }
        if (OrientationCopy_j[r] != HA::BYE){
            sol.Orientation[j][r] = OrientationCopy_j[r];
        }
    }
}

void Operator::ReplenishLantarn(const int i, const int j, const int StartColor, int& delta){
    assert(StartColor >= 0);
    assert(i != j);
    int c_i = -1, c_j = StartColor;
    int k; 
    bool InfeasibleColorFound = false;
    do{
        c_i = c_j;
        // std::cout << "c_i = " << c_i << std::endl;
        k = sol.TeamColorOpp[i][c_i]; 
        assert(i != k);
        assert(j != k);

        lantarn.middle.push_back(k);
        // std::cout << "k = " << k << std::endl;
        c_j = sol.MatchColor[k][j];

        // delta computation:
        if (c_j > -1){
            delta += CostRoundSwapTeamiTTP(k, c_i, c_j); 
        }
        else{
            delta += CostUncoloredRoundSwapTeamiTTP(k, j, c_i, c_j);
        }

        assert(c_i != c_j);
        // std::cout << "c_j = " << c_j << std::endl;

        if (c_j < 0){
            InfeasibleColorFound = true;
            lantarn.InfeasibleColor = true;
            lantarn.c_[i] = c_i;
            if (!sol.isEligible(j,k)){
                lantarn.InfeasibleOpponents = true;
            }
        }
    }
    while(c_j != StartColor && !InfeasibleColorFound);
}

void Operator::CreateLantarn(const int i, const int j, const int StartColor, int& delta){
    assert(StartColor >= 0);
    lantarn.reset(i,j);
    // cout << "i = " << i << ", j = " << j << endl;
    ReplenishLantarn(i, j, StartColor, delta);
    if (lantarn.InfeasibleColor /*&& !lantarn.InfeasibleOpponents*/){
        if (sol.ViolationEligibleOpponents_allowed || !lantarn.InfeasibleOpponents){
            ReplenishLantarn(j, i, StartColor, delta);
        }
        else{
            return; // garbage lantarn, check outside if lantarn contains infeasible opponents!
        }
    }
    if (lantarn.InfeasibleColor && (sol.Orientation[i][lantarn.c_[i]] != sol.Orientation[j][lantarn.c_[j]])){
        lantarn.PathReversalNeeded = true;
    }
    else{
        lantarn.PathReversalNeeded = false;
    }
    return;
}


bool Operator::RepairOrientationsEdgesLantarn(const bool MinCostP, int& delta){
    // Assumes orientations are already reversed!!
    // PrintLantarn(sol, lantarn);
    const int i = lantarn.i;
    const int j = lantarn.j;
    if (!sol.IsTeamBalanced(i)){
        // cout << "Repair orientations lantern!" << endl;
        assert(!sol.IsTeamBalanced(j));
        assert(lantarn.InfeasibleColor);
        int source, sink;
        if (sol.Orientation[i][lantarn.c_[j]] == HA::A){
            source = i, sink = j;
        }
        else{
            source = j, sink = i;
        }
        // find path from source to sink, possible using edges in the lantern
#ifndef NDEBUG
        for (int t = 0; t < N; ++t){
            if (t != i && t != j){
                assert(sol.IsTeamBalanced(t));
            }
        }
#endif
        // first look if we can flip the HA of an edge in the lantern:
        int c_source, c_sink;
        for (int k: lantarn.middle){
            c_source = sol.MatchColor[k][source], c_sink = sol.MatchColor[k][sink];
            if (c_source < 0 || c_sink < 0){
                continue;
            }
            if (sol.Orientation[k][c_source] == HA::H && sol.Orientation[k][c_sink] == HA::A){
                path.emplace_back(std::array<int, 3>{sink, k, c_sink}); // sink <- k <- source
                path.emplace_back(std::array<int, 3>{k, source, c_source});
                // cout << "Path in lantern via " << k << endl;
                delta += ReversePath(true, true);
                return true;
            }
        }


        // Now, everything is balanced except for i and j
        // Fix this by finding a path between them!
        // cout << "try to find path from " << source << " to " << sink << endl;
        if (!FindNormalPathOneLeague(source, sink, delta, MinCostP, true)){
            return false;
        }
        // cout << "Found path outside lantern" << endl;
        // Now, everyone should be balanced!!
#ifndef NDEBUG
        for (int t = 0; t < N; ++t){
            assert(sol.IsTeamBalanced(t));
        }
#endif
    }
    return true;
}

int Operator::ReversePath(const bool PR, const bool ComputeDelta){
    // If Path Reversal, we only take into account the deltas of the middle teams
    int i,j,c,e,c2;
    int delta = 0;
    // cout << "New path is" << endl;
    // vector<int>Count(sol.getNrTeams(), 0);
    for (e = 0; e < path.size(); ++e){
        // path is traversed from sink to source
        // sink: +H, source: +A
        i = path[e][0], j = path[e][1], c = path[e][2];
        if (e > 0){
            c2 = path[e-1][2];
            if (ComputeDelta && sol.getSetting()==Setting::TTP){
                delta += CostOrientationSwapTeamiTTP(i, c, c2);
            }
        }
        else if (!PR){
            c2 = path.back()[2];
            if (ComputeDelta && sol.getSetting()==Setting::TTP){
                delta += CostOrientationSwapTeamiTTP(i, c, c2);
            }
        }
    }

    for (e = 0; e < path.size(); ++e){
        // path is traversed from sink to source
        // sink: +H, source: +A
        i = path[e][0], j = path[e][1], c = path[e][2];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);

        if (!sol.SRR){
            std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
        }

        // Count[i]++, Count[j]++;
        /*
        if (sol.Orientation[i][c] == HA::H){
            cout << "(" << i << " <- " << j << ")" << endl;
        }
        else{
            cout << "(" << i << " -> " << j << ")" << endl;
        }
        */
    }
    // cout << "Path length = " << path.size() << endl;
/*
#ifndef NDEBUG
    for (int i = 0; i < sol.getNrTeams(); ++i){
        if (Count[i] > 2){
            cout << i << " occurs " << Count[i] << " times in the cycle!" << endl;
            std::abort(); // abort if we are not considering the line graph
        }
    }
#endif
*/
    return delta;
}

bool Operator::DFS_path_recursion(const int current, const int sink){

    if (current == sink){
        return true;
    }

    Visited[current] = 1;

    int i;
    for (int r = 0; r < R; ++r){
        if (sol.Orientation[current][r] == HA::A){
            i = sol.TeamColorOpp[current][r];
            if (!Visited[i]){
                Pred[i] = current;
                if (DFS_path_recursion(i, sink)){
                    return true;
                }
            }
        }
    }

    return false;
}

bool Operator::DFS_path(const int source, const int sink) {

    // DFS for finding a path from source to sink
    // Path might be longer than what we would get with BFS
    
    clearVisited(); 
    if (DFS_path_recursion(source, sink)){
        return true;
    }
    return false;
}

bool Operator::BFS_path(const int source, const int sink) {

    // BFS for finding a path from source to sink
    // Returns the shortest path!!
    // But: if we are stuck in local optimum, then this might always return the same cycle?

    clearVisited();
    // queue<int> q; // FIFO
    int head = 0, tail = 0;
    
    Visited[source] = 1;
    Queue[tail++] = source;
    
    while (head < tail) {
        
        // Dequeue a vertex
        int curr = Queue[head++];

        if (curr == sink)
            return true; // then path found!!
        
        int i;
        for (int r = 0; r < R; ++r) {
            if (sol.Orientation[curr][r] == HA::A){
                i = sol.TeamColorOpp[curr][r];
                if (!Visited[i]) {
                    Pred[i] = curr;
                    if (i == sink){
                        return true;
                    }
                    Visited[i] = 1;
                    Queue[tail++] = i;
                }
            }
        }
    }
    return false;
}

bool Operator::FindNormalPathOneLeague(const int source, const int sink, int& delta, const bool MinCostP, const bool ComputeDelta){ // source: A too much, sink: H too much
    int i,j,r;
    // cout << "source = " << source << endl;
    // cout << "sink = " << sink << endl;

    clearPred();

    if (gen() % 10 == 1){
        if (!DFS_path(source, sink)){
            return false;
        }
    }
    else{
       if (!BFS_path(source, sink)){
            return false;
        } 
    }

    int curr = sink;
    int k;
    while (curr != source){
        k = Pred[curr];
        r = sol.MatchColor[curr][k];
        path.emplace_back(std::array<int, 3>{curr, k, r});
        assert(sol.Orientation[curr][r] == HA::H);
        assert(sol.Orientation[k][r] == HA::A);
        curr = k;
    }

    delta += ReversePath(true, ComputeDelta);

    return true;
}


void Operator::CycleBalanced(){
    // In principe zou deze genoeg moeten zijn want ik kan van eender welk balanced schedule naar eender ander balanced schedule gaan
    // Do this one if we just want to find a cycle in a balanced schedule
    // Given the cycle, calculate the delta afterwards
    // We know that in a balanced schedule, if we just do a random path, we always end up at a node already in the path

    std::uniform_int_distribution<>dist_team = std::uniform_int_distribution<>(0,N-1);
    std::uniform_int_distribution<>dist_colour = std::uniform_int_distribution<>(0,R-1);
    int i = dist_team(gen);
    int j,c;
    clearVisited();
    clearPath();
    // cout << "Path: " << endl;
    // cout << i;
    while (!Visited[i]){
        assert(sol.IsTeamBalanced(i));
        Visited[i] = 1;
        c = dist_colour(gen);
        int start_color = c;
        while (sol.Orientation[i][c] != HA::H || (!sol.SRR && sol.MatchColor[sol.TeamColorOpp[i][c]][i] >= 0)){
            c = (c + 1)%R;
            if (c == start_color){
                if (sol.SRR){
                    throw std::runtime_error("Infinite loop in CycleBalanced!!"); 
                }
                else{
                    return;
                }
            }
        }
        j = sol.TeamColorOpp[i][c];
        assert(sol.Orientation[j][c] == HA::A);
        path.emplace_back(std::array<int, 3>{i,j,c});
        i = j;
        // cout << " <- " << i;
    }
    // cout << endl;
    // i has been visited twice
    int e = 0;
    if (path[e][0] != i){
        while(path[e][1] != i){
            ++e;
        }
        // cout << "e = " << e << endl;
        path.erase(path.begin(), path.begin()+e+1);
    }
    
    /*
    cout << "Cycle: " << endl;
    for (auto& e: Cycle){
        if (sol.Orientation[e[0]][e[2]] == HA::H){
            cout << e[0] << "<-" << e[1] << endl;
        }
        else{
            cout << e[0] << "->" << e[1] << endl;
        }
    }
    cout << endl;
    */
}

int Operator::ComputeEdgeWeightM(const int i, const int j, const int c, const bool MinCostM, const bool bipartite){
    assert(sol.Orientation[i][c] != sol.Orientation[j][c]);
    if (sol.Orientation[i][c] == sol.Orientation[j][c]){
        return sol.CostImbalance;
    }
    int d = 0;
    if (sol.getSetting() == Setting::TTP){
        if (sol.Orientation[i][c] == HA::A){
            d += sol.getDistanceTeams(getLoc(i,c-1), j) + sol.getDistanceTeams(j, getLoc(i,c+1));
        }
        else if (sol.Orientation[j][c] == HA::A){
            d += sol.getDistanceTeams(getLoc(j,c-1), i) + sol.getDistanceTeams(i, getLoc(j,c+1));
        }
    }
    else {
        d = sol.getDistanceTeams(i,j);
        if (!sol.isEligible(i,j)){
            d += sol.NonEligibleCost;
        }
    }

    return d;
}

void Operator::FindMinCostBalancedACycle(const int r){  

    // cout << "MinCost cycle" << endl;

    int i,j;
    const bool MinCostM = true, bipartite = true;

    int cost = 1; 
    // Make bipartite directed graph: left: A, right: H (incoming arc : H)

    // dist[i][j]: distance of going of going from i to j: i -> j
    // if arc i->j does not exist: cost of infinity
    // In dist the distance x 2 because we set min_weight_cycle = INF, but dist can become less than INF even if there is no path, because some edge weights are negative!!
    clearFloydWarshall();

    // pred[i][j] = last vertex before visiting j from the path from i to j
    // i.e. 1->2->3->4->5->6, then pred[2][6] = 5, pred[3][6] = 4, pred[2][5] = 4
    // in retrieving the path, we fix i (the source), and iteratively take j = pred[i][j]

    // Preparation:
    for (i = 0; i < N; ++i){
        for (j = i+1; j < N; ++j){
            if (sol.MatchColor[i][j] == r){
                cost = -ComputeEdgeWeightM(i, j, r, MinCostM, bipartite); // negative because this we subtract!!+
                if (sol.Orientation[i][r] == HA::H){ // j -> i
                    // cout << j << " -> " << i << "(" << cost << ")" << endl;
                    dist[j][i] = cost;
                    predAC[j][i] = j;
                }
                else{ // i -> j
                    // cout << i << " -> " << j << "(" << cost << ")" << endl;
                    dist[i][j] = cost;
                    predAC[i][j] = i; 
                }
                EdgeWeight[i][j] = cost;
                EdgeWeight[j][i] = cost;
            }
            else if (sol.MatchColor[i][j] < 0){
                if (sol.Orientation[i][r] == sol.Orientation[j][r]){
                    continue;
                }
                // Remember: fictive edges must always point from the H team to the A team!!!!
                cost = ComputeEdgeWeightM(i, j, r, MinCostM, bipartite);
                if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A){
                    // i plays H and j plays A, so we need the arc i -> j
                    // cout << i << " ->- " << j << "(" << cost << ")" << endl;
                    dist[i][j] = cost;
                    predAC[i][j] = i;
                }
                else if (sol.Orientation[i][r] == HA::A && sol.Orientation[j][r] == HA::H){
                    // i plays A and j plays H, so we need the arc j -> i
                    // cout << j << " ->- " << i << "(" << cost << ")" << endl;
                    dist[j][i] = cost;
                    predAC[j][i] = j;
                }
                EdgeWeight[i][j] = cost;
                EdgeWeight[j][i] = cost;
            }
        }
    }

    // Floyd Warshall

    // cout << "FW" << endl;

    for (int i = 0; i < N; ++i){
        dist[i][i] = 0;
        predAC[i][i] = i;
    }
    for (int k = 0; k < N; ++k){
        for (int i = 0; i < N; ++i){
            if (dist[i][k] > INF-1){
                continue;
            }
            for (int j = 0; j < N; ++j){
                if (dist[k][j] > INF-1){
                    continue;
                }
                if (dist[i][j] > dist[i][k] + dist[k][j]){
                    dist[i][j] = dist[i][k] + dist[k][j];
                    predAC[i][j] = predAC[k][j];
                }
            }
        }
    }

    // cout << "FW done" << endl;

    // First, check if negative cycle:

    int cycle_node = -1;
    for (int i = 0; i < N; ++i) {
        if (dist[i][i] < 0) {
            cycle_node = i;
            break; // Found one!
        }
    }

    int start_node = -1;
    int curr;

    // cout << "Look for NC" << endl;

    if (cycle_node != -1) { // if so, then negative cycle exists!
        // cout << "NC" << endl;
        curr = cycle_node;
        
        // To ensure we are inside the cycle and not just on a path leading to it
        // we walk back N times.
        for (i = 0; i < N; ++i) {
            // cout << "curr = " << curr << endl;
            curr = predAC[cycle_node][curr];
        }

        // Now curr is definitely inside a negative cycle.
        start_node = curr;
    }
    else{ // otherwise, no negative cycle. So now, find the best non-negative cycle:
        // cout << "No negative cycle" << endl;
        int min_weight_cycle = INF;
        int h,a;

        // cout << "No NC" << endl;

        for (i = 0; i < N; ++i){
            j = sol.TeamColorOpp[i][r];
            if (i < j){
                continue;
            }
            if (sol.Orientation[i][r] == HA::H){
                h = i, a = j;
            }
            else{
                h = j, a = i;
            }
            // always dist(a -> h) = dist[a][h]
            // but here we are interested in weight[a][h] (a -> h) + dist[h][a] (h -> .. -> a)!!!
            // check which colored edge lies in the best cycle!
            // cout << "weight = " << dist[h][a] << endl;
            if (EdgeWeight[i][j]+dist[h][a] < min_weight_cycle && dist[h][a] < INF){
                min_weight_cycle = EdgeWeight[i][j]+dist[h][a];
                start_node = h;
            }
        }

        // cout << "MinWeight cycle of length = " << min_weight_cycle << endl;
        // cout << "start node = " << start_node << endl;
    }

    // retrieve the cycle in case there exists one

    // For other functions, the edges of the initial matching must be in odd position in cycle!!!
    // So start with uncolored edge, then colored edge, uncolored, etc.

    // cout << "retrieve cycle" << endl;

    if (start_node != -1){
        int k;
        int c = 1;
        i = start_node; 
        j = sol.TeamColorOpp[i][r];
        if (sol.Orientation[i][r] == HA::A){
            std::swap(i,j);
        }
        // cout << "i = " << i << ", j = " << j << endl;
        k = predAC[i][j];

        while (k != i) {
            // cout << "k = " << k << endl;
            if ((++c) % 2 == 0){
                assert(sol.MatchColor[j][k] == -1);
                assert(sol.Orientation[k][r] == HA::H);
                assert(sol.Orientation[j][r] == HA::A);
                AlternatingCycle.emplace_back(j,k);
                // cout << j << " -- " << k << endl;
            }
            else{
                assert(sol.Orientation[j][r] == HA::H);
                assert(sol.Orientation[k][r] == HA::A);
                assert(sol.MatchColor[j][k] == r);
                AlternatingCycle.emplace_back(j,k);
                // cout << j << " <- " << k << endl;
            }
            j = k;
            k = predAC[i][j];
            // cout << k << " = pred " << i << ", " << j << endl;
        }
        // cout << "final 2 edges: " << endl;
        AlternatingCycle.emplace_back(j,i);
        // cout << j << " -- " << i << endl;

        AlternatingCycle.emplace_back(i, sol.TeamColorOpp[i][r]); // i <- j
        // cout << i << " <- " << sol.TeamColorOpp[i][r] << endl;
    }
    // cout << "done" << endl;
}

void Operator::EvaluateAlternatingCycleWithPaths(const int r, const bool bipartite, int& delta, const bool MinCostP, bool NoPathDueTo2RRConstraint){

    clearHA_teamsAC();
    clearPathsAC();

    // cout << "Evaluate alternating cycle" << endl;

    int i,j,e,k,e2;
    // First, uncolor all odd edges (edges of initial matching)
    // subtract from delta

    // Alternating cycle goes like this: h -- i <- j -- k <- l -- m, with e_1 = (i,j), e_2 = (j,k), e_3 = (k,l), etc.

    for (e = 1; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        // cout << i << " <- " << j << endl;
        assert(sol.Orientation[i][r] == HA::H); // most convenient if these are already correct
        assert(sol.Orientation[j][r] == HA::A); // i <- j
        if (sol.getSetting() != Setting::TTP){
            delta -= sol.getDistanceTeams(i, j);
            // cout << sol.getDistanceTeams(i,j) << endl;
        }
        if (!sol.SRR){
            sol.MatchColor[i][j] = -1;
        }
        else{
            sol.MatchColor[i][j] = -1;
            sol.MatchColor[j][i] = -1;
        }
    }

    // cout << "Uncolored edges:" << endl;
    int h_team, a_team;
    for (e = 0; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        if (bipartite){
            assert(sol.Orientation[i][r] == HA::A); 
            assert(sol.Orientation[j][r] == HA::H);
        }
        a_team = i, h_team = j; // default in balanced alternating cycle
        if (sol.Orientation[i][r] == sol.Orientation[j][r] && sol.Orientation[i][r] == HA::H){
            assert(!bipartite);
            if (gen() % 2 == 0){
                h_team = i, a_team = j;
            }
            A_teamsAC.push_back(a_team);
        }
        else if (sol.Orientation[i][r] == sol.Orientation[j][r] && sol.Orientation[i][r] == HA::A){
            assert(!bipartite);;
            if (gen() % 2 == 0){
                h_team = i, a_team = j;
            }
            H_teamsAC.push_back(h_team);
        }
        else if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A){
            a_team = j, h_team = i;
        }
        if (sol.getSetting() != Setting::TTP){
            delta += sol.getDistanceTeams(i, j);
        }
        else{
            // compute deltas of the teams whose orientations did not change!!

            // for the home team, nothing changes! Only compute for away team
            
            // cout << AlternatingCycle[e-1].first << "<-" << AlternatingCycle[e-1].second << endl;
            // cout << j << " -- " << k << endl;
            // assert(sol.Orientation[k][r] == HA::H);
            if (bipartite){
                assert(sol.Orientation[i][r] == HA::A);
                delta += CostUncoloredRoundSwapTeamiTTP(i, j, r, -1); 
            }
            // cout << "delta " << j << " = " << CostUncoloredRoundSwapTeamiTTP(j, k, r, -1, sol) << endl;

            // cout << i << "," << j << "," << r << ": -" << sol.getCostMatchRound(i,j,r) << endl;
            // cout << sol.getDistanceTeams(i,j) << endl;
        }
        SetValueCircleMethod(h_team,a_team,r,sol);
        sol.Orientation[h_team][r] = HA::H;
        sol.Orientation[a_team][r] = HA::A;
    }

    /*
    for (int i = 0; i < sol.getNrTeams(); ++i){
        DeltaReal[i] += sol.ComputeTravelCostTeamTTP(i);
        if (DeltaReal[i] != 0){
            cout << "Delta real of " << i << " = " << DeltaReal[i] << endl;
            for (int r = 0; r < sol.getNrRounds(); ++r){
                if (sol.Orientation[i][r] == HA::H){
                    cout << i;
                }
                else{
                    cout << sol.TeamColorOpp[i][r];
                }
                if (r < sol.getNrRounds()-1){
                    cout << " -> ";
                }
            }
            cout << endl;
        }
    }
    */

    if (!bipartite){
        // fix paths from H_teams to A_teams
        // Pick random H_team and random A_team, and find path between them, and reverse
        // do this by shuffling the vectors, then iteratively go over them
        assert(H_teamsAC.size() == A_teamsAC.size());
        shuffle(H_teamsAC.begin(), H_teamsAC.end(), gen);
        shuffle(A_teamsAC.begin(), A_teamsAC.end(), gen);
        int a;
        for (int k = 0; k < H_teamsAC.size(); ++k){
            // path is shortest in distance, but does not take into account the costs
            // It can be that not path exists between the teams because they are in different disconnected components!
            bool PathFound = false;
            clearPath();
            a = -1;
            do{
                ++a;
                // delta is computed in this function!!
                PathFound = FindNormalPathOneLeague(A_teamsAC[k+a], H_teamsAC[k], delta, MinCostP, false);
            }
            while(k+a+1 < A_teamsAC.size() && !PathFound);

            if (sol.SRR && !PathFound){
                cout << "No path found in M+PR" << endl;
                throw std::runtime_error("Error!!!");
            }
            else if (!sol.SRR && !PathFound){
                // This can happen because 2RR violations are forbidden->these edges are not made
                // In this case, return empty path
                NoPathDueTo2RRConstraint = true;
                return;
            }
            // path is reversed in function!!
            PathsAC.emplace_back(std::move(path));
            std::swap(A_teamsAC[k], A_teamsAC[k+a]);
        }
    }
}

void Operator::GoBackToOldCycle(const int r){
    // cout << "Go back to old matching: " << endl;
    int i,j,e;
    // First, uncolor all even edges (edges of new matching)
    for (e = 0; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        if (!sol.SRR){
            if (sol.Orientation[i][r] == HA::H){
                sol.MatchColor[i][j] = -1;
            }
            else{
                sol.MatchColor[j][i] = -1;
            }
        }
        else{
            sol.MatchColor[i][j] = -1;
            sol.MatchColor[j][i] = -1;
        }
    }
    for (e = 1; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        SetValueCircleMethod(i,j,r,sol);
        sol.Orientation[i][r] = HA::H;
        sol.Orientation[j][r] = HA::A;
        // cout << i << " <- " << j << " in " << r << endl;
    }
}

bool Operator::DFS_Modified(int u, const int r){

    // AdjC[i] = j <=> {i,j} is colored
    // Adj[i] = {k,l,..} <=> {i,k}, {i,l} are all uncolored

    // cout << u << endl;

    if (Count[u] > -1 && Count[u] % 2 == 0){
        // cout << u << " already visited!" << endl;
        // Cycle found!
        Cycle_AC.push_back(u);
        // cout << u << " -> ";
        int cur = Parent[u];
        while (cur != u){
            // cout << cur << " -> ";
            Cycle_AC.push_back(cur);
            cur = Parent[cur];
        }
        Cycle_AC.push_back(cur);
        // cout << u << endl;
        return true;
    }

    if (Count[u] > -1 && Count[u] % 2 == 1){
        return false;
    }

    if (Parent[u] < 0){
        Count[u] = 0;
    }
    else{
        // cout << "Count[" << u << "] = " << "Count[" << Parent[u] << "]+1 = " << Count[Parent[u]]+1 << endl;
        Count[u] = Count[Parent[u]]+1;
    }

    if (Count[u] % 2 == 0){
        int w = sol.TeamColorOpp[u][r];
        Parent[w] = u;
        // cout << "Parent[" << AdjC[u] << "] = " << u << endl;
        // cout << u << "--C--" << AdjC[u] << endl;
        if (DFS_Modified(w, r)){
            return true;
        }
    }
    else{
        const int degree = Adj[u].size();
        if (degree > 0){
            std::uniform_int_distribution<int> dist(0, degree - 1); // ensure some randomness
            int start = dist(gen);
            int i,v,w;
            for (i = 0; i < degree; ++i){
                v = i+start;
                if (v > degree-1){
                    v -= degree;
                }
                w = Adj[u][v];
                if (Visited[w]){
                    continue;
                }
                int ParentCopy = Parent[w];
                // cout << "ParentCopy = " << ParentCopy << endl;
                Parent[w] = u;
                // cout << "Parent[" << v << "] = " << u << endl;
                // cout << u << "-----" << v << endl;
                if (DFS_Modified(w, r)){
                    return true;
                }
                Parent[w] = ParentCopy;
            }
        }
    }

    Visited[u] = true;

    // Count[u] = -1;

    return false;
}


bool Operator::DFS_cycle(int u){

    if (Stack[u]){
        // cout << u << " already visited!" << endl;
        // Cycle found!
        Cycle_AC.push_back(u);
        // cout << u << " <- ";
        int cur = Parent[u];
        while (cur != u){
            // cout << cur << " <- ";
            Cycle_AC.push_back(cur);
            cur = Parent[cur];
        }
        Cycle_AC.push_back(cur);
        // cout << u << endl;
        return true;
    }

    if (Visited[u]){
        return false;
    }

    Visited[u] = 1;
    Stack[u] = 1;

    const int degree = Adj[u].size();
    if (degree > 0){
        std::uniform_int_distribution<int> dist(0, degree - 1); // ensure some randomness
        int start = dist(gen);
        int i,v,w;
        for (i = 0; i < degree; ++i){
            v = i+start;
            if (v > degree-1){
                v -= degree;
            }
            w = Adj[u][v];
            Parent[u] = w;
            if (DFS_cycle(w)){
                return true;
            }
        }
    }
    Stack[u] = 0;
    return false;
}

void Operator::AlternatingCycleBM(const int r, const bool bipartite){
    // Here, we provide a general way for finding an alternating cycle where edges have "the same orientation"

    const int C = N/2;

    int i,j,k;

    clearAdj_AC();
    for (i = 0; i < N; ++i){
        for (k = i+1; k < N; ++k){
            if (sol.MatchColor[i][k] == -1 && sol.isEligible(i,k)){
                if (bipartite && sol.Orientation[i][r] != sol.Orientation[k][r]){
                    if (sol.Orientation[i][r] == HA::H){
                        Adj[i].push_back(k);
                    }
                    else {
                        Adj[k].push_back(i);
                    }
                }
                else if (!bipartite){
                    Adj[i].push_back(k);
                    Adj[k].push_back(i);
                }
            }
            else if (sol.MatchColor[i][k] == r){
                if (bipartite){
                    if (sol.Orientation[i][r] == HA::H){
                        Adj[k].push_back(i);
                    }
                    else {
                        Adj[i].push_back(k);
                    }
                }
            }
        }
    }

    clearParent(); // For DFS_Modified, Parent[u] = Parent[u] is the predecessor of u
    clearCycle_AC();
    clearVisited();
    clearStack();
    clearCount();

    for (i = 0; i < N; ++i){
        if (bipartite && !Visited[i]){
            if (DFS_cycle(i)){
                break;
            }
        }
        else if (!bipartite && !Visited[i]){
            if (DFS_Modified(i, r)){
                break;
            }
        }
    }

    if (!Cycle_AC.empty()){
        for (int i = Cycle_AC.size()-1; i >= 1; --i){ // backwards because alternating cycle with DFS will always be i -- j -> k -- l -> m
            if (sol.MatchColor[Cycle_AC[i-1]][Cycle_AC[i]] == -1){
                if (sol.Orientation[Cycle_AC[i-1]][r] == HA::A){
                    // cout << Edges[i][0] << " -- " << Edges[i][1] << endl;
                    AlternatingCycle.emplace_back(Cycle_AC[i-1], Cycle_AC[i]);
                }
                else{
                    // cout << Edges[i][1] << " -- " << Edges[i][0] << endl;
                    AlternatingCycle.emplace_back(Cycle_AC[i], Cycle_AC[i-1]);
                }
            }
            else{
                if (sol.Orientation[Cycle_AC[i-1]][r] == HA::H){
                    // cout << Edges[i][0] << " <- " << Edges[i][1] << endl;
                    AlternatingCycle.emplace_back(Cycle_AC[i-1], Cycle_AC[i]);
                }
                else{
                    // cout << Edges[i][0] << " -> " << Edges[i][1] << endl;
                    AlternatingCycle.emplace_back(Cycle_AC[i], Cycle_AC[i-1]);
                }
            }
        }
        if (sol.MatchColor[AlternatingCycle[0].first][AlternatingCycle[0].second] != -1){
            pair<int,int>E = AlternatingCycle[0];
            for (int k = 0; k < AlternatingCycle.size()-1; ++k){
                AlternatingCycle[k] = AlternatingCycle[k+1];
            }
            AlternatingCycle.back() = E;
        }
    }
    else{
        // cout << "No alternating cycle!!" << endl;
    }
}

// ********* LINE GRAPH ******* LINE GRAPH ****** LINE GRAPH ******* LINE GRAPH ****** LINE GRAPH *********** //

tuple<vector<Edge>,vector<Edge>,vector<int>> MakeLineGraph(Solution& sol, const int source, const int sink){

    // Update 07/01/2026: Just the line graph does not work, a cycle or path in the line graph results in a walk or closed walk, respectively, in the original graph
    // But: add N nodes in the line graph and I think it works
    // e.h. (ij)->(j)->(jk) instead of (ij)->(jk)

    // source: team with A game too much, sink: team with H too much
    vector<Edge>Nodes; // Nodes of the line graph are edges of the original graph

    // cout << "Find path between " << source << " and " << sink << endl;

    int i,j,r,h,a;
    vector<bool>NodeSeen(sol.getNrTeams(), false);
    for (r = 0; r < sol.getNrRounds(); ++r){
        std::fill(NodeSeen.begin(), NodeSeen.end(), false);
        for (i = 0; i < sol.getNrTeams(); ++i){
            if (!NodeSeen[i]){
                j = sol.TeamColorOpp[i][r];
                if (sol.Orientation[i][r] == HA::H){
                    h = i, a = j;
                }
                else{
                    assert(sol.Orientation[i][r] == HA::A);
                    h = j, a = i;
                }
                if (!sol.SRR && sol.MatchColor[a][h] >= 0){
                    // then we cannot reverse the orientation (unless both their orientations are reversed, but this cannot happen in a path)!!
                    continue;
                }
                if (h != source && a != sink){ // no point in adding the edge a->SOURCE, because SOURCE always must have an outgoing edge (and SINK always an incoming edge)
                    Nodes.push_back({a,h}); // arc goes like a->h
                    // cout << "add the node (" << a << "," << h << ")" << endl;
                }
                NodeSeen[j] = true;
            }
        }
    }
    // cout << "Added all edge nodes" << endl;

    /*
    for (i = 0; i < sol.getNrTeams(); ++i){
        Nodes.push_back({i,i});
    }
    */

    // cout << "Added all the nodes" << endl;

    // Now, evaluate for pair of nodes what the cost will be
    vector<Edge>edge_vector;
    vector<int>weights;

    int weight, v, w, k, r1, r2;
    for (v = 0; v < Nodes.size(); ++v){
        for (w = v+1; w < Nodes.size(); ++w){
            if (Nodes[v].second == Nodes[w].first){ // e.g. if (i,k) = (k,j), add the edge {(i,k),(k,j)}
                i = Nodes[v].first, j = Nodes[w].second, k = Nodes[v].second;
                h = v, a = w;
            }
            else if (Nodes[w].second == Nodes[v].first){ 
                i = Nodes[w].first, j = Nodes[v].second, k = Nodes[v].first;
                h = w, a = v;
            }
            else{
                continue;
            }
            r1 = sol.MatchColor[k][i]; // remember, arc (i,k) = i->k, so k is the home team!
            r2 = sol.MatchColor[j][k]; // remember, arc (k,j) = k->j, so j is the home team!
    
            weight = sol.ComputeCostReversingOrientationTeam(k, r1, r2);

            edge_vector.push_back(Edge(h, a));
            weights.push_back(weight);

            // cout << "add the arc (" << i << "," << k << ") -> (" << k << "," << j << ")" << " with weight " << weight << endl;
        }
    }
    // cout << "added all the edges" << endl;
    return {Nodes, edge_vector, weights};
}

vector<pair<int,int>>ForbiddenPairNC(Solution& sol, const vector<array<int,3>> path){
    int i,j,c,e;
    cout << "New path is" << endl;
    vector<pair<int,int>>ForbiddenPair;
    vector<int>Count(sol.getNrTeams(), 0);
    for (e = 0; e < path.size(); ++e){
        // path is traversed from sink to source
        // sink: +H, source: +A
        i = path[e][0], j = path[e][1], c = path[e][2];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);

        if (!sol.SRR){
            std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
        }

        Count[i]++, Count[j]++;

        if (sol.Orientation[i][c] == HA::H){
            cout << "(" << i << " <- " << j << ")" << endl;
        }
        else{
            cout << "(" << i << " -> " << j << ")" << endl;
        }
    }

    bool VertexFound = false;
    int Vertex = -1;

    for (e = 0; e < path.size(); ++e){
        i = path[e][0];
        if (Count[i] > 2 && (!VertexFound || Vertex == i)){
            cout << i << " occurs " << Count[i] << " times in the cycle!" << endl;
            j = path[e][1];
            if (sol.Orientation[i][c] == HA::H){
                ForbiddenPair.push_back({j,i});
                cout << "Forbidden pair: " << j << " -> " << i << endl;
            }
            else{
                ForbiddenPair.push_back({i,j});
                cout << "Forbidden pair: " << i << " -> " << j << endl;
            }
            if (ForbiddenPair.size() > 1){
                break;
            }
        }
    }
    return ForbiddenPair;
}

vector<array<int,3>> NonIncreasingCycle(Solution& sol){

    // LG

    int source = -1, sink = -1;
    auto [Nodes,edge_vector,weights] = MakeLineGraph(sol, source, sink);

    // Floyd Warshall algorithm:

    int N = Nodes.size();
    const int INF = 10000;
    vector<vector<int>>dist(N, vector<int>(N, INF));
    vector<vector<int>>pred(N, vector<int>(N,-1));

    for (int e = 0; e < edge_vector.size(); ++e){
        auto edge = edge_vector[e]; // edge = {h,a}
        if (weights[e] < 0){
            weights[e] = 0; // non-increasing!!
        }
        dist[edge.second][edge.first] = weights[e];
        pred[edge.second][edge.first] = edge.second;
    }
    for (int i = 0; i < N; ++i){
        dist[i][i] = 0;
        pred[i][i] = i;
    }
    for (int k = 0; k < N; ++k){
        for (int i = 0; i < N; ++i){
            for (int j = 0; j < N; ++j){
                if (dist[i][j] > dist[i][k] + dist[k][j]){
                    dist[i][j] = dist[i][k] + dist[k][j];
                    pred[i][j] = pred[k][j];
                }
            }
        }
    }

    int min_weight_cycle = INF;
    int start_edge = -1;

    for (int e = 0; e < edge_vector.size(); ++e){
        auto edge = edge_vector[e];
        if (weights[e]+dist[edge.first][edge.second] < min_weight_cycle){
            min_weight_cycle = weights[e]+dist[edge.first][edge.second];
            start_edge = e;
        }
    }

    vector<array<int,3>>Cycle;

    // retrieve the cycle:
    Edge e;
    int i,k,r;
    int start = edge_vector[start_edge].first;
    int curr = edge_vector[start_edge].second;
    // cout << start << " <- " << curr;
    // cout << "(" << Nodes[start].first << " -> " << Nodes[start].second << ") -> " << "(" << Nodes[curr].first << " -> " << Nodes[curr].second << ") -> "; 

    e = Nodes[curr];
    i = e.first, k = e.second; // (i,k) = i->k
    r = sol.MatchColor[k][i];
    Cycle.emplace_back(std::array<int, 3>{k, i, r});

    while(curr != start){
        curr = pred[start][curr];
        // cout << " <- " << curr;
        e = Nodes[curr];
        i = e.first, k = e.second; // (i,k) = i->k
        r = sol.MatchColor[k][i];
        // cout << "(" << i << " -> " << k << ")";
        Cycle.emplace_back(std::array<int, 3>{k, i, r});
        assert(sol.Orientation[k][r] == HA::H);
        assert(sol.Orientation[i][r] == HA::A);
        // if (curr != start){cout << " -> ";};
    }
    // cout << endl;

    // cout << "min_weight_cycle = " << min_weight_cycle << endl;
    // cin.get();

    return Cycle;
}

vector<array<int,3>> NegativeCycle(Solution& sol){
    // See: https://www.boost.org/doc/libs/1_31_0/libs/graph/example/bellman-example.cpp

    int source = -1, sink = -1;
    auto [Nodes,edge_vector,weights] = MakeLineGraph(sol, source, sink);

    int N = Nodes.size();
    int SOURCE = N;
    for (int v = 0; v < N; ++v){
        edge_vector.push_back(Edge(SOURCE, v)); // create edges SOURCE --> v
        weights.push_back(0); // get weight of 0
        // cout << "add " << SOURCE << " -> " << "(" << Nodes[v].first << "," << Nodes[v].second << ")" << endl;
    }

    BGraph g(N+1); // Nodes.size()+2 vertices, +2 bc of extra SOURCE and SINK node

    for (std::size_t j = 0; j < edge_vector.size(); ++j){
        boost::add_edge(edge_vector[j].first, edge_vector[j].second, weights[j], g);
    }

    typedef boost::property_map<BGraph, boost::edge_weight_t>::type WeightMap;
    WeightMap weight_map = boost::get(boost::edge_weight, g);
    typedef boost::graph_traits<BGraph>::vertex_descriptor Vertex;

    vector<double> distance(N+1, std::numeric_limits<double>::max());
    vector<Vertex> predecessor(N+1, boost::graph_traits<BGraph>::null_vertex());

    distance[SOURCE] = 0; // arbitrary start node

    // cout << "start BF" << endl;

    // bool r = boost::bellman_ford_shortest_paths(g, C+1, weight_pmap, &parent[0], &distance[0], boost::closed_plus<int>(), std::less<int>(), boost::default_bellman_visitor());

    bool NC = boost::bellman_ford_shortest_paths(g, N+1, boost::weight_map(get(boost::edge_weight, g)).distance_map(&distance[0]).predecessor_map(&predecessor[0]));

    vector<array<int,3>>Cycle;
     
    if (!NC)
    {
        Vertex cycle_vertex = boost::graph_traits<BGraph>::null_vertex();
 
        for (int i = 0; i < N+1; ++i)
        {
            for (auto e : make_iterator_range(edges(g)))
            {
                Vertex u = boost::source(e, g), v = boost::target(e, g);
                double weight = get(boost::edge_weight, g, e);
 
                // If we can still relax an edge, then v is part of a negative cycle
                if (distance[u] + weight < distance[v])
                {
                    cycle_vertex = v;
                    break;
                }
            }
            if (cycle_vertex != boost::graph_traits<BGraph>::null_vertex())
                break;
        }

        // cout << "Negative cycle found: " << endl;
        if (cycle_vertex != boost::graph_traits<BGraph>::null_vertex())
        {
            Vertex current = cycle_vertex;
            for (int i = 0; i < N+1; ++i) // Move `N` steps to guarantee being inside the cycle
                current = predecessor[current];
 
            // Track the cycle
            Vertex cycle_start = current;

            Edge e;
            int i,k,r;

            Vertex prev;
            do {
                e = Nodes[current];
                i = e.first, k = e.second; // (i,k) = i->k
                r = sol.MatchColor[k][i];
                Cycle.emplace_back(std::array<int, 3>{k, i, r});
                // cout << "(" << k << " <- " << i << ")";
                assert(sol.Orientation[k][r] == HA::H);
                assert(sol.Orientation[i][r] == HA::A);
                prev = predecessor[current];
                int s = 0;
                while(!(edge_vector[s].first == prev && edge_vector[s].second == current)){
                    ++s;
                }
                current = prev;
                // if (current != cycle_start){cout << " -> (" << weights[s] << ")";};
                // cout << " <- (" << weights[s] << ")";
                
            }while (current != cycle_start);
            // cout << endl;
        }
    }
    else{
        // cout << "No negative cycle found!" << endl;
        // cout << "NC true" << endl;
        assert(Cycle.empty());
    }

    return Cycle;

}

// ********* LINE GRAPH ******* LINE GRAPH ****** LINE GRAPH ******* LINE GRAPH ****** LINE GRAPH *********** //


