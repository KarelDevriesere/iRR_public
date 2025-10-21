#include "Operators.h"

using namespace std; 

void PrintEdgeLantarn(Solution& sol, const int i, const int k, const int j, Lantarn& lantarn){
    // k: position in middle, not the team itself!!!
    int c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
    int c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];
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

void PrintLantarn(Solution& sol, Lantarn& lantarn){
    int i = lantarn.i;
    int j = lantarn.j;
    for (int k = 0; k < lantarn.middle.size(); ++k){
        PrintEdgeLantarn(sol, i, k, j, lantarn);
    }
}

// Another option is to swap all the colors: the HAPs of the middle teams do not change, only of i and j
// Next, we find paths to restore the balance but we use the trick 

void KeepOrientation(Solution& sol, Lantarn& lantarn, const int i, const int k, const int j, const vector<vector<HA>>& OrientationsCopy){
    // this can be seen as swapping the colors in the lantarn but not the orientations. 
    // E.g. if we have: 
    // i<-G-s<-B-j
    // i<-R-t<-G-j
    // i<-B-u<-R-j
    // then the new lantarn will be:
    // i<-B-s<-G-j
    // i<-G-t<-R-j
    // i<-R-u<-B-j
    // Hence, the HAPs of all teams change but at least everything stays balanced
    // except if the orientations of i and j next to the possible fictive color do not match, so at most 1 path needs to be found!!
    int c_i, c_j;
    // k: position in middle, not the team itself!!!
    // this + swapping colors: orientations of the middle teams does not change!!!
    c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
    c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];

    if (c_i < 0){
        sol.Orientation[i][c_j] = OrientationsCopy[j][c_j];
    }
    else if (c_j < 0){
        sol.Orientation[j][c_i] = OrientationsCopy[i][c_i];
    }
    else{
        sol.Orientation[i][c_j] = OrientationsCopy[i][c_i]; // TODO: code duplication
        sol.Orientation[j][c_i] = OrientationsCopy[j][c_j];
        std::swap(sol.Orientation[lantarn.middle[k]][c_i], sol.Orientation[lantarn.middle[k]][c_j]);
    }
}

void SwapOrientation(Solution& sol, Lantarn& lantarn, const int i, const int k, const int j, const vector<vector<HA>>& OrientationsCopy){
    // this can be seen as swapping the colors in the lantarn with the orientations. 
    // E.g. if we have: 
    // i<-G-s<-B-j
    // i<-R-t<-G-j
    // i<-B-u<-R-j
    // then the new lantarn will be:
    // i->B-s-G->j
    // i->G-t-R->j
    // i-R->u-B->j
    // Hence, the HAPs of the middle teams stay the same!!
    // Similar to TS

    // k: position in middle, not the team itself!!!
    int c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
    int c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];
    if (c_j >= 0){
        sol.Orientation[i][c_j] = OrientationsCopy[j][c_j];
    }
    if (c_i >= 0){
        sol.Orientation[j][c_i] = OrientationsCopy[i][c_i];
    }
    else{
        std::swap(sol.Orientation[i][c_i], sol.Orientation[j][c_j]);
    }
    // TODO: how to orchestrate the MatchColors???
}

void setMatchColorR(Solution& sol, const int i, const int r, const int s){
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

bool TS_feasible(Solution& sol, const int i, const int j){
    int c, opp_i, opp_j;
    int club_i = sol.getTeamClub(i), club_j = sol.getTeamClub(j);
    int NrSameClub_i = 0, NrSameClub_j = 0;
    for (c = 0; c < sol.getNrRounds(); ++c){
        opp_i = sol.TeamColorOpp[i][c];
        opp_j = sol.TeamColorOpp[j][c];
        if (opp_j == i || opp_i == j){
            continue;
        }
        if (!sol.isEligible(i, opp_j) || !sol.isEligible(j, opp_i)){
            return false;
        }
        if (club_i == sol.getTeamClub(opp_j)){
            NrSameClub_i++;
        }
        if (club_j == sol.getTeamClub(opp_i)){
            NrSameClub_j++;
        }
    }
    if (NrSameClub_i > sol.getMaxSameClub() || NrSameClub_j > sol.getMaxSameClub()){
        return false;
    }
    return true;
}


void TS(Solution& sol, const int i, const int j){
    // Swaps 2 teams
    // std::cout << "Swap teams " << i << " and " << j << std::endl;
    // ALWAYS KEEPS THE BALANCE!!
    // This moves works by swapping the colors on both sides. The orienations are such that the middle teams
    // keep their orientations in each color, hence only the HAPs of i and j change!!
    // Because we swap all the colors and orientations, including i and j
    // the resulting HAPs of i and j will still be balanced
    // However, the HAPs of i and j might break..

    int c, opp_i, opp_j;
    for (c = 0; c < sol.getNrRounds(); ++c){
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

    for (int k = 0; k < sol.getNrTeams(); ++k){
        if (i == k || j == k){
            continue;
        }
        std::swap(sol.MatchColor[i][k], sol.MatchColor[j][k]);
        std::swap(sol.MatchColor[k][i], sol.MatchColor[k][j]);
        assert(sol.ComputeHACostTeam(k) == 0); // test assumption that the HAPs of the middle teams does not change
    }
    std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]); // do this to make the HAPs of i and j balanced!
}

/*
void RS(Solution& sol, const int r, const int s){
    // Swaps 2 rounds
    // std::cout << "Swap rounds " << r << " and " << s << std::endl;
    for (int i = 0; i < sol.NrNodes; ++i){
        setMatchColorR(G, i, r, s);
        std::swap(sol.TeamColorOpp[i][r], sol.TeamColorOpp[i][s]);
        std::swap(sol.Orientation[i][r], sol.Orientation[i][s]);
    }
}
*/

void RS(Solution& sol, const int r, const int s){
    // std::cout << "Partially swap rounds " << r << " and " << s << std::endl;
    // r and s are always real colors!!
    for (int i = 0; i < sol.getNrTeams(); ++i){
        if (!sol.SRR){
            setMatchColorR(sol, i, r, s);
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

void PRS(Solution& sol, const int r, const int s, const int StartNode){
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
        setMatchColorR(sol, next, r, s);
        std::swap(sol.TeamColorOpp[next][r], sol.TeamColorOpp[next][s]);
        std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
        next = sol.TeamColorOpp[next][s];
        // std::cout << "t2: " << next << std::endl;
        setMatchColorR(sol, next, r, s);
        std::swap(sol.TeamColorOpp[next][s], sol.TeamColorOpp[next][r]);
        std::swap(sol.Orientation[next][r], sol.Orientation[next][s]);
        next = sol.TeamColorOpp[next][r];
    }
    while (next != StartNode);
}

void SwapColorsLantarn(Solution& sol, Lantarn& lantarn){

    // Does NOT set the orientations!!
    // Only the opponents

    int i = lantarn.i;
    int j = lantarn.j;
    for (int k = 0; k < lantarn.middle.size(); ++k){
        int c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
        int c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];

        int k_ = lantarn.middle[k];
        
        if (!sol.SRR){
            sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second] = c_j;
            sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second] = c_i; 
        }
        else{
            sol.MatchColor[i][k_] = c_j;
            sol.MatchColor[k_][i] = c_j;
            sol.MatchColor[j][k_] = c_i;
            sol.MatchColor[k_][j] = c_i;
        }

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

    if (!sol.SRR){
        lantarn.i = j;
        lantarn.j = i;
    }
}

