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

int Operator::DeltaHASwap(const int i, const int r){
    // Effect of swapping the orientation of team i in round r on the HAPs for TTP
    const int min_r = std::max(0, r-3);
    const int max_r = std::min(R-1, r+3);
    int delta = -sol.ComputeTTPViolations(i, min_r, max_r);

    if (sol.Orientation[i][r] == HA::H){
        sol.Orientation[i][r] = HA::A;
    }
    else{
        sol.Orientation[i][r] = HA::H;
    }

    delta += sol.ComputeTTPViolations(i, min_r, max_r);
    if (sol.Orientation[i][r] == HA::H){
        sol.Orientation[i][r] = HA::A;
    }
    else{
        sol.Orientation[i][r] = HA::H;
    }

    return sol.getCostTTPViolation()*delta;
}

int Operator::CostTTPSingleEdgeSwap(const int i, const int r){
    // cost of swapping orientation of edge {i,j} for i
    // Both effect on HAPs and travel distance for TTP
    int delta = 0;

    int L_r_prev = getLoc(i,r-1);
    int L_r      = getLoc(i,r);
    int L_r_next = getLoc(i,r+1);

    int L_r_after = i;
    if (sol.Orientation[i][r] == HA::H){
        L_r_after = sol.TeamColorOpp[i][r];
    }

    delta += DeltaHASwap(i,r);
    delta -= (sol.getDistanceTeams(L_r_prev, L_r) + sol.getDistanceTeams(L_r, L_r_next));
    delta += (sol.getDistanceTeams(L_r_prev, L_r_after) + sol.getDistanceTeams(L_r_after, L_r_next));
    return delta;
}

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