pair<int,int> ChooseMatch_kj(Lantarn& lantarn, const Solution& sol, const int k, const int j){
    if (!lantarn.MatchAlreadyPresent[k].first){
        lantarn.MatchAlreadyPresent[k].first = true;
        return {k,j};
    }
    else {
        assert(!lantarn.MatchAlreadyPresent[k].second);
        lantarn.MatchAlreadyPresent[k].second = true;
        return {j,k};
    }
}

void ReplenishLantarn(Solution& sol, const int i, const int j, const int StartColor, Lantarn& lantarn){
    assert(StartColor >= 0);
    assert(i != j);
    int c_i = -1, c_j = StartColor;
    int k; 
    bool InfeasibleColorFound = false;
    do{
        c_i = c_j;
        // std::cout << "c_i = " << c_i << std::endl;
        if (sol.Orientation[i][c_i] == HA::H){
            lantarn.NrH[i]++;
        }
        k = sol.TeamColorOpp[i][c_i]; 
        assert(i != k);
        assert(j != k);

        if (sol.Orientation[i][c_i] == HA::H){
            lantarn.EdgesMatch[i].push_back({i,k});
        }
        else{
            lantarn.EdgesMatch[i].push_back({k,i});
        }

        lantarn.middle.push_back(k);
        // std::cout << "k = " << k << std::endl;
        // pair<int,int>match = ChooseMatch_kj(lantarn, sol, k, j);
        /*
        Take the color with the same orientation as k
        If we already took this match: infeasible
        */

        pair<int,int>match;
        if (sol.Orientation[k][c_i] == HA::H){
            match = {k,j};
        }
        else{
            match = {j,k};
        }
        lantarn.EdgesMatch[j].push_back(match);

        c_j = sol.MatchColor[match.first][match.second];

        assert(c_i != c_j);
        // std::cout << "c_j = " << c_j << std::endl;
        if (c_j >= 0 && sol.Orientation[j][c_j] == HA::H){
            lantarn.NrH[j]++;
        }

        if (c_j < 0){
            InfeasibleColorFound = true;
            lantarn.InfeasibleColor = true;
            lantarn.c_[i] = c_i;
            lantarn.fictive_nb[j] = k;
            if (!sol.isEligible(j,k)){
                lantarn.InfeasibleOpponents = true;
            }
        }
    }
    while(c_j != StartColor && !InfeasibleColorFound);
}

Lantarn CreateLantarn(Solution& sol, const int i, const int j, const int StartColor){
    assert(StartColor >= 0);
    Lantarn lantarn;
    lantarn.i = i;
    lantarn.j = j;
    // cout << "i = " << i << ", j = " << j << endl;
    lantarn.NrH[i] = 0;
    lantarn.NrH[j] = 0;
    lantarn.EdgesMatch[i] = vector<pair<int,int>>();
    lantarn.EdgesMatch[j] = vector<pair<int,int>>();
    ReplenishLantarn(sol, i, j, StartColor, lantarn);
    if (lantarn.InfeasibleColor && !lantarn.InfeasibleOpponents){
        ReplenishLantarn(sol, j, i, StartColor, lantarn);
    }
    return lantarn;
}

bool MaxSameClubViolated(Solution& sol, Lantarn& lantarn){
    if (!lantarn.InfeasibleColor){
        return false;
    }
    int i = lantarn.i, j = lantarn.j;
    int club_j = sol.getTeamClub(sol.TeamColorOpp[i][lantarn.c_[i]]);
    int club_i = sol.getTeamClub(sol.TeamColorOpp[j][lantarn.c_[j]]);
    if (club_i != club_j){
        // cout << "Test " << i << " and " << j << endl;
        int nr_i = 1, nr_j = 1;
        for (int r = 0; r < sol.getNrRounds(); ++r){
            if (r != lantarn.c_[i] && sol.getTeamClub(sol.TeamColorOpp[i][r]) == club_i){
                nr_i++;
            }
            if (r != lantarn.c_[j] && sol.getTeamClub(sol.TeamColorOpp[j][r]) == club_j){
                nr_j++;
            }
        }
        if (nr_i > sol.getMaxSameClub() || nr_j > sol.getMaxSameClub()){
            // cout << "Max same club violated" << endl;
            // cin.get();
            return true;
        }
    }
    // Now test the same for the middle teams!!
    club_i = sol.getTeamClub(i), club_j = sol.getTeamClub(j);
    int k1 = sol.TeamColorOpp[i][lantarn.c_[i]];
    int k2 = sol.TeamColorOpp[j][lantarn.c_[j]];
    if (club_i != club_j){
        // cout << "Test " << k1 << " and " << k2 << endl;
        int nr_i = 1, nr_j = 1;
        for (int r = 0; r < sol.getNrRounds(); ++r){
            if (r != lantarn.c_[i] && sol.getTeamClub(sol.TeamColorOpp[k1][r]) == club_j){
                nr_i++;
            }
            if (r != lantarn.c_[j] && sol.getTeamClub(sol.TeamColorOpp[k2][r]) == club_i){
                nr_j++;
            }
        }
        if (nr_i > sol.getMaxSameClub() || nr_j > sol.getMaxSameClub()){
            // cout << "Max same club violated" << endl;
            // cin.get();
            return true;
        }
    }
    return false;
}


int ComputeCostLantarn(Solution& sol, Lantarn& lantarn, vector<int>& path){
    const int i = lantarn.i;
    const int j = lantarn.j;
    int cost = sol.ComputeHACostTeam(i) + sol.ComputeHACostTeam(j);
    for (auto& k: lantarn.middle){
        cost += sol.ComputeHACostTeam(k);
    }
    for (auto& k: path){
        cost += sol.ComputeHACostTeam(k);
    }
    // + Cost capacities!!
    return cost;
}

void KeepOrientationsAllEdgesLantarn(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy){
    // for DRR
    const int i = lantarn.i;
    const int j = lantarn.j;
    for (int k = 0; k < lantarn.middle.size(); ++k){
        KeepOrientation(sol, lantarn, i, k, j, OrientationsCopy);
    }
}

bool FindPath(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy, const bool SingleEdge, vector<array<int,3>>& path, const bool MinCostP){
    const int i = lantarn.i;
    const int j = lantarn.j;
    int h,a,h_nb,a_nb;
    bool PathFound = false;
    for (int option = 0; option < 1; ++option){
        if (sol.Orientation[i][lantarn.c_[i]] == HA::H){
            // j -- k -> i
            // j -> l -- i
            if (option == 0){
                // results in
                //      j <- k -- i
                //      j -- l <- i
                a = i, h = j;
                a_nb = lantarn.fictive_nb[i], h_nb = lantarn.fictive_nb[j];
            }
            else{
                // results in
                //      j -> k -- i
                //      j -- l -> i
                a = lantarn.fictive_nb[j], h = lantarn.fictive_nb[i]; // fictive nb j: e.g. i <- k -- j
            }
        }
        else{
            if (option == 0){
                a = j, h = i;
                a_nb = lantarn.fictive_nb[j], h_nb = lantarn.fictive_nb[i];
            }
            else{
                a = lantarn.fictive_nb[i], h = lantarn.fictive_nb[j];
            }
        }
        if (SingleEdge && sol.MatchColor[h][a] >= 0 && sol.MatchColor[a][h] < 0){
            // cout << "Possible to switch orientation of arc (" << h << "," << a << ") with option = " << option << endl;
            PathFound = true;
            path.emplace_back(std::array<int, 3>{h, a, sol.MatchColor[h][a]});
            // Reverse h<-a to a->h
            // All HAP costs can be computed based on orientations so only swap orientations
            std::swap(sol.Orientation[h][sol.MatchColor[h][a]], sol.Orientation[a][sol.MatchColor[h][a]]);
            std::swap(sol.MatchColor[h][a], sol.MatchColor[a][h]); 
            path.reserve(3);
        }
        else if (!SingleEdge){
            // cout << "try to find path from " << a << " to " << h << endl;
            int delta = 0; // useless for now!!
            if (/*FindZeroCostPathLineGraph(a, h, l, sol)*/FindNormalPathOneLeague(a,h,sol,path, delta, MinCostP)){
                PathFound = true;
            }
            else{
                PathFound = false;
            }
            // cin.get();
        }
        if (PathFound){
            // Next, the lantarn arcs
            for (int k = 0; k < lantarn.middle.size(); ++k){
                if (k == lantarn.fictive_nb[j] && option == 1){
                    int c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
                    sol.Orientation[j][c_i] = OrientationsCopy[i][c_i];
                }
                else if (k == lantarn.fictive_nb[i] && option == 1){
                    int c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];
                    sol.Orientation[i][c_j] = OrientationsCopy[j][c_j];
                }
                else{
                    KeepOrientation(sol, lantarn, i, k, j, OrientationsCopy);
                }
            }
            break;
        }
        else{
            // cout << "color of h<-a = " << sol.MatchColor[h][a] << endl;
            // cout << "color of a->h = " << sol.MatchColor[a][h] << endl;
        }
    }
    return PathFound;
}


bool RepairOrientationsEdgesLantarn_CM(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy, vector<array<int,3>>& path, const bool MinCostP, const bool CM){
    // Assumes orientations are already reversed!!
    // PrintLantarn(sol, lantarn);
    const int i = lantarn.i;
    const int j = lantarn.j;
    if (!sol.IsTeamBalanced(i)){
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
        for (int t = 0; t < sol.getNrTeams(); ++t){
            if (t != i && t != j){
                assert(sol.IsTeamBalanced(t));
            }
        }
#endif
        // Now, everything is balanced except for i and j
        // Fix this by finding a path between them!
        int delta = 0; // we will not calculate delta here
        // cout << "try to find path from " << source << " to " << sink << endl;
        if (!FindNormalPathOneLeague(source, sink, sol, path, delta, MinCostP)){
            return false;
        }
        // Now, everyone should be balanced!!
#ifndef NDEBUG
        for (int t = 0; t < sol.getNrTeams(); ++t){
            assert(sol.IsTeamBalanced(t));
        }
#endif
    }
    return true;
}


bool SetOrientationsEdgesLantarn(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy, vector<array<int,3>>& path, const bool MinCostP, const bool CM){
    // First, test whether lantarn can be kept balanced
    // PrintLantarn(sol, lantarn);
    if (!lantarn.InfeasibleColor){
        // if not fictive color, lantarn can be kept balanced by keeping the orientations of all edges
        KeepOrientationsAllEdgesLantarn(sol, lantarn, OrientationsCopy);
        cout << "no fictive color" << endl;
        cin.get();
    }
    else{
        const int i = lantarn.i;
        const int j = lantarn.j;
        if (sol.Orientation[i][lantarn.c_[i]] == sol.Orientation[j][lantarn.c_[j]]){
            // in this case, keeping the orientations of all edges guaranteed balancedness
            KeepOrientationsAllEdgesLantarn(sol, lantarn, OrientationsCopy);
            cout << "orientations fictive color same" << endl;
            cin.get();
        }
        else{
            if (CM){
                RepairOrientationsEdgesLantarn_CM(sol, lantarn, OrientationsCopy, path, MinCostP, CM);
            }
            else{
                // If not, we have to reverse another edge in the lantarn.
                // Look if we can find such an edge
                int MiddleTeamFound = -1;
                int c_i, c_j;
                for (int k = 0; k < lantarn.middle.size(); ++k){
                    c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
                    c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];
                    if (c_i < 0 || c_j < 0){
                        continue;
                    }
                    else if (sol.Orientation[lantarn.middle[k]][c_i] != sol.Orientation[lantarn.middle[k]][c_j]){
                        if (sol.Orientation[lantarn.middle[k]][c_i] == sol.Orientation[i][lantarn.c_[i]]){
                            MiddleTeamFound = k;
                            break;
                        }
                    }
                }
                if (MiddleTeamFound != -1){
                    // cout << "Suitable edge found that can be reversed, do this for middle team " << MiddleTeamFound << endl;
                    // cin.get();
                    for (int k = 0; k < lantarn.middle.size(); ++k){
                        if (k != MiddleTeamFound){
                            KeepOrientation(sol, lantarn, i, k, j, OrientationsCopy);
                        }
                        else{
                            SwapOrientation(sol, lantarn, i, k, j, OrientationsCopy);
                        }
                    }
                }
                else{
                    // First try to switch edge if this exists
                    // cout << "No suitable edge found that can be reversed within lantarn, try outside lantarn to reverse 1 edge" << endl;
                    bool SingleEdge = true;
                    if (!FindPath(sol, lantarn, OrientationsCopy, SingleEdge, path, MinCostP)){
                        // cout << "No single edge switch possible" << endl;
                        return false;
                        /*
                        SingleEdge = false;
                        // PrintLantarn(sol, lantarn);
                        if (!FindPath(sol, lantarn, OrientationsCopy, SingleEdge, path)){
                            return false;
                        }
                            */
                    }
                }
            }
        }
    }
    return true;
}