int Operator::DeltaHACostOrientationSwapTeam(const int i, const int r, const int s){
    int delta = 0;
    auto& RowOrientation = sol.Orientation[i];

    if (RowOrientation[r] == RowOrientation[s]){
        return delta;
    }

    if (abs(r-s) >= 4){
        const int min_r = std::max(0, r-3);
        const int max_r = std::min(R-1, r+3);
        const int min_s = std::max(0, s-3);
        const int max_s = std::min(R-1, s+3);

        delta -= (sol.getCostTTPViolation()*(sol.ComputeTTPViolations(i, min_r, max_r) + sol.ComputeTTPViolations(i, min_s, max_s)));
        std::swap(RowOrientation[r], RowOrientation[s]);
        delta += sol.getCostTTPViolation()*(sol.ComputeTTPViolations(i, min_r, max_r) + sol.ComputeTTPViolations(i, min_s, max_s));
        std::swap(RowOrientation[r], RowOrientation[s]);
    }
    else{
        const int min_r = std::max(0, std::min(r,s)-3);
        const int max_r = std::min(R-1, std::max(r,s)+3);

        delta -= sol.getCostTTPViolation()*sol.ComputeTTPViolations(i, min_r, max_r);
        std::swap(RowOrientation[r], RowOrientation[s]);
        delta +=  sol.getCostTTPViolation()*sol.ComputeTTPViolations(i, min_r, max_r);
        std::swap(RowOrientation[r], RowOrientation[s]);
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

int Operator::CostUncoloredRoundSwapHASwapTeamiTTP(const int i, const int UncoloredOpponent_i, const int r, const int s){
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

    int L_c_prev = getLoc(i, c - 1);
    int L_c = getLoc(i, c);
    int L_c_next = getLoc(i, c + 1);

    int L_c_after = i;
    if (sol.Orientation[i][c] == HA::H){
        L_c_after = UncoloredOpponent_i;
    }

    delta += DeltaHASwap(i,r);
    delta -= (sol.getDistanceTeams(L_c_prev, L_c) + sol.getDistanceTeams(L_c, L_c_next));
    // cout << "initial travel for team " << i << ": " << L_c_prev << " -> (" << sol.getDistanceTeams(L_c_prev, L_c) << ")" << L_c << " -> (" << sol.getDistanceTeams(L_c, L_c_next) << ")" << L_c_next << endl;
    delta += (sol.getDistanceTeams(L_c_prev, L_c_after) + sol.getDistanceTeams(L_c_after, L_c_next));
    // cout << "new travel for team " << i << ": " << L_c_prev << " -> (" << sol.getDistanceTeams(L_c_prev, UncoloredOpponent_i) << ")" << UncoloredOpponent_i << " -> (" << sol.getDistanceTeams(UncoloredOpponent_i, L_c_next) << ")" << L_c_next << endl;

    return delta; 
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
        if (!sol.ConstraintViolationAllowed && delta > sol.getCostTTPViolation()){
            return delta;
        }
        // std::cout << "t: " << next << std::endl;
        next = sol.TeamColorOpp[next][s];
        delta += CostRoundSwapTeamiTTP(next, r, s);
        if (!sol.ConstraintViolationAllowed && delta > sol.getCostTTPViolation()){
            return delta;
        }
        next = sol.TeamColorOpp[next][r];
    }
    while (next != StartNode);
    return delta;
}

int Operator::DeltaiPTS_TS(const int k, int r, int s){
    assert(sol.Orientation[k][r] != sol.Orientation[k][s]);
    int delta = 0;
    int L_r_next, L_s_next;
    if (r > s){
        std::swap(r,s);
    }
    if (sol.Orientation[k][r] == HA::H){
        L_r_next = k;
        L_s_next = sol.TeamColorOpp[k][r];
    }
    else{
        L_r_next = sol.TeamColorOpp[k][s];
        L_s_next = k;
    }
    if (abs(r-s) > 1){
        delta -= (sol.getDistanceTeams(getLoc(k,r-1), getLoc(k,r)) + sol.getDistanceTeams(getLoc(k,r), getLoc(k,r+1)));
        delta -= (sol.getDistanceTeams(getLoc(k,s-1), getLoc(k,s)) + sol.getDistanceTeams(getLoc(k,s), getLoc(k,s+1)));

        delta += (sol.getDistanceTeams(getLoc(k,r-1), L_r_next) + sol.getDistanceTeams(L_r_next, getLoc(k,r+1)));
        delta += (sol.getDistanceTeams(getLoc(k,s-1), L_s_next) + sol.getDistanceTeams(L_s_next, getLoc(k,s+1)));
    }
    else{
        delta -= (sol.getDistanceTeams(getLoc(k,r-1), getLoc(k,r)) + sol.getDistanceTeams(getLoc(k,r), getLoc(k,s)) + sol.getDistanceTeams(getLoc(k,s), getLoc(k,s+1)));
        delta += (sol.getDistanceTeams(getLoc(k,r-1), L_r_next) + sol.getDistanceTeams(L_r_next, L_s_next) + sol.getDistanceTeams(L_s_next, getLoc(k,s+1)));
    }
    return delta;
}

void Operator::DeltaUnbalancedAlternatingCycle(int& delta, const vector<int>& CostBeforeTTPTeams){
    clearVisited();
    int t1,t2,e;
    /*
    for (e = 1; e < AlternatingCycle.size(); e+=2){
        t1 = AlternatingCycle[e].first, t2 = AlternatingCycle[e].second;
        delta += sol.ComputeTotalCostTeamTTP(t1);
        if (!sol.ConstraintViolationAllowed && delta >= sol.getCostTTPViolation()){
            return;
        }
        delta += (CostBeforeTTPTeams[t1] + sol.ComputeTotalCostTeamTTP(t2));
        if (!sol.ConstraintViolationAllowed && delta >= sol.getCostTTPViolation()){
            return;
        }
        delta += CostBeforeTTPTeams[t2];
        Visited[t1] = 1, Visited[t2] = 1;
    }
    */
    for (auto& current_path: PathsAC){
        path = current_path;
        for (e = 0; e < path.size(); ++e){
            t1 = path[e][0], t2 = path[e][1];
            if (!Visited[t1]){
                delta += sol.ComputeTotalCostTeamTTP(t1);
                if (!sol.ConstraintViolationAllowed && delta >= sol.getCostTTPViolation()){
                    return;
                }
                delta += CostBeforeTTPTeams[t1]; 
                Visited[t1] = 1;
            }
            if (!Visited[t2]){
                delta += sol.ComputeTotalCostTeamTTP(t2);
                if (!sol.ConstraintViolationAllowed && delta >= sol.getCostTTPViolation()){
                    return;
                }
                delta += CostBeforeTTPTeams[t2];
                Visited[t2] = 1;
            }
        }
    }
    return;
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

// delta lantarn: this is only needed when we do cycle reversals in the lantarn!

void Operator::DeltaLantarn(int& delta){
    int c_i, c_j;
    for (auto& k: lantarn.middle){
        c_i = sol.MatchColor[lantarn.i][k];
        c_j = sol.MatchColor[lantarn.j][k];
        // int delta_k = 0;
        if (c_i > -1 && c_j > -1){
            if (SwapColorLantarn[k]){
                delta += CostRoundSwapTeamiTTP(k, c_i, c_j);
                // delta_k += CostRoundSwapTeamiTTP(k, c_i, c_j);
            }
            else{
                delta += DeltaiPTS_TS(k, c_i, c_j);
                // delta_k += DeltaiPTS_TS(k, c_i, c_j);
            }
        }
        else if (c_i < 0){
            delta += CostUncoloredRoundSwapTeamiTTP(k, lantarn.i, c_j, c_i);
            // delta_k += CostUncoloredRoundSwapTeamiTTP(k, lantarn.i, c_j, c_i);
        }
        else{
            delta += CostUncoloredRoundSwapTeamiTTP(k, lantarn.j, c_i, c_j);
            // delta_k += CostUncoloredRoundSwapTeamiTTP(k, lantarn.j, c_i, c_j);
        }
        // cout << "delta of " << k << " = " << delta_k << endl;
    }
    return;
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
    cout << k;
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
    for (auto& k: lantarn.middle){
        PrintEdgeLantarn(lantarn.i, k, lantarn.j);
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
            if (SwapColorLantarn[k_]){
                std::swap(sol.Orientation[k_][c_i], sol.Orientation[k_][c_j]);
                OrientationCopy_i[c_j] = sol.Orientation[i][c_i];
                OrientationCopy_j[c_i] = sol.Orientation[j][c_j];
            }
            else{
                OrientationCopy_i[c_j] = sol.Orientation[j][c_j];
                OrientationCopy_j[c_i] = sol.Orientation[i][c_i];
            }
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

void Operator::ReplenishLantarn(const int i, const int j, const int StartColor){
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
            if (lantarn.i == i && sol.Orientation[k][c_i] == HA::H && sol.Orientation[k][c_j] == HA::A){
                lantarn.up.push_back(k);
            }
            else if (lantarn.i == i && sol.Orientation[k][c_i] == HA::A && sol.Orientation[k][c_j] == HA::H){
                lantarn.down.push_back(k);
            }
            else if (lantarn.i == j && sol.Orientation[k][c_i] == HA::H && sol.Orientation[k][c_j] == HA::A){
                lantarn.down.push_back(k);
            }
            else if (lantarn.i == j && sol.Orientation[k][c_i] == HA::A && sol.Orientation[k][c_j] == HA::H){
                lantarn.up.push_back(k);
            }
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

void Operator::CreateLantarn(const int i, const int j, const int StartColor){
    assert(StartColor >= 0);
    lantarn.reset(i,j);
    // cout << "i = " << i << ", j = " << j << endl;
    ReplenishLantarn(i, j, StartColor);
    if (lantarn.InfeasibleColor /*&& !lantarn.InfeasibleOpponents*/){
        if (sol.ViolationEligibleOpponents_allowed || !lantarn.InfeasibleOpponents){
            ReplenishLantarn(j, i, StartColor);
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
        assert(lantarn.PathReversalNeeded);
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
                path.push_back({sink, k, c_sink}); // sink <- k <- source
                path.push_back({k, source, c_source});
                // cout << "Path in lantern via " << k << endl;
                delta += ReversePath(true, true);
                return true;
            }
        }


        // Now, everything is balanced except for i and j
        // Fix this by finding a path between them!
        // cout << "try to find path from " << source << " to " << sink << endl;
        /*
        if (MinCostP){
            if (!ShortestPathLineGraph(source, sink, delta)){
                return false;
            }
        }
        else{
            */
            if (!FindNormalPathOneLeague(source, sink, delta, true)){
                return false;
            }
        //}
        // cout << "Found path outside lantern" << endl;
        // Now, everyone should be balanced!!
#ifndef NDEBUG
        for (int t = 0; t < N; ++t){
            assert(sol.IsTeamBalanced(t));
        }
#endif
    }
    else{
        assert(!lantarn.PathReversalNeeded);
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
                // cout << "delta " << i << " = " << CostOrientationSwapTeamiTTP(i, c, c2) << endl;
            }
        }
        else if (!PR){
            c2 = path.back()[2];
            if (ComputeDelta && sol.getSetting()==Setting::TTP){
                delta += CostOrientationSwapTeamiTTP(i, c, c2);
                // cout << "delta " << i << " = " << CostOrientationSwapTeamiTTP(i, c, c2) << endl;
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
    // Queue does not need reinitialization, just overwrite
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

bool Operator::FindNormalPathOneLeague(const int source, const int sink, int& delta, const bool ComputeDelta){ // source: A too much, sink: H too much
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
        path.push_back({curr, k, r});
        assert(sol.Orientation[curr][r] == HA::H);
        assert(sol.Orientation[k][r] == HA::A);
        curr = k;
    }

    delta += ReversePath(true, ComputeDelta);

    return true;
}


bool Operator::CycleBalanced(){
    // In principe zou deze genoeg moeten zijn want ik kan van eender welk balanced schedule naar eender ander balanced schedule gaan
    // Do this one if we just want to find a cycle in a balanced schedule
    // Given the cycle, calculate the delta afterwards
    // We know that in a balanced schedule, if we just do a random path, we always end up at a node already in the path

    int i = DisN->Sample();
    int j,next_c;
    int first_c = DisR->Sample();
    int c = first_c;
    clearVisited();
    clearPath();
    // cout << "Path: " << endl;
    // cout << i;
    while (!Visited[i]){
        assert(sol.IsTeamBalanced(i));
        Visited[i] = 1;
        int start_color = c;
        while (sol.Orientation[i][c] != HA::H || (!sol.SRR && sol.MatchColor[sol.TeamColorOpp[i][c]][i] >= 0)){
            c = (c + 1)%R;
            if (c == start_color){
                if (sol.SRR){
                    throw std::runtime_error("Infinite loop in CycleBalanced!!"); 
                }
                else{
                    clearPath();
                    return false;
                }
            }
        }
        j = sol.TeamColorOpp[i][c];
        assert(sol.Orientation[j][c] == HA::A);
        path.push_back({i,j,c});
        if (!Visited[j]){
            next_c = DisR->Sample();
            if (!sol.ConstraintViolationAllowed && DeltaHACostOrientationSwapTeam(j, c, next_c) >= sol.getCostTTPViolation()){
                clearPath();
                return false;
            }
            c = next_c;
        }
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

    assert(path.front()[0] == path.back()[1]);
    if (!sol.ConstraintViolationAllowed && DeltaHACostOrientationSwapTeam(path.front()[0], path.front()[2], path.back()[2]) >= sol.getCostTTPViolation()){
        clearPath();
        return false;
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
   return true;
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

void Operator::FloydWarshall(vector<vector<int>>& dist, vector<vector<int>>& pred){
    // Floyd Warshall

    // cout << "FW" << endl;

    const int N = dist.size();

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
                    pred[i][j] = pred[k][j];
                }
            }
        }
    }

    // cout << "FW done" << endl;
    return;
}

void Operator::PrepareFloydWarshallAlternatingCycle(const int r){

    int i,j;
    const bool MinCostM = true, bipartite = true;
    int cost = 1;

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

    for (int i = 0; i < N; ++i){
        dist[i][i] = 0;
        predAC[i][i] = i;
    }
}

int Operator::StartNodeAlternatingCycleFloydWarshall(const int r){
    // First, check if negative cycle:

    const int N = dist.size();

    int cycle_node = -1;
    for (int i = 0; i < N; ++i) {
        if (dist[i][i] < 0) {
            cycle_node = i;
            break; // Found one!
        }
    }

    int start_node = -1;
    int curr,i,j;

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
    return start_node;
}

bool Operator::RetrieveAlternatingCycleFloydWarshall(const int start_node, const int r){
    // cout << "retrieve cycle" << endl;
    int i,j,k;
    int safety_counter = 0;
    if (start_node == -1){
        return false;
    }
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

        safety_counter++;
        if (safety_counter > N + 5) {
            cout << "CRITICAL ERROR: Infinite loop detected in cycle retrieval!" << endl;
            return false;
        }
    }
    // cout << "final 2 edges: " << endl;
    AlternatingCycle.emplace_back(j,i);
    // cout << j << " -- " << i << endl;

    AlternatingCycle.emplace_back(i, sol.TeamColorOpp[i][r]); // i <- j
    // cout << i << " <- " << sol.TeamColorOpp[i][r] << endl;
    return true;
}

bool Operator::FindMinCostBalancedACycle(const int r){  

    // cout << "MinCost cycle" << endl;

    // Make bipartite directed graph: left: A, right: H (incoming arc : H)

    // dist[i][j]: distance of going of going from i to j: i -> j
    // if arc i->j does not exist: cost of infinity
    // In dist the distance x 2 because we set min_weight_cycle = INF, but dist can become less than INF even if there is no path, because some edge weights are negative!!
    clearFloydWarshall();

    // pred[i][j] = last vertex before visiting j from the path from i to j
    // i.e. 1->2->3->4->5->6, then pred[2][6] = 5, pred[3][6] = 4, pred[2][5] = 4
    // in retrieving the path, we fix i (the source), and iteratively take j = pred[i][j]

    // Preparation:
    PrepareFloydWarshallAlternatingCycle(r);


    FloydWarshall(dist, predAC); // this function fills dist and predAC
    // Based on this, we can find the start node of the cycle: retrieve the cycle in case there exists one
    int start_node = StartNodeAlternatingCycleFloydWarshall(r);

    // For other functions, the edges of the initial matching must be in odd position in cycle!!!
    // So start with uncolored edge, then colored edge, uncolored, etc.
    bool success = RetrieveAlternatingCycleFloydWarshall(start_node, r); // populates AlternatingCycle

    // cout << "done" << endl;
    return success;
}

void Operator::EvaluateAlternatingCycleWithPaths(const int r, const bool bipartite, int& delta, const bool MinCostP){

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

    /*
    cout << "---------------------------------" << endl;
    for (int t = 0; t < N; ++t){
        cout << "cost before of " << t << " = " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    cout << "---------------------------------" << endl;
    */

    // cout << "Uncolored edges:" << endl;
    int h_team, a_team;
    for (e = 0; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        a_team = i, h_team = j; // default in balanced alternating cycle
        if (bipartite){
            assert(sol.Orientation[i][r] == HA::A); 
            assert(sol.Orientation[j][r] == HA::H);
            if (sol.getSetting() != Setting::TTP){
                delta += sol.getDistanceTeams(i, j);
            }
            else{
                // compute deltas of the teams whose orientations did not change!!

                // for the home team, nothing changes! Only compute for away team
                
                // cout << AlternatingCycle[e-1].first << "<-" << AlternatingCycle[e-1].second << endl;
                // cout << j << " -- " << k << endl;
                // assert(sol.Orientation[k][r] == HA::H);

                delta += CostUncoloredRoundSwapTeamiTTP(i, j, r, -1);
                
                // cout << "delta " << j << " = " << CostUncoloredRoundSwapTeamiTTP(j, k, r, -1, sol) << endl;

                // cout << i << "," << j << "," << r << ": -" << sol.getCostMatchRound(i,j,r) << endl;
                // cout << sol.getDistanceTeams(i,j) << endl;
            }
        }
        else{
            if (sol.Orientation[i][r] == sol.Orientation[j][r]){
                if (sol.Orientation[i][r] == HA::H){
                    assert(!bipartite);
                    if (gen() % 2 == 0){
                        h_team = i, a_team = j;
                    }
                    A_teamsAC.push_back(a_team);
                    delta += CostUncoloredRoundSwapHASwapTeamiTTP(a_team, h_team, r, -1);
                    // cout << "1" << endl;
                    // cout << "delta of " << a_team  << " = " << CostUncoloredRoundSwapHASwapTeamiTTP(a_team, h_team, r, -1) << endl;
                }
                else{
                    assert(!bipartite);;
                    if (gen() % 2 == 0){
                        h_team = i, a_team = j;
                    }
                    H_teamsAC.push_back(h_team);
                    delta += CostUncoloredRoundSwapTeamiTTP(a_team, h_team, r, -1);
                    delta += CostUncoloredRoundSwapHASwapTeamiTTP(h_team, a_team, r, -1);
                    // cout << "2" << endl;
                    // cout << "delta of " << a_team  << " = " << CostUncoloredRoundSwapTeamiTTP(a_team, h_team, r, -1) << endl;
                    // cout << "delta of " << h_team  << " = " << CostUncoloredRoundSwapHASwapTeamiTTP(h_team, a_team, r, -1) << endl;
                }
            }
            else{
                if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A){
                    a_team = j, h_team = i;
                }
                // nothing changes for the team that played home -> still plays home!!
                delta += CostUncoloredRoundSwapTeamiTTP(a_team, h_team, r, -1);
                // cout << "3" << endl;
                // cout << "delta of " << a_team << " = " << CostUncoloredRoundSwapTeamiTTP(a_team, h_team, r, -1) << endl;
            }
        }
        sol.SetColorMatch(h_team,a_team,r);
        sol.Orientation[h_team][r] = HA::H;
        sol.Orientation[a_team][r] = HA::A;
    }

    /*
    cout << "---------------------------------" << endl;
    for (int t = 0; t < N; ++t){
        cout << "cost after of " << t << " = " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    cout << "---------------------------------" << endl;
    cin.get();
    */
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
            h_team = H_teamsAC[k];
            // path is shortest in distance, but does not take into account the costs
            // It can be that not path exists between the teams because they are in different disconnected components!
            bool PathFound = false;
            clearPath();
            a = -1;
            do{
                ++a;
                a_team = A_teamsAC[k+a];
                // delta is computed in this function!!
                PathFound = FindNormalPathOneLeague(a_team, h_team, delta, true);
            }
            while(k+a+1 < A_teamsAC.size() && !PathFound);

            // also compute deltas of source and sink!! (single edge)
            // because we already reversed the arc in  FindNormalPathOneLeague, we have to do minus here
            if (path.size() > 1){
                delta -= CostTTPSingleEdgeSwap(path.front()[0], path.front()[2]);
                delta -= CostTTPSingleEdgeSwap(path.back()[1], path.back()[2]);
            }
            else{
                delta -= CostTTPSingleEdgeSwap(path.front()[0], path.front()[2]);
                delta -= CostTTPSingleEdgeSwap(path.front()[1], path.front()[2]);
            }

            if (sol.SRR && !PathFound){
                cout << "No path found in M+PR" << endl;
                throw std::runtime_error("Error!!!");
            }
            // path is reversed in function!!
            PathsAC.emplace_back(std::move(path));
            std::swap(A_teamsAC[k], a_team);
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
        sol.SetColorMatch(i,j,r);
        sol.Orientation[i][r] = HA::H;
        sol.Orientation[j][r] = HA::A;
        // cout << i << " <- " << j << " in " << r << endl;
    }
}

bool Operator::DFS_Modified(int u, const int l, const int r){

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
        // cout << "return from " << u << ": odd count" << endl;
        return false;
    }

    if (Parent[u] < 0){
        // cout << "count of u = " << 0 << endl;
        Count[u] = 0;
    }
    else{
        // cout << "Count[" << u << "] = " << "Count[" << Parent[u] << "]+1 = " << Count[Parent[u]]+1 << endl;
        Count[u] = Count[Parent[u]]+1;
    }

    if (Count[u] % 2 == 0){
        // int w = sol.getIndexInLeague(sol.TeamColorOpp[sol.getGlobalIndexTeam(l,u)][r]);
        int w = sol.TeamColorOpp[u][r];
        // int ParentCopy = Parent[w];
        Parent[w] = u;
        // cout << "Parent[" << w << "] = " << Parent[w] << endl;
        // cout << u << "--C--" << w << endl;
        if (DFS_Modified(w, l, r)){
            return true;
        }
        // Parent[w] = ParentCopy;
    }
    else{
        // cout << "explore fictive neighbours of " << u << endl;
        /*
        const int degree = Adj[u].size();
        if (degree > 0){
            int start = rand() % degree; // ensure some randomness
            int i,v,w;
            for (i = 0; i < degree; ++i){
                v = i+start;
                if (v > degree-1){
                    v -= degree;
                }
                w = Adj[u][v];
                // cout << "next is " << w << endl;
                if (Visited[w]){
                    // cout << "but already visited" << endl;
                    continue;
                }
                int ParentCopy = Parent[w];
                // cout << "ParentCopy = " << ParentCopy << endl;
                Parent[w] = u;
                // cout << "Parent[" << w << "] = " << u << endl;
                // cout << u << "-----" << w << endl;
                if (DFS_Modified(w, l, r)){
                    return true;
                }
                Parent[w] = ParentCopy;
            }
        }
        */
        int N_l = sol.getNrTeamsLeague(l);
        int start = gen() % N_l; // ensure some randomness
        int i,w;
        for (i = 0; i < N_l; ++i){
            w = i+start;
            if (w > N_l-1){
                w -= N_l;
            }
            w = sol.getTeamsLeague(l)[w];
            if (sol.MatchColor[u][w] != -1 && sol.isEligible(u,w)){
                continue;
            }
            // cout << "next is " << w << endl;
            if (Visited[w]){
                // cout << "but already visited" << endl;
                continue;
            }
            int ParentCopy = Parent[w];
            // cout << "ParentCopy = " << ParentCopy << endl;
            Parent[w] = u;
            // cout << "Parent[" << w << "] = " << u << endl;
            // cout << u << "-----" << w << endl;
            if (DFS_Modified(w, l, r)){
                return true;
            }
            Parent[w] = ParentCopy;
        }
    }

    Visited[u] = false;

    Count[u] = -1;

    return false;
}