int ComputeWeightLineGraph(Solution& sol, const pair<int,int>arc1, const pair<int,int>arc2, const int NrNodes, const bool OneTeamPerClub){
    // Reverse orientations of arc1 and arc2 in their respective colors
    assert(arc1.second == arc2.first);
    int c, i, j;
    int total_cost_before = sol.ComputeTotalHACost();
    int cost_before = sol.ComputeHACostTeam(arc1.second);
    assert(cost_before == 0);
    int cost_after = 0, cost_cap = 0;

    for (auto& arc: {arc1, arc2}){
        j = arc.first, i = arc.second;
        c = sol.MatchColor[i][j];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
        if (arc1.first != arc2.second){ 
            std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
        }
    }
    if (arc1.first == arc2.second){ 
        std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
    }
    cost_after = sol.ComputeHACostTeam(arc1.second);
    cin.get();
    for (auto& arc: {arc1, arc2}){
        j = arc.first, i = arc.second;
        c = sol.MatchColor[j][i];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
        if (arc1.first != arc2.second){ 
            std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
        }
    }
    if (arc1.first == arc2.second){ 
        std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
    }
    if (OneTeamPerClub){
        cost_cap = sol.CostCapacityClubHapSwitchTeam(arc1.second, sol.MatchColor[arc1.second][arc1.first]) + sol.CostCapacityClubHapSwitchTeam(arc1.second, sol.MatchColor[arc2.second][arc2.first]);
    }

    assert(total_cost_before == sol.ComputeTotalHACost()); // check if everything went well!!!
    if (cost_after == 0 && cost_cap <= 0){
        return 0;
    }
    else{
        return NrNodes;
    }
}

tuple<vector<Edge>,vector<Edge>,vector<int>> MakeLineGraphOnlyZeroWeightEdges(Solution& sol, const int source, const int sink, const int source_nb, const int sink_nb){
    const int l = sol.getLeagueTeam(source);
    const bool OneTeamPerClub = false; // parameter, turn this on or off
    vector<bool>ClubPresent(sol.getNrClubs(), false);
    vector<Edge>Nodes;
    int i,c,club_i;
    vector<int>TeamsPresent;
    for (int i_ = 0; i_ < sol.getNrTeamsLeague(l); ++i_){
        i = sol.getTeamsLeague(l)[i_];
        club_i = sol.getTeamClub(i);
        if (OneTeamPerClub && ClubPresent[club_i]){
            continue;
        }
        TeamsPresent.push_back(i);
        ClubPresent[club_i] = true;
    }

    for (auto& i: TeamsPresent){
        for (auto& j: TeamsPresent){
            c = sol.MatchColor[i][j];
            if (c >= 0){ // no point in adding the edge j->SOURCE, because SOURCE always must have an outgoing edge (and SINK always an incoming edge)
                assert(sol.Orientation[i][c] == HA::H);
                assert(sol.Orientation[j][c] == HA::A);
                if (i != source && j != sink && sol.MatchColor[j][i] < 0){ // otherwise, we violate DRR constraint: then this node cannot be reversed!!
                    Nodes.push_back({j,i}); // make for each arc a node
                    // remember, arc is j->i, so in arc {j,i} i is the home team
                    // cout << "add the node (" << j << "," << i << ")" << endl; // add node per round?
                }
            }
        }
    }
    cout << "Added all the nodes" << endl;

    // Now, evaluate for pair of nodes what the cost will be
    vector<Edge>edge_vector;
    vector<int>weights;

    int weight;
    for (int v = 0; v < Nodes.size(); ++v){
        for (int w = v+1; w < Nodes.size(); ++w){
            if (Nodes[v].second == Nodes[w].first){ // e.g. if (i,k) = (k,j), add the edge {(i,k),(k,j)}
                weight = ComputeWeightLineGraph(sol, Nodes[v], Nodes[w], (int)Nodes.size(), OneTeamPerClub);
                if (weight == 0){
                    edge_vector.push_back(Edge(v, w));
                    weights.push_back(weight);
                    cout << "add the arc (" << Nodes[v].first << "," << Nodes[v].second << ") -> (" << Nodes[w].first << "," << Nodes[w].second << ")" << endl;
                }
                else{
                    cout << "The arc (" << Nodes[v].first << "," << Nodes[v].second << ") -> (" << Nodes[w].first << "," << Nodes[w].second << ") has weight > 0" << endl;
                }
                /*
                if (weight <= 0){
                    cout << "give the arc (" << Nodes[v].first << "," << Nodes[v].second << ") -> (" << Nodes[w].first << "," << Nodes[w].second << ") weight " << weight << endl;
                }
                    */
            }
            else if (Nodes[w].second == Nodes[v].first){ // e.g. if (k,i) = (j,k), add the edge {(i,k),(k,j)}
                weight = ComputeWeightLineGraph(sol, Nodes[w], Nodes[v], (int)Nodes.size(), OneTeamPerClub);
                if (weight == 0){
                    edge_vector.push_back(Edge(w, v));
                    weights.push_back(weight);
                    cout << "add the arc (" << Nodes[w].first << "," << Nodes[w].second << ") -> (" << Nodes[v].first << "," << Nodes[v].second << ")" << endl;
                }
                else{
                    cout << "The arc (" << Nodes[w].first << "," << Nodes[w].second << ") -> (" << Nodes[v].first << "," << Nodes[v].second << ") has weight > 0" << endl;
                }
                /*
                if (weight <= 0){
                    cout << "give the arc (" << Nodes[w].first << "," << Nodes[w].second << ") -> (" << Nodes[v].first << "," << Nodes[v].second << ") weight " << weight << endl;
                }
                    */
            }
            // otherwise, edges are not incident to same node
        }
    }
    cout << "added all the edges" << endl;
    cin.get();
    return {Nodes, edge_vector, weights};
}

bool FindNormalPathOneLeague(const int source, const int sink, Solution& sol, vector<array<int,3>>& path, int& delta, const bool MinCostP){ // source: A too much, sink: H too much
    int i,j,r;
    const int N = sol.getNrTeams();
    BGraph G(N);
    // cout << "source = " << source << endl;
    // cout << "sink = " << sink << endl;

    int cost = 0;
    for (i = 0; i < sol.getNrTeams(); ++i){
        for (j = i+1; j < sol.getNrTeams(); ++j){
            if (!sol.SRR){
                if (sol.MatchColor[i][j] >= 0 && sol.MatchColor[j][i] < 0){
                    boost::add_edge(j, i, 1, G); // remember, edge goes like j->i
                    // cout << "add edge " << i << "<-" << j << endl;
                }
                if (sol.MatchColor[j][i] >= 0 && sol.MatchColor[i][j] < 0){
                    boost::add_edge(i, j, 1, G);
                    // cout << "add edge " << i << "->" << j << endl;
                }
            }
            else{
                r = sol.MatchColor[i][j];
                assert(sol.MatchColor[j][i] == r);
                if (r >= 0){
                    cost = 1;
                    if (sol.Orientation[i][r] == HA::H){
                        if (MinCostP){
                            cost = sol.getCostMatchRound(j,i,r); // on edge i<-j we stick the cost of doing i->j
                        }
                        boost::add_edge(j, i, cost, G);
                    }
                    else{
                        if (MinCostP){
                            cost = sol.getCostMatchRound(i,j,r);
                        }
                        boost::add_edge(i, j, cost, G);
                    }
                }
            }
        }
    }

    std::vector< Vertex > p(N);
    std::vector< int > d(N);
    Vertex s = vertex(source, G);

    dijkstra_shortest_paths(G, s, 
        predecessor_map(boost::make_iterator_property_map(p.begin(), 
            get(boost::vertex_index, G))).distance_map(boost::make_iterator_property_map(d.begin(), 
                get(boost::vertex_index, G))));
    
    int v_ = sink;
    if (p[v_] == v_){
        // cout << "No path found" << endl;
        return false; // this means that the vertex could not be reached
    }

    // cout << "path found:" << endl;
    // cout << v_ << " <- ";
    while (true){
        // cout << p[v_] << " <- ";
        // p[v_]->v_ because we start with v_ as the sink (=H)
        r = sol.MatchColor[v_][p[v_]];
        path.emplace_back(std::array<int, 3>{v_, (int)p[v_], r});
        assert(sol.Orientation[v_][r] == HA::H);
        assert(sol.Orientation[p[v_]][r] == HA::A);
        delta += (sol.getCostMatchRound(p[v_], v_, r) - sol.getCostMatchRound(v_, p[v_], r));
        // cout << "Reverse " << v_ << "<-" << p[v_] << " to " << v_ << "->" << p[v_] << " in " << r << " : " << (sol.getCostMatchRound(p[v_], v_, r) - sol.getCostMatchRound(v_, p[v_], r)) << endl;
        if (source == p[v_]){
            break;
        }
        else{
            v_ = p[v_];
        }
    }

    ReversePath(sol, path);

    return true;
}

bool FindZeroCostPathLineGraph(const int source, const int sink, const int source_nb, const int sink_nb, const int l, Solution& sol){ // source: team with A too much, sink: team with H too much

    auto [Nodes,edge_vector,weights] = MakeLineGraphOnlyZeroWeightEdges(sol, source, sink, source_nb, sink_nb);

    // Now, create the real source and sink nodes
    // In line graph, nodes are of the form ({j,k}), with j->k

    int SOURCE = Nodes.size();
    int SINK = Nodes.size()+1;
    for (int v = 0; v < Nodes.size(); ++v){
        if (Nodes[v].first == source){
            edge_vector.push_back(Edge(SOURCE, v)); // create edges SOURCE --> v
            weights.push_back(0); // get weight of 0
        }
        else if (Nodes[v].second == sink){
            edge_vector.push_back(Edge(v, SINK)); // create edges SOURCE --> v
            weights.push_back(0); // get weight of 0
        }
    }

    BGraph G(Nodes.size()+2); // Nodes.size()+2 vertices, +2 bc of extra SOURCE and SINK node

    for (std::size_t j = 0; j < edge_vector.size(); ++j){
        boost::add_edge(edge_vector[j].first, edge_vector[j].second, weights[j], G);
    }

    const int N = num_vertices(G);

    std::vector< Vertex > p(N);
    std::vector< int > d(N);
    Vertex s = vertex(SOURCE, G);

    dijkstra_shortest_paths(G, s, 
        predecessor_map(boost::make_iterator_property_map(p.begin(), 
            get(boost::vertex_index, G))).distance_map(boost::make_iterator_property_map(d.begin(), 
                get(boost::vertex_index, G))));

    vector<int>path;
    
    int v_ = SINK;
    if (p[v_] == v_){
        cout << "No positive path found" << endl;
        return false; // this means that the vertex could not be reached
    }
    else { // because line graph only contains edges of cost 0
        assert(d[v_] == 0);
    }

    path.push_back(v_);
    while (SOURCE != p[v_]){
        path.push_back(p[v_]);
        v_ = p[v_];
    }
    path.push_back(SOURCE);

    ReversePathLineGraph(sol, path, Nodes);

    return true;
}

void ReversePathLineGraph(Solution& sol, const vector<int>& path, const vector<Edge>& Nodes){
    Edge e;
    int i,j,c;
    // cout << "path is" << endl;
    assert(path.size() > 2); // must at least be SOURCE->v->SINK, with v representing 2 edges incident to same vertex
    assert(path.front() == Nodes.size());
    assert(path.back() == Nodes.size()+1);
    for (int v = 1; v < path.size()-1; ++v){
        // path is traversed from sink to source (excluding the SOURCE and the SINK)
        // sink: +A, source: +H
        for (auto& w: {v,v+1}){
            e = Nodes[path[w]];
            i = e.second, j = e.first;
            c = sol.MatchColor[i][j];
            // the assert below does not work for balanced cycle, I do not understand why..
            // cout << "swap orientation of " << i << "<-" << j << endl;
            assert(sol.Orientation[i][c] == HA::H && sol.Orientation[j][c] == HA::A);
            std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
        }
        // cout << i << " <- ";
    }
    // cout << path[path.size()-1] << endl;
}

void ReversePath(Solution& sol, const vector<array<int,3>> path){
    int i,j,c;
    // cout << "path is" << endl;
    for (int e = 0; e < path.size(); ++e){
        // path is traversed from sink to source
        // sink: +H, source: +A
        i = path[e][0], j = path[e][1], c = path[e][2];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);

        if (!sol.SRR){
            std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
        }

        // cout << i << " <- ";
    }
    // cout << path.back() << endl;
}