bool Operator::DFS_cycle(int u, const int l, const int r){

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

    /*
    if (sol.MatchColor[i][k] == -1 && sol.isEligible(i,k)){
                if (bipartite && sol.Orientation[i][r] != sol.Orientation[k][r]){
                    if (sol.Orientation[i][r] == HA::H){
                        Adj[i_].push_back(k_);
                    }
                    else {
                        Adj[k_].push_back(i_);
                    }
                }
                */
    /*
    const int degree = Adj[u].size();
    if (degree > 0){
        int start = rand() % degree; // ensure some randomness
        int i,v,w;
        for (i = 0; i < degree; ++i){
            v = i+start;
            if (v > degree-1){
                v -= degree;
            }
            w = Adj[u][v];
            Parent[u] = w;
            if (DFS_cycle(w,r)){
                return true;
            }
        }
    }
    */

    int N_l = sol.getNrTeamsLeague(l);
    int start = gen() % N_l; // ensure some randomness
    int i,w;
    for (i = 0; i < N_l; ++i){
        w = i+start;
        if (w > N_l-1){
            w -= N_l;
        }
        w = sol.getTeamsLeague(l)[w];
        if (!sol.isEligible(u,w)){
            continue;
        }
        if (sol.MatchColor[u][w] == -1){
            if (sol.Orientation[u][r] == HA::A || (sol.Orientation[u][r] == sol.Orientation[w][r])){
                continue;
            }
        }
        else if (sol.MatchColor[u][w] == r){
            if (sol.Orientation[u][r] == HA::H){
                continue;
            }
        }
        else{
            continue;
        }
        Parent[u] = w;
        if (DFS_cycle(w,l,r)){
            return true;
        }
    }

    Stack[u] = 0;
    return false;
}