vector<array<int,3>> CycleBalanced(Solution& sol, std::mt19937& gen){
    // In principe zou deze genoeg moeten zijn want ik kan van eender welk balanced schedule naar eender ander balanced schedule gaan
    // Do this one if we just want to find a cycle in a balanced schedule
    // Given the cycle, calculate the delta afterwards
    // We know that in a balanced schedule, if we just do a random path, we always end up at a node already in the path

    int N = sol.getNrTeams();
    int C = sol.getNrRounds();
    std::uniform_int_distribution<>dist_team = std::uniform_int_distribution<>(0,N-1);
    std::uniform_int_distribution<>dist_colour = std::uniform_int_distribution<>(0,C-1);
    int i = dist_team(gen);
    int j,c;
    vector<bool>NodeVisited(N, false);
    vector<array<int,3>>Cycle;
    Cycle.reserve(N);
    // cout << "Path: " << endl;
    // cout << i;
    while (!NodeVisited[i]){
        assert(sol.IsTeamBalanced(i));
        NodeVisited[i] = true;
        c = dist_colour(gen);
        int start_color = c;
        while (sol.Orientation[i][c] != HA::H){
            assert(sol.Orientation[i][c] == HA::A);
            c = (c + 1)%C;
            if (c == start_color){
                throw std::runtime_error("Infinite loop in CycleBalanced!!"); 
            }
        }
        j = sol.TeamColorOpp[i][c];
        assert(sol.Orientation[j][c] == HA::A);
        Cycle.emplace_back(std::array<int, 3>{i,j,c});
        i = j;
        // cout << " <- " << i;
    }
    // cout << endl;
    // i has been visited twice
    int e = 0;
    if (Cycle[e][0] != i){
        while(Cycle[e][1] != i){
            ++e;
        }
        // cout << "e = " << e << endl;
        Cycle.erase(Cycle.begin(), Cycle.begin()+e+1);
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

    return Cycle;
}

void SwapMatchings(Solution& sol, vector<pair<int,int>>Matching, const int l, const int r, const bool bipartite){
    // cout << "SWAP MATCHINGS" << endl;
    const int N = sol.getNrTeams();
    vector<bool>NodeSeen(N, false);
    int i,j,h,a;
    // cout << "Old matching: " << endl;
    for (auto& i: sol.getTeamsLeague(l)){ 
        if (!NodeSeen[i]){
            j = sol.TeamColorOpp[i][r];
            NodeSeen[j] = true;
            // if match is still in matching, then it will be overwritten to the right color later
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
    }

    // cout << "New matching:" << endl;
    // Bipartite: behoudt orientations
    // Normal Matching: geef random orientations
    for (int m = 0; m < Matching.size(); ++m){
        i = Matching[m].first, j = Matching[m].second;
        if (bipartite){
            if (sol.Orientation[i][r] == HA::H){
                assert(sol.Orientation[j][r] == HA::A);
                h = i, a = j;
            }
            else{
                assert(sol.Orientation[i][r] == HA::A);
                assert(sol.Orientation[j][r] == HA::H);
                h = j, a = i;
            }
        }
        else{
            h = i, a = j; // this has already been done in MoveMWPM!!
            /*
            if (sol.Orientation[i][r] != sol.Orientation[j][r]){
                // then, nothing: orientation of the teams can stay the same
                // But, for 2RR: we need to check something additionally
                if (!sol.SRR && sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A && sol.MatchColor[i][j] >= 0 && sol.MatchColor[i][j] != r){
                    h = j, a = i;
                    sol.Orientation[i][r] = HA::A;
                    sol.Orientation[j][r] = HA::H;
                    sol.MatchColor[j][i] = r;
                }
                else if (!sol.SRR && sol.Orientation[j][r] == HA::H && sol.Orientation[i][r] == HA::A && sol.MatchColor[j][i] >= 0 && sol.MatchColor[j][i] != r){
                    h = i, a = j;
                    sol.Orientation[i][r] = HA::H;
                    sol.Orientation[j][r] = HA::A;
                    sol.MatchColor[i][j] = r;
                }
                else{
                    if (sol.Orientation[i][r] == HA::H){
                        assert(sol.Orientation[j][r] == HA::A);
                        h = i, a = j;
                    }
                    else{
                        assert(sol.Orientation[i][r] == HA::A);
                        assert(sol.Orientation[j][r] == HA::H);
                        h = j, a = i;
                    }
                }
            }
            else{
                if (sol.MatchColor[i][j] >= 0){
                    a = i, h = j;
                    sol.Orientation[i][r] = HA::A;
                    sol.Orientation[j][r] = HA::H;
                    sol.MatchColor[j][i] = r;
                }
                else{
                    a = j, h = i;
                    sol.Orientation[i][r] = HA::H;
                    sol.Orientation[j][r] = HA::A;
                    sol.MatchColor[i][j] = r;
                }
            }
                */
        }
        // Problem with normal matching and 2RR is that later we maybe change matchcolor again!!
        SetValueCircleMethod(h, a, r, sol);
        sol.Orientation[h][r] = HA::H;
        sol.Orientation[a][r] = HA::A;
        // cout << "match " << h << " vs " << a << " = " << r << endl;
        // cout << "give match " << i << ", " << j << " with dist " << sol.getDistanceTeams(i,j) << " color " << r << endl;
    }
}

vector<bool> SelectTeamsMatching(Solution& sol, const int l, const int r, const int SizeMatching){
    int N = sol.getNrTeams();
    const int NrTeamsToSelect = 2*SizeMatching;
    vector<bool>Selected(N, false);
    int m = 0;
    int i,j;
    for (int i_ = 0; i_ < sol.getNrTeamsLeague(l); ++i_){
        i = sol.getTeamsLeague(l)[i_];
        if (!Selected[i]){
            j = sol.TeamColorOpp[i][r];
            Selected[i] = true;
            Selected[j] = true;
            m += 2;
        }
        if (m >= NrTeamsToSelect){
            return Selected;
        }
        /*
        for (int j_ = i_+1; j_ < sol.getNrTeamsLeague(l); ++j_){
            i = sol.getTeamsLeague(l)[i_], j = sol.getTeamsLeague(l)[j_];
            if (r == sol.MatchColor[i][j]){
                Selected[i] = true;
                Selected[j] = true;
                m += 2;
                if (m >= NrTeamsToSelect){
                    return Selected;
                }
            }
        }
        */
    }
    return Selected;
}


bool ForbiddenEdge(const int i, const int j, const int r, const bool bipartite, const vector<bool>& TeamSelected, Solution& sol){
    const int H = sol.getNrRounds()/2;
    if (!sol.isEligible(i,j)){
        return true;
        // ForbiddenEdge[i][j] = true;
    }
    else if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A && sol.MatchColor[i][j] >= 0 && r != sol.MatchColor[i][j]){
        // Dot not set matches in r as forbidden so that we can always go back to the original solution!!
        // ForbiddenEdge[i][j] = true; // DRR: forbid any game from happening twice
        // cout << "forbid " << i << " vs " << j << endl;
        assert(sol.TeamColorOpp[i][sol.MatchColor[i][j]] == j);
        assert(sol.TeamColorOpp[j][sol.MatchColor[i][j]] == i);
        return true;
    }
    else if (sol.Orientation[i][r] == HA::A && sol.Orientation[j][r] == HA::H && sol.MatchColor[j][i] >= 0 && r != sol.MatchColor[j][i]){
        assert(sol.TeamColorOpp[i][sol.MatchColor[j][i]] == j);
        assert(sol.TeamColorOpp[j][sol.MatchColor[j][i]] == i);
        return true;
    }
    else {
        // Then i and j are elgible and their match either does not happen or happens in round r
        if (!sol.SRR){
            if (r < H && sol.MatchColor[j][i] < H && sol.MatchColor[j][i] >= 0){
                // forbid the game if (j,i) happended in one of the other games this half
                // ForbiddenEdge[i][j] = true;
                return true;
            }
            else if (r >= H && sol.MatchColor[j][i] >= H){
                // ForbiddenEdge[i][j] = true;
                return true;
            }
        }
        if (bipartite){
            if (sol.Orientation[i][r] == sol.Orientation[j][r]){ // this makes the graph bipartite!!
                // ForbiddenEdge[i][j] = true;
                // ForbiddenEdge[j][i] = true;
                return true;
            }
            // Example: we want to construct a matching for round 5, the match (0,19) happened in round 1
            // If now 19 plays A and 0 plays 0 in round 5, then without this forbidden edge we would select (19,0), but this game already happened!
            // Remember: matching works with edges, not with arcs
            
            /*
            if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A){
                // ForbiddenEdge[j][i] = true;
                continue;
            }
            else 
            */
            /* JUST REMOVED THIS
            if (sol.MatchColor[j][i] >= 0 && sol.MatchColor[j][i] != r && sol.Orientation[j][r] == HA::H && sol.Orientation[i][r] == HA::A){
                // ForbiddenEdge[i][j] = true;
                continue;
            }
                */
        }
        else{
            // TeamSelected: if we do matching on the subset of the teams
            if (!TeamSelected[i] || !TeamSelected[j]){
                // ForbiddenEdge[i][j] = true;
                // ForbiddenEdge[j][i] = true;
                return true;
            }
        }
    }
    
    if (sol.getTeamClub(i) == sol.getTeamClub(j)){ // extra: teams can only play at max nr of times vs teams from the same club
        int minus_i = 0, minus_j = 0;
        if (sol.TeamColorOpp[i][r] != -1 && sol.getTeamClub(i) == sol.getTeamClub(sol.TeamColorOpp[i][r])){
            minus_i = -1;
        }
        if (sol.TeamColorOpp[j][r] != -1 && sol.getTeamClub(j) == sol.getTeamClub(sol.TeamColorOpp[j][r])){
            minus_j = -1;
        }
        // We subtract with -1 if in the current round, i or j also plays against a team from the same club
        // This because getNrSameClub() still returns the number under the old solution
        // In that case, we can play against another team from the same club without problem!!
        if (sol.getNrSameClub(i)+1+minus_i > sol.getMaxSameClub() || sol.getNrSameClub(j)+1+minus_j > sol.getMaxSameClub()){
            // ForbiddenEdge[i][j] = true;
            // ForbiddenEdge[j][i] = true;
            return true;
        }
    }
    return false;
}

bool ForbiddenEdge_CM(const int i, const int j, const int r, const bool bipartite, const vector<bool>& TeamSelected, Solution& sol){
    if (sol.MatchColor[i][j] >= 0 && sol.MatchColor[i][j] != r){
        return true;
        // ForbiddenEdge[i][j] = true;
    }
    else if (sol.MatchColor[j][i] >= 0 && sol.MatchColor[j][i] != r){
        return true;
        // ForbiddenEdge[i][j] = true;
    }
    if (bipartite){
        if (sol.Orientation[i][r] == sol.Orientation[j][r]){
            return true;
        }
    }
    return false;
}

vector<vector<pair<int,int>>> CreateAlternatingCycles(Solution& sol, const vector<int>& OpponentMatching, const int r, const bool bipartite){

    vector<bool>NodeSeen(sol.getNrTeams(), false);
    vector<vector<pair<int,int>>>AlternatingCycles; // each cycle starts with edge found in the matching (so was initially uncolored)
    AlternatingCycles.reserve(sol.getNrTeams()/4);
    int i,j;
    for (int t = 0; t < OpponentMatching.size(); ++t){
        if (!NodeSeen[t]){
            if (sol.TeamColorOpp[t][r] == OpponentMatching[t]){
                // in this case, both in original and enw matching we have (i,j) and (i,j)
                NodeSeen[t] = true, NodeSeen[OpponentMatching[t]] = true;;
                continue;
            }
            i = t;
            // alternating cycle:
            vector<pair<int,int>>AlternatingCycle;
            AlternatingCycle.reserve(sol.getNrTeams());
            // cout << "-------------------" << endl;
            // cout << "Alternating cycle: " << endl;
            // cout << "-------------------" << endl;
            while (true){
                j = OpponentMatching[i];
                NodeSeen[j] = true;
                AlternatingCycle.emplace_back(i,j);
                // cout << i << "- f - " << j << endl;
                i = sol.TeamColorOpp[j][r];
                NodeSeen[i] = true;
                if (sol.Orientation[i][r] == HA::H){
                    AlternatingCycle.emplace_back(i,j);
                    // cout << i << " <- " << j << endl;
                }
                else{
                    AlternatingCycle.emplace_back(j,i);
                    // cout << i << " -> " << j << endl;
                }
                if (i == t){
                    break;
                }
            }
            // cout << "-------------------" << endl;
            AlternatingCycles.push_back(AlternatingCycle);
            if (!bipartite){
                break; // only 1 alternating cycle in case of M+PR, because otherwise a path may use an edge from an other alternating cycle
                // so becomes messy
            }
        }
    }
#ifndef NDEBUG
    if (AlternatingCycles.empty()){
        for (int t = 0; t < OpponentMatching.size(); ++t){
            if (sol.TeamColorOpp[t][r] != OpponentMatching[t]){
                cout << "new matching but no alternating cycle?" << endl;
                cin.get();
            }
        }
    }
#endif
    return AlternatingCycles;
}

pair<vector<pair<int,int>>,vector<int>> MoveMWPM(Solution& sol, const int l, const int r, const bool bipartite, const bool includeHAPs, const bool CM, std::mt19937& gen, const bool MinCostM){

    int N;
    if (CM){
        N = sol.getNrTeams();
    }
    else{
        N = sol.getNrTeamsLeague(l);
    }
    int R = sol.getNrRounds();
    int SizeMatching = N/2;
    vector<bool>TeamSelected(sol.getNrTeams(), true);

    typedef boost::property< boost::edge_weight_t, float, boost::property< boost::edge_index_t, int > >EdgeProperty;

    typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EdgeProperty>BGraph;
    boost::graph_traits< BGraph >::vertex_iterator vi, vi_end;

    BGraph g(N); 

    int UB_random = 100;
    std::uniform_int_distribution<>dist_int = std::uniform_int_distribution<>(0,UB_random); 

    int i_,j_, i, j;
    // vector<vector<bool>>ForbiddenEdge(sol.getNrTeams(), vector<bool>(sol.getNrTeams(), false));
    // forbidden edges: all edges in the current schedule (coloring of the current schedule must remain feasible)
    for (i_ = 0; i_ < N; ++i_){
        for (j_ = i_+1; j_ < N; ++j_){
            if (CM){
                i = i_, j = j_;
            }
            else{
                i = sol.getTeamsLeague(l)[i_], j = sol.getTeamsLeague(l)[j_];
            }
            if (ForbiddenEdge_CM(i,j,r,bipartite,TeamSelected,sol)){
                continue;
            }
            // If still here, edge can be included:
            
            int d;
            if (MinCostM){
                d = sol.getMaxEdgeCost()*(sol.getNrTeams()/2)+1;
                if (CM){
                    d -= sol.getCostMatchRound(i,j,r);
                }
                else{
                    d -= sol.getDistanceTeams(i,j); // - bc algorithm finds matching of MAXIMUM weight
                }
            }
            else{
                d = UB_random*(sol.getNrTeams()/2)+1; // each edge randomly gets a cost of 0 and 1, in a perfect matching there are N/2 edges so max cost should be N/2+1
                d -= dist_int(gen);
            }
            assert(d >= 0);
            boost::add_edge(i_, j_, EdgeProperty(d), g);
        }
    }

    // sol.print_all_rounds();
    // cout << "Bipartite matching in round " << r << endl;

    // cout << "do MWPM" << endl;
    assert(boost::num_edges(g) > 0); // graph cannot be empty

    std::vector< boost::graph_traits< BGraph >::vertex_descriptor > mate1(N);
    // cout << "do maximum weighted matching" << endl;
    assert(mate1.size() == num_vertices(g));
    boost::maximum_weighted_matching(g, &mate1[0]);
    // cout << "maximum weighted matching done" << endl;

    vector<pair<int,int>>Matching(sol.getNrTeams()/2);
    vector<int>OpponentMatching(sol.getNrTeams(), -1); // i.e. OpponentMatching[i] = j, then the opponent of i in the matching is j

    // std::cout << "The new matching is:" << std::endl;
    int m = 0;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi){
        if (mate1[*vi] != boost::graph_traits< BGraph >::null_vertex() && *vi < mate1[*vi]){
            // std::cout << "{" << *vi << ", " << mate1[*vi] << "}" << std::endl;
            i = *vi, j = mate1[*vi];
            if (!CM){
                i = sol.getTeamsLeague(l)[i], j = sol.getTeamsLeague(l)[j];
            }
            OpponentMatching[i] = j;
            OpponentMatching[j] = i;
            Matching[m++] = {i,j};
        }
    }
    assert(Matching.size() == N/2); // check whether matching is perfect
    
    return {Matching, OpponentMatching}; // for Miao's algo: Matching, for my algo: OpponentMatching (bc I do no want full matching but alternating cycles instead)
}

vector<vector<pair<int,int>>>iPRS(Solution& sol, const int l, const int r, const bool bipartite, const bool includeHAPs, const bool CM, std::mt19937& gen, const bool MinCostM){

    /*
    The difference with Miao is that she really starts from 0, while I already have to take into account the matches in the other rounds
    Now: return set of alternating cycles instead of one matching
    Then, evaluate the cycles separately
    */

    // cout << "Start iPRS" << endl;

    pair<vector<pair<int,int>>,vector<int>>OpponentMatching_Matching = MoveMWPM(sol, l, r, bipartite, includeHAPs, CM, gen, MinCostM);

    vector<vector<pair<int,int>>>AlternatingCycles = CreateAlternatingCycles(sol, OpponentMatching_Matching.second, r, bipartite); // first edge in alternating cycle: new match (so initially uncolored)

    return AlternatingCycles;

}


vector<vector<array<int,3>>>EvaluateAlternatingCycleWithPaths(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r, const bool bipartite, const bool CM, int& delta, std::mt19937& gen, const bool MinCostP){

    vector<int>H_teams;
    vector<int>A_teams;

    vector<vector<array<int,3>>>Paths;

    // cout << "Evaluate alternating cycle" << endl;

    int i,j,e;
    // First, uncolor all odd edges (edges of initial matching)
    // subtract from delta
    for (e = 1; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        assert(sol.Orientation[i][r] == HA::H); // most convenient if these are already correct
        assert(sol.Orientation[j][r] == HA::A);
        if (!sol.SRR){
            sol.MatchColor[i][j] = -1;
        }
        else{
            sol.MatchColor[i][j] = -1;
            sol.MatchColor[j][i] = -1;
        }
        if (CM){
            delta -= sol.getCostMatchRound(i,j,r);
            // cout << i << "," << j << "," << r << ": -" << sol.getCostMatchRound(i,j,r) << endl;
            // cout << sol.getDistanceTeams(i,j) << endl;
        }
        else{
            delta -= sol.getDistanceTeams(i, j);
            // cout << sol.getDistanceTeams(i,j) << endl;
        }
    }

    for (e = 0; e < AlternatingCycle.size(); e+=2){
        i = AlternatingCycle[e].first, j = AlternatingCycle[e].second;
        if (sol.Orientation[i][r] == sol.Orientation[j][r] && sol.Orientation[i][r] == HA::H){
            assert(!bipartite);
            if (sol.getCostMatchRound(i,j,r) <= sol.getCostMatchRound(j,i,r)){
                A_teams.push_back(j);
            }
            else{
                A_teams.push_back(i);
                std::swap(i,j);
            }
        }
        else if (sol.Orientation[i][r] == sol.Orientation[j][r] && sol.Orientation[i][r] == HA::A){
            assert(!bipartite);
            if (sol.getCostMatchRound(i,j,r) <= sol.getCostMatchRound(j,i,r)){
                H_teams.push_back(i);
            }
            else{
                H_teams.push_back(j);
                std::swap(i,j);
            }
        }
        else{
            if (sol.Orientation[i][r] == HA::A){
                std::swap(i,j);
            }
        }
        if (CM){
            delta += sol.getCostMatchRound(i,j,r);
            // cout << i << "," << j << "," << r << ": +" << sol.getCostMatchRound(i,j,r) << endl;
            // cout << sol.getDistanceTeams(i,j) << endl;
        }
        else{
            delta += sol.getDistanceTeams(i, j);
            // cout << sol.getDistanceTeams(i,j) << endl;
        }
        SetValueCircleMethod(i,j,r,sol);
        sol.Orientation[i][r] = HA::H;
        sol.Orientation[j][r] = HA::A;
    }
    // cout << "cost now = " << sol.ComputeCostGeneralMatrix() << endl;

    if (!bipartite){
        // fix paths from H_teams to A_teams
        // Pick random H_team and random A_team, and find path between them, and reverse
        // do this by shuffling the vectors, then iteratively go over them
        assert(H_teams.size() == A_teams.size());
        shuffle(H_teams.begin(), H_teams.end(), gen);
        shuffle(A_teams.begin(), A_teams.end(), gen);
        for (int k = 0; k < H_teams.size(); ++k){
            // path is shortest in distance, but does not take into account the costs
            vector<array<int,3>>path;
            bool PathFound = FindNormalPathOneLeague(A_teams[k], H_teams[k], sol, path, delta, MinCostP);
            if (!PathFound){
                throw std::runtime_error("No path found that can repair imbalance caused by M");
            }
            // path is reversed in function!!
            Paths.push_back(path);
        }
    }
        
    return Paths;
}

void GoBackToOldCycle(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r){
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