void Operator::AlternatingCycleBM(const int l, const int r, const bool bipartite){
    // Here, we provide a general way for finding an alternating cycle where edges have "the same orientation"

    const int C = N/2;
    const int N_l = sol.getNrTeamsLeague(l);
    if (sol.getSetting() == Setting::TTP || sol.getSetting() == Setting::Football){
        assert(N == N_l);
    }

    int i,j,k,i_,j_,k_;

    // cout << "****** GRAPH ********" << endl;

    // TODO: the following is very expensive!!

    // Adj in case of DFS: fictive edge and opposing HA
    // 

    /*
    clearAdj_AC();
    for (i_ = 0; i_ < N_l; ++i_){
        i = sol.getGlobalIndexTeam(l, i_);
        for (k_ = i_+1; k_ < N_l; ++k_){
            k = sol.getGlobalIndexTeam(l, k_);
            if (sol.getSetting() == Setting::TTP || sol.getSetting() == Setting::Football){
                assert(i == i_);
                assert(k == k_);
            }
            if (sol.MatchColor[i][k] == -1 && sol.isEligible(i,k)){
                if (bipartite && sol.Orientation[i][r] != sol.Orientation[k][r]){
                    if (sol.Orientation[i][r] == HA::H){
                        Adj[i_].push_back(k_);
                    }
                    else {
                        Adj[k_].push_back(i_);
                    }
                }
                else if (!bipartite){
                    Adj[i_].push_back(k_);
                    Adj[k_].push_back(i_);
                    // cout << i << "--" << k << endl;
                }
            }
            else if (sol.MatchColor[i][k] == r){
                if (bipartite){
                    if (sol.Orientation[i][r] == HA::H){
                        Adj[k_].push_back(i_);
                    }
                    else {
                        Adj[i_].push_back(k_);
                    }
                }
            }
        }
    }
        */

    /*
    for (int i = 0; i < N; ++i){
        for (int j = i+1; j < N; ++j){
            if (sol.MatchColor[i][j] == r){
                if (sol.Orientation[i][r] == HA::H){
                    cout << i << " <- " << j << endl;
                }
                else{
                    cout << i << " -> " << j << endl;
                }
            }
        }
    }
    cout << "***********************" << endl;
    */

    clearParent(); // For DFS_Modified, Parent[u] = Parent[u] is the predecessor of u
    clearCycle_AC();
    clearVisited();
    clearStack();
    clearCount();

    int start = gen() % N_l; // ensure some randomness
    for (i = 0; i < N_l; ++i){
        int w = i+start;
        if (w > N_l-1){
            w -= N_l;
        }
        if (bipartite && !Visited[w]){
            if (DFS_cycle(w, l, r)){
                break;
            }
        }
        else if (!bipartite && !Visited[w]){
            if (DFS_Modified(w, l, r)){
                break;
            }
        }
    }

    if (!Cycle_AC.empty()){
        for (i = Cycle_AC.size()-1; i >= 1; --i){ // backwards because alternating cycle with DFS will always be i -- j -> k -- l -> m
            // k = sol.getGlobalIndexTeam(l, Cycle_AC[i-1]), j = sol.getGlobalIndexTeam(l, Cycle_AC[i]);
            k = Cycle_AC[i-1], j = Cycle_AC[i];
            if (sol.MatchColor[k][j] == -1){
                if (sol.Orientation[k][r] == HA::A){
                    // cout << Edges[i][0] << " -- " << Edges[i][1] << endl;
                    AlternatingCycle.emplace_back(k, j);
                }
                else{
                    // cout << Edges[i][1] << " -- " << Edges[i][0] << endl;
                    AlternatingCycle.emplace_back(j, k);
                }
            }
            else{
                if (sol.Orientation[k][r] == HA::H){
                    // cout << Edges[i][0] << " <- " << Edges[i][1] << endl;
                    AlternatingCycle.emplace_back(k, j);
                }
                else{
                    // cout << Edges[i][0] << " -> " << Edges[i][1] << endl;
                    AlternatingCycle.emplace_back(j, k);
                }
            }
        }
        if (sol.MatchColor[AlternatingCycle[0].first][AlternatingCycle[0].second] != -1){
            pair<int,int>E = AlternatingCycle[0];
            for (k = 0; k < AlternatingCycle.size()-1; ++k){
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

bool Operator::DijkstraLineGraph(vector<int>& Pred, vector<int>& Cycle){
    const int source = LineGraph.SOURCE;
    const int sink = LineGraph.SINK;
    priority_queue<StateDijkstra, vector<StateDijkstra>, greater<StateDijkstra>> pq;
    vector<int>Dist(LineGraph.N+2, INF);
    Dist[source] = 0; 
    pq.push(StateDijkstra(source,0));

    int v,d;

    while (!pq.empty()){
        StateDijkstra curr = pq.top();
        pq.pop();
        v = curr.v;

        if (v == sink){
            break;
        }

        if (curr.dist > Dist[v]){
            continue;
        }

        for (int w: LineGraph.Adj[v]){
            // v->w
            assert(LineGraph.Weights[v][w] >= 0);
            d = curr.dist + LineGraph.Weights[v][w];
            if (d < Dist[w]){
                Pred[w] = v;
                Dist[w] = d;
                pq.push(StateDijkstra(w,d));
            }
        }
    }

    if (Dist[sink] == INF) {
        return false; 
    }

    int w = sink;
    // Cycle.push_back(w); -> do not push back, this is a dummy sink
    int it = 0;
    do{
        v = Pred[w];
        w = v;
        if (w != source){
            Cycle.push_back(v);
        }
        else{
            break;
        }
        if (++it > LineGraph.N){
            cout << "infinite loop in dijkstra LG" << endl;
            std::abort();
        }
    }
    while (true);

    return true;
}

bool Operator::SPFALineGraph(vector<int>& Pred, vector<int>& Cycle) {
    // Shortest path faster
    const int source = LineGraph.SOURCE;
    const int N_total = LineGraph.N + 2; // Total nodes including SOURCE and SINK

    std::queue<int> q;
    vector<int> Dist(N_total, INF);
    vector<bool> inQueue(N_total, false);
    vector<int> enqueueCount(N_total, 0);

    // 1. Initialize Source
    Dist[source] = 0; 
    q.push(source);
    inQueue[source] = true;
    enqueueCount[source] = 1;

    int cycleStartNode = -1;

    // 2. Process the Queue
    while (!q.empty()) {
        int v = q.front();
        q.pop();
        inQueue[v] = false;

        for (int w : LineGraph.Adj[v]) {
            int weight = LineGraph.Weights[v][w]; 
            
            if (Dist[v] + weight < Dist[w]) {
                Dist[w] = Dist[v] + weight;
                Pred[w] = v;

                if (!inQueue[w]) {
                    q.push(w);
                    inQueue[w] = true;
                    enqueueCount[w]++;

                    // 3. Negative Cycle Detection!
                    // If a node is relaxed N_total times, we are caught in an infinite negative loop.
                    if (enqueueCount[w] >= N_total) {
                        cycleStartNode = w;
                        break; 
                    }
                }
            }
        }
        if (cycleStartNode != -1) break; // Break out of the while loop early
    }

    // 4. Extract the Negative Cycle
    if (cycleStartNode != -1) {
        int curr = cycleStartNode;
        
        // TRICK: `cycleStartNode` might be on a "stem" leading INTO the cycle, 
        // not in the cycle itself. To guarantee we are inside the cycle, 
        // we walk backwards through Pred N_total times.
        for (int i = 0; i < N_total; ++i) {
            curr = Pred[curr];
        }
        
        // Now `curr` is definitely inside the loop. Walk backwards until we hit it again.
        int cycleNode = curr;
        int next;
        do {
            Cycle.push_back(cycleNode);
            next = Pred[cycleNode];
            cycleNode = next;
        } while (cycleNode != curr);
        
        // The path was traced backwards, so reverse it to get the forward execution order
        // std::reverse(Cycle.begin(), Cycle.end());
        
        return true; // We found a negative cycle!
    }

    return false; // No negative cycle found (Dist and Pred now hold valid shortest paths)
}

void Operator::AddEdgeToLineGraph(const int a, const int h, const int weight){
    LineGraph.Adj[a].push_back(h);
    LineGraph.Weights[a][h] = weight;
    LineGraph.Costs[a][h] = weight;
}


void Operator::MakeLineGraph(const int source, const int sink, const bool NC){

    // Update 07/01/2026: Just the line graph does not work, a cycle or path in the line graph results in a walk or closed walk, respectively, in the original graph
    // But: add N nodes in the line graph and I think it works
    // e.h. (ij)->(j)->(jk) instead of (ij)->(jk)

    // source: team with A game too much, sink: team with H too much

    // cout << "Find path between " << source << " and " << sink << endl;

    int i,j,r,h,a;

    // Original graph:
    /*
    cout << "Original graph: " << endl;
    cout << "--------------------" << endl;
    for (r = 0; r < sol.getNrRounds(); ++r){
        for (i = 0; i < sol.getNrTeams(); ++i){
            j = sol.TeamColorOpp[i][r];
            if (i < j){
                continue;
            }
            if (sol.Orientation[i][r] == HA::H){
                h = i, a = j;
            }
            else{
                assert(sol.Orientation[i][r] == HA::A);
                h = j, a = i;
            }
            cout << h << " <- " << a << endl;
        }
    }
    cout << "--------------------" << endl;
    */

    int index = 0;
    for (r = 0; r < sol.getNrRounds(); ++r){
        for (i = 0; i < sol.getNrTeams(); ++i){
            j = sol.TeamColorOpp[i][r];
            if (i < j){
                continue;
            }
            if (sol.Orientation[i][r] == HA::H){
                h = i, a = j;
            }
            else{
                assert(sol.Orientation[i][r] == HA::A);
                h = j, a = i;
            }
            /*
            if (h != source && a != sink){ // no point in adding the edge a->SOURCE, because SOURCE always must have an outgoing edge   (and SINK always an incoming edge)
            }
            */
            LineGraph.Nodes[index++] = EdgeNode(a,h); // arc goes like a->h
            // cout << "add the node (" << a << "," << h << ")" << endl;
        }
    }
    assert(index == LineGraph.N);

    // cout << "Added all edge nodes" << endl;

    // cout << "Added all the nodes" << endl;

    // Now, evaluate for pair of nodes what the cost will be

    LineGraph.reset();

    int weight, v, w, k, r1, r2;
    for (v = 0; v < LineGraph.N; ++v){
        for (w = v+1; w < LineGraph.N; ++w){
            // cout << "v = " << v << ", w = " << w << endl;
            if (LineGraph.Nodes[v].h == LineGraph.Nodes[w].a){ // e.g. if (i,k) and (k,j), add the arc {(i,k),(k,j)} // i->k->j
                i = LineGraph.Nodes[v].a, j = LineGraph.Nodes[w].h, k = LineGraph.Nodes[v].h;
                a = v, h = w; // v->w
            }
            else if (LineGraph.Nodes[w].h == LineGraph.Nodes[v].a){ // e.g. if (k,i) and (j,k), add the arc {(j,k),(i,j)} // j->k->i
                i = LineGraph.Nodes[w].a, j = LineGraph.Nodes[v].h, k = LineGraph.Nodes[w].h;
                a = w, h = v;
            }
            else{
                continue;
            }
            // cout << "i = " << i << ", k = " << k << ", j = " << j << endl;
            r1 = sol.MatchColor[k][i]; // remember, arc (i,k) = i->k, so k is the home team!
            r2 = sol.MatchColor[j][k]; // remember, arc (k,j) = k->j, so j is the home team!
            
            // Dijkstra requires positive edge weights, so not really the shortest but a non-increasing path 
            weight = CostOrientationSwapTeamiTTP(k,r1,r2);
            if (!NC && weight < sol.CostTTPViolation){
                // if no TTP violations: random weight
                weight = DisWeightLG->Sample();
            }

            // cout << "a = " << a << ", h = " << h << endl;
            AddEdgeToLineGraph(a, h, weight);

            // cout << "add the arc (" << i << "," << k << ") -> (" << k << "," << j << ")" << " with weight " << weight << endl;
        }
        if (LineGraph.Nodes[v].h == sink){
            r1 = sol.MatchColor[LineGraph.Nodes[v].a][sink];
            r2 = sol.MatchColor[sink][source];
            weight = CostOrientationSwapTeamiTTP(sink,r1,r2);
            if (weight < sol.CostTTPViolation){
                weight = DisWeightLG->Sample();
            }
            AddEdgeToLineGraph(v, LineGraph.SINK, weight);
            // cout << "add the arc (" << LineGraph.Nodes[v].a << ", " << LineGraph.Nodes[v].h << ") -> SINK" << endl; 
        }
        else if (LineGraph.Nodes[v].a == source){
            r1 = sol.MatchColor[LineGraph.Nodes[v].h][source];
            r2 = sol.MatchColor[sink][source];
            weight = CostOrientationSwapTeamiTTP(source,r1,r2);
            if (weight < sol.CostTTPViolation){
                weight = DisWeightLG->Sample();
            }
            AddEdgeToLineGraph(LineGraph.SOURCE, v, weight);
            // cout << "add the arc SOURCE -> (" << LineGraph.Nodes[v].a << ", " << LineGraph.Nodes[v].h << ")" << endl;
        }
    }
    if (source == -1 || sink == -1){
        // if no source and sink in line graph (i.e. when we want to find cycles), connect SOURCE to all nodes 
        // and let all nodes go to sink
        for (v = 0; v < LineGraph.N; ++v){
            AddEdgeToLineGraph(v, LineGraph.SINK, 0);
            AddEdgeToLineGraph(LineGraph.SOURCE, v, 0);
        }
    }
    // cout << "added all the edges" << endl;
    return;
}

bool Operator::RetrieveCycleLineGraphFloydWarshall(vector<int>& Cycle, int& delta){
    int start_node = -1;
    int MinWeight = INF;
    int i,h,a,prev;
    for (i = 0; i < LineGraph.N; ++i){
        if (distLG[i][i] < MinWeight){
            start_node = i;
            MinWeight = distLG[i][i];
        }
    }
    if (MinWeight == INF){
        // then no negative cycle
        return false;
    }
    assert(start_node != -1); // there must always exist a cycle, this cycle necessarily has a cost less than INF

    int curr = start_node;
    
    vector<uint8_t>Visited(LineGraph.N, 0); // if cycle has a cost of 0, the algorithm may treat it as a negative cycle!
    // Walk backward until we hit a node for the second time
    while (Visited[curr] == 0) {
        Visited[curr] = 1;
        curr = predLG[start_node][curr];
        
        // Failsafe: if the graph is somehow completely disconnected
        if (curr == -1){
            cout << "failed to find cycle" << endl;
            std::abort();
            return false; 
        }
    }

    int cycle_start = curr;
    int it = 0;
    do {
        Cycle.push_back(curr);
        prev = predLG[start_node][curr];
        delta += LineGraph.Costs[prev][curr];
        curr = prev;
        if (it++ > LineGraph.N){
            std::abort();
        }
    }
    while (curr != cycle_start);
    return true;
}

void Operator::RestoreWeights(const State& curr, const bool NC){
    for (int f: curr.ForbiddenNodes){
        for (int w: LineGraph.Adj[f]){
            LineGraph.Weights[f][w] = LineGraph.Costs[f][w];
            if (NC){
                distLG[f][w] = LineGraph.Costs[f][w];
            }
        }
    }
}

bool Operator::FindCycleLineGraph(const double& current_obj, const bool FindNC){
    // We just want to find any negative cycle, not necessarily the most negative!!
    // This is faster than exploring all states
    // current_obj and delta are fossils because they were needed to compute lower bounds!
    int source = -1, sink = -1, r = -1;
    if (FindNC){
        MakeLineGraph(-1,-1, FindNC); // no source and sink here
    }
    else{
        source = DisN->Sample(), r = DisR->Sample();
        sink = sol.TeamColorOpp[source][r];
        if (sol.Orientation[source][r] == HA::A){
            std::swap(source,sink);
        }
        // if i <- j, then i is the source and j is the sink because .. <- i <- j <- ..
        MakeLineGraph(source, sink, false);
    }
    // cout << "Try to find NC" << endl;
    // cout << "Cost before = " << current_obj << endl;
    vector<int>Pred(LineGraph.N+2,-1);
    vector<int>Cycle;
    queue<State>Queue;
    clearPath();
    // dummy state
    State dummy;
    Queue.push(dummy);
    while (!Queue.empty()){
        State curr = Queue.front();
        Queue.pop();
        std::fill(Pred.begin(), Pred.end(), -1);
        // cout << "Evaluate state with forbidden nodes:" << endl;
        for (int f: curr.ForbiddenNodes){
            // cout << f << ", ";
            for (int w: LineGraph.Adj[f]){
                LineGraph.Weights[f][w] = INF;
                /*
                if (!FindNC){
                    // then we look for a normal MinWeight cycle
                    // set the weights that we use in Floyd Warshall to INF
                    distLG[f][w] = INF;
                }
                    */
            }
        }
        // cout << endl;
        Cycle.clear();
        bool cycle_found = false;
        if (FindNC){
            cycle_found = SPFALineGraph(Pred, Cycle);
        }
        else{
            cycle_found = DijkstraLineGraph(Pred, Cycle);
        }
        if (!cycle_found){
            // cout << "No cycle found when these are forbidden!" << endl;
            RestoreWeights(curr, FindNC);
            continue; // if no cycle: go further with the next one
        }

        // cout << "NC found, check for forbidden pairs" << endl;
        // if NC found: check for forbidden pairs
        std::fill(Count.begin(), Count.end(), 0);
        int ForbiddenNode = -1;
        for (int v: Cycle){
            int i = LineGraph.Nodes[v].h;
            int j = LineGraph.Nodes[v].a;
            if (++Count[i] > 2){
                // cout << i << "is included more than twice in Cycle!!" << endl;
                ForbiddenNode = i;
                break;
            }
            if (++Count[j] > 2){
                // cout << j << "is included more than twice in Cycle!!" << endl;
                ForbiddenNode = j;
                break;
            }
        }
        if (ForbiddenNode == -1){
            // then no forbidden nodes -> valid UB
            // What if we return as soon as we have found an UB?
            // cout << "No forbidden nodes, obj = " << obj << endl;
            // store the best found path
            for (int v: Cycle){
                int i = LineGraph.Nodes[v].h;
                int j = LineGraph.Nodes[v].a;
                path.push_back({i, j, sol.MatchColor[i][j]});
                // cout << i << " <- " << j << endl;
            }
            if (!FindNC){
                path.push_back({source,sink,sol.MatchColor[sink][source]});
            }
            break;
        }
        else{
            // retrieve the two forbidden nodes in the line graph:
            array<int,2>ForbiddenPair;
            int u = 0;
            for (int v: Cycle){
                int i = LineGraph.Nodes[v].h;
                int j = LineGraph.Nodes[v].a;
                if (i == ForbiddenNode){
                    ForbiddenPair[u++] = v;
                }
                else if (j ==  ForbiddenNode){
                    ForbiddenPair[u++] = v;
                }
                if (u >= 2){
                    break;
                }
            }
            // add two states to the queue
            State state1;
            State state2;
            for (int w: curr.ForbiddenNodes){
                state1.ForbiddenNodes.push_back(w);
                state2.ForbiddenNodes.push_back(w);
            } 
            // cout << "The 2 states have a LB = " << state1.LB << endl;
            state1.ForbiddenNodes.push_back(ForbiddenPair[0]);
            state2.ForbiddenNodes.push_back(ForbiddenPair[1]);
            // cout << "add " << ForbiddenPair[0] << " to state 1" << endl;
            // cout << "add " << ForbiddenPair[1] << " to state 2" << endl;
            Queue.push(state1);
            Queue.push(state2);
        }
        RestoreWeights(curr, FindNC);
    }
    if (!path.empty()){
        // cout << "NC cycle found, new best cost = " << UB << endl;
#ifndef NDEBUG
        ReversePath(false, false);
        if (FindNC){
            assert(sol.ComputeTotalCost() <= current_obj);
        }
        ReversePath(false, false);
#endif
        return true;
    }
    // cout << "No NC found" << endl;
    return false;
}

bool Operator::ShortestPathLineGraph(const int source, const int sink, int& delta){

    vector<int>Pred(LineGraph.N+2,-1);
    vector<int>Cycle;

    // while we keep on finding negative cycles: reverse them. Else: find path with minimum number of violations (if one exists)
    // If not, there does not exist any path between those two vertices now that would lead to accepting the lantern
    // Hence, with this approach, we maximize the chances of a lantern getting accepted

    bool NC = false;
    do{
        MakeLineGraph(source, sink, true);
        double obj = sol.ComputeTotalCost();
        // NC = NegativeCycleLineGraph(obj);
    }
    while (NC);

    cout << "No negative cycle anymore!" << endl;
    cin.get();

    // Now Pred is filled, based on this we retrieve the path:
    // retrieve the path -> check for forbidden nodes!!
    clearPath();
    std::fill(Count.begin(), Count.end(), 0);
    int curr = LineGraph.SINK;
    curr = Pred[curr];
    int i,j;
    bool SourceAdded = false;
    bool ForbiddenPairFound = false;
    // cout << "Path: " << endl;
    do{
        i = LineGraph.Nodes[curr].h;
        j = LineGraph.Nodes[curr].a;
        if (Count[i]++ > 2){
            // cout << i << "is included more than twice!!" << endl;
            ForbiddenPairFound = true;
            // return false;
        }
        if (Count[j]++ > 2){
            // cout << j << "is included more than twice!!" << endl;
            ForbiddenPairFound = true;
            // return false;
        }
        cout << i << " <- " << j << endl;
        path.push_back({i, j, sol.MatchColor[i][j]}); // path goes like this <-
        curr = Pred[curr];
    }
    while (curr != LineGraph.SOURCE && LineGraph.Nodes[curr].h != source);

    cin.get();

    delta += ReversePath(true, true);

    return true;
}

/*

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
*/

// ********* LINE GRAPH ******* LINE GRAPH ****** LINE GRAPH ******* LINE GRAPH ****** LINE GRAPH *********** //

// **************** GPTS ***************** //

void Operator::PrintChain(const vector<int>& chain){
    if (sol.Orientation[lantarnGPTS.i][sol.MatchColor[lantarnGPTS.i][chain[0]]] == HA::H){
        cout << lantarnGPTS.i << " <-" << sol.MatchColor[lantarnGPTS.i][chain[0]] << "- ";
    }
    else{
        cout << lantarnGPTS.i << " -" << sol.MatchColor[lantarnGPTS.i][chain[0]] << "-> ";
    }
    for (int q = 0; q < chain.size()-1; ++q){
        if (sol.Orientation[chain[q]][sol.MatchColor[chain[q]][chain[q+1]]] == HA::H){
            cout << chain[q] << " <-" << sol.MatchColor[chain[q]][chain[q+1]] << "- ";
        }
        else{
            cout << chain[q] << " -" << sol.MatchColor[chain[q]][chain[q+1]] << "-> ";
        }
    }
    if (sol.Orientation[lantarnGPTS.j][sol.MatchColor[chain.back()][lantarnGPTS.j]] == HA::H){
        cout << chain.back() << " -" << sol.MatchColor[chain.back()][lantarnGPTS.j] << "-> " << lantarnGPTS.j << endl;
    }
    else{
        cout << chain.back() << " <-" << sol.MatchColor[chain.back()][lantarnGPTS.j] << "- " << lantarnGPTS.j << endl;
    }
}                                             

bool Operator::FindChain(const int i, const int j, const int c_i, const int c_j, vector<int>& chain, vector<vector<uint8_t>>& EdgeIncluded){
    // This function: we are looking for a chain from i to j, starting with color c_i from i, ending with c_j in j
    int c1 = c_i, c2 = c_j;
    // if new color, try to construct a chain!
    int next;
    int prev = i;
    int start = prev;
    do{
        next = sol.TeamColorOpp[prev][c1];
        if (EdgeIncluded[next][prev] || EdgeIncluded[prev][next]){
            return false; // chains need to be edge-disjoint!
        }
        if (next == j && c1 == c_j){
            return true;
        }
        chain.push_back(next);
        EdgeIncluded[next][prev] = 1;
        std::swap(c1,c2);
        prev = next;
    }
    while(next != start);
    return false;
}


void Operator::SwapColorsChainGPTS(const vector<int>& chain, const bool reverse){
    int c_i = sol.MatchColor[lantarnGPTS.i][chain[0]], c_j = sol.MatchColor[chain.back()][lantarnGPTS.j];
    int start, end, step;
    if (reverse){
        start = chain.size()-1;
        end = -1;
        step = -1;
    }
    else{
        start = 0;
        end = chain.size();
        step = 1;
    }
    for (int q = start; q != end; q += step){
        std::swap(sol.TeamColorOpp[chain[q]][c_i], sol.TeamColorOpp[chain[q]][c_j]);
        std::swap(sol.Orientation[chain[q]][c_i], sol.Orientation[chain[q]][c_j]);
        if (q == 0){
           sol.MatchColor[lantarnGPTS.i][chain[q]] = c_j;
           sol.MatchColor[chain[q]][lantarnGPTS.i] = c_j;
        }
        if (q == chain.size()-1){
            sol.MatchColor[lantarnGPTS.j][chain[q]] = c_i;
            sol.MatchColor[chain[q]][lantarnGPTS.j] = c_i;
        }
        if (q > 0 && q % 2 == 1){
            sol.MatchColor[chain[q-1]][chain[q]] = c_i;
            sol.MatchColor[chain[q]][chain[q-1]] = c_i;
        }
        if (q > 0 && q % 2 == 0){
            sol.MatchColor[chain[q-1]][chain[q]] = c_j;
            sol.MatchColor[chain[q]][chain[q-1]] = c_j;
        }
    }
}

void Operator::SwapColorsGPTSLantarn(const bool reverse){
    int c_i, c_j;
    vector<HA>OrientationCopy_i(N, HA::BYE);
    vector<HA>OrientationCopy_j(N, HA::BYE);
    for (int r = 0; r < R; ++r){
        OrientationCopy_i[r] = sol.Orientation[lantarnGPTS.i][r];
        OrientationCopy_j[r] = sol.Orientation[lantarnGPTS.j][r];
    }
    // cout << "New lantarn:" << endl;
    int start, end, step;
    if (reverse){
        start = lantarnGPTS.Chains.size()-1;
        end = -1;
        step = -1;
    }
    else{
        start = 0;
        end = lantarnGPTS.Chains.size();
        step = 1;
    }
    for (int q = start; q != end; q += step){
        auto& chain = lantarnGPTS.Chains[q];
        c_i = sol.MatchColor[lantarnGPTS.i][chain[0]], c_j = sol.MatchColor[chain.back()][lantarnGPTS.j];
        SwapColorsChainGPTS(chain, reverse);
        sol.TeamColorOpp[lantarnGPTS.i][c_j] = chain[0];
        sol.TeamColorOpp[lantarnGPTS.j][c_i] = chain.back();
        sol.Orientation[lantarnGPTS.i][c_j] = OrientationCopy_i[c_i];
        sol.Orientation[lantarnGPTS.j][c_i] = OrientationCopy_j[c_j];
    }
    /*
    for (auto& chain: lantarnGPTS.Chains){
        PrintChain(chain);
    }
        */
}

bool Operator::GPTS(const int i, const int c1, const int c2){
    /*
    Not so efficient implementation of GPTS
    Only consider GPTS with 3 colors -> sufficient to find good solutions?
    */
    // all we need is to a start node
    // sample two random colors:
    // cout << "GPTS: " << endl;
    int StartColor = c1;
    int CurrentColor = c2;
    assert(c1 != c2);
    lantarnGPTS.reset();
    lantarnGPTS.i = i;
    // First chain: arbitrary length
    vector<int>chain;
    vector<vector<uint8_t>>EdgeIncluded0(N, vector<uint8_t>(N,0));
    FindChain(i,-1,c1,c2,chain,EdgeIncluded0);
    // PrintChain(chain);
    // sample a random odd number 
    assert(chain.size() % 2 == 1);
    std::uniform_int_distribution<>dist_j = std::uniform_int_distribution<>(0,0);
    int p = 2*dist_j(gen)+1; // position of j
    lantarnGPTS.j = chain[p];
    // cout << "j = " << lantarnGPTS.j << endl;
    chain.erase(chain.begin() + p, chain.end());
    // PrintChain(chain);
    vector<vector<uint8_t>>EdgeIncluded(N, vector<uint8_t>(N,0));
    EdgeIncluded[lantarnGPTS.i][chain[0]] = 1;
    EdgeIncluded[lantarnGPTS.j][chain.back()] = 1;
    for (int q = 0; q < chain.size()-1; ++q){
        EdgeIncluded[chain[q]][chain[q+1]] = 1;
    }
    lantarnGPTS.Chains.push_back(chain);
    // cout << i << " -" << c1 << "- " << k << " -" << c2 << "- " << j << endl;
    int start,prev,next,NewColor;

    // First, try idea to only construct lantarns of 3 colors
    // Sample a random chain for the next color
    int start_round = DisR->Sample();
    int r;
    for (int r_ = start_round; r_ < start_round+R; ++r_){
        if (r_ >= R){
            r = r_-R;
        }
        else{
            r = r_;
        }
        if (r == StartColor || r == CurrentColor || r == sol.MatchColor[lantarnGPTS.i][lantarnGPTS.j]){
            continue;
        }
        // cout << "try color " << r << endl;
        chain.clear();
        vector<vector<uint8_t>>EdgeIncluded2(N, vector<uint8_t>(N,0));
        for (int a = 0; a < N; ++a){
            for (int b = 0; b < N; ++b){
                EdgeIncluded2[a][b] = EdgeIncluded[a][b];
            }
        }
        if (FindChain(lantarnGPTS.i, lantarnGPTS.j, CurrentColor, r, chain, EdgeIncluded2)){
            // cout << "try middle chain: " << endl;
            // PrintChain(chain);
            vector<int>chain2;
            if (FindChain(lantarnGPTS.i, lantarnGPTS.j, r, StartColor, chain2, EdgeIncluded2)){
                // PrintChain(chain2);
                lantarnGPTS.Chains.push_back(chain);
                lantarnGPTS.Chains.push_back(chain2);
                // cout << "GPTS found!!" << endl;
                return true;
            }
        }
    }
    return false;
}

// **************** GPTS ***************** //



