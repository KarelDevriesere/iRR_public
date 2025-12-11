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
        if (sol.getSetting() != Setting::Miao){
            assert(sol.ComputeHACostTeam(k) == 0); // test assumption that the HAPs of the middle teams do not change
            // For Miao instances, we do allow HAP violations
        }
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
    if (lantarn.InfeasibleColor /*&& !lantarn.InfeasibleOpponents*/){
        if (sol.ViolationEligibleOpponents_allowed || !lantarn.InfeasibleOpponents){
            ReplenishLantarn(sol, j, i, StartColor, lantarn);
        }
    }
    return lantarn;
}

bool Check2RRConstraintsPTS(Solution& sol, Lantarn& lantarn){
    const int i = lantarn.i;
    if (sol.ComputeCost2RRConstraintTeam(i) > 0){
        return false;
    }
    const int j = lantarn.j;
    if (sol.ComputeCost2RRConstraintTeam(j) > 0){
        return false;
    }
    for (int k = 0; k < lantarn.middle.size(); ++k){
        if (sol.ComputeCost2RRConstraintTeam(lantarn.middle[k]) > 0){
            return false;
        }
    }
    return true;
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
        if (CM || !MinCostP){
            if (!FindNormalPathOneLeague(source, sink, sol, path, delta, MinCostP)){
                return false;
            }
        }
        else{
            if (!FindPathLineGraphOneLeague(source, sink, sol, path)){
                return false;
            }
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

tuple<vector<Edge>,vector<Edge>,vector<int>> MakeLineGraph(Solution& sol, const int source, const int sink){

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

void ReversePath(Solution& sol, const vector<array<int,3>> path){
    int i,j,c,e;
    // cout << "New path is" << endl;
    for (e = 0; e < path.size(); ++e){
        // path is traversed from sink to source
        // sink: +H, source: +A
        i = path[e][0], j = path[e][1], c = path[e][2];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);

        if (!sol.SRR){
            std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]);
        }

        if (sol.Orientation[i][c] == HA::H){
            // cout << "(" << i << " <- " << j << ")" << endl;
        }
        else{
            // cout << "(" << i << " -> " << j << ")" << endl;
        }
    }
}

bool FindNormalPathOneLeague(const int source, const int sink, Solution& sol, vector<array<int,3>>& path, int& delta, const bool MinCostP){ // source: A too much, sink: H too much
    int i,j,r;
    const int N = sol.getNrTeams();
    BGraph G(N);
    // cout << "source = " << source << endl;
    // cout << "sink = " << sink << endl;

    int cost = 1;
    for (i = 0; i < sol.getNrTeams(); ++i){
        for (j = i+1; j < sol.getNrTeams(); ++j){
            if (!sol.SRR){
                if (sol.MatchColor[i][j] >= 0 && sol.MatchColor[j][i] < 0){
                    if (MinCostP){
                        cost = sol.getCostMatchRound(i,j,sol.MatchColor[j][i]);
                    }
                    boost::add_edge(j, i, 1, G); // remember, edge goes like j->i
                    // cout << "add edge " << i << "<-" << j << endl;
                }
                if (sol.MatchColor[j][i] >= 0 && sol.MatchColor[i][j] < 0){
                    if (MinCostP){
                        cost = sol.getCostMatchRound(j,i,sol.MatchColor[i][j]);
                    }
                    boost::add_edge(i, j, 1, G);
                    // cout << "add edge " << i << "->" << j << endl;
                }
            }
            else{
                r = sol.MatchColor[i][j];
                assert(sol.MatchColor[j][i] == r);
                if (r >= 0){
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
        // delta += (sol.getCostMatchRound(p[v_], v_, r) - sol.getCostMatchRound(v_, p[v_], r));
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

bool FindPathLineGraphOneLeague(const int source, const int sink, Solution& sol, vector<array<int,3>>& path){ // source: team with A too much, sink: team with H too much

    auto [Nodes,edge_vector,weights] = MakeLineGraph(sol, source, sink);

    // Now, create the real source and sink nodes
    // In line graph, nodes are of the form ({j,k}), with j->k
    // Suppose the cheapest path is sink->source. Because every edge in the line graph is the combination of 2 edges, if 
    // we do not add an extra source and sink, this single edge will never be taken as the cheapest path!!
    // But now, we have SOURCE->(sink,source)->SINK as an option

    int SOURCE = Nodes.size();
    int SINK = Nodes.size()+1;
    for (int v = 0; v < Nodes.size(); ++v){
        if (Nodes[v].first == source){
            edge_vector.push_back(Edge(SOURCE, v)); // create edges SOURCE --> v
            weights.push_back(0); // get weight of 0
            // cout << "add " << SOURCE << " -> " << "(" << Nodes[v].first << "," << Nodes[v].second << ")" << endl;
        }
        else if (Nodes[v].second == sink){
            edge_vector.push_back(Edge(v, SINK)); // create edges v --> SINK
            weights.push_back(0); // get weight of 0
            // cout << "(" << Nodes[v].first << "," << Nodes[v].second << ")" << " -> " << SINK << endl;
        }
    }

    BGraph G(Nodes.size()+2); // Nodes.size()+2 vertices, +2 bc of extra SOURCE and SINK node

    for (std::size_t j = 0; j < edge_vector.size(); ++j){
        assert(weights[j] >= 0);
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
    
    int v_ = SINK;
    if (p[v_] == v_){
        // cout << "vertex could not be reached" << endl;
        return false; // this means that the vertex could not be reached
    }

    // cout << "path found:" << endl;
    Edge e;
    int i,j,k,r;
    while (true){
        if (v_ != SOURCE && v_ != SINK){
            e = Nodes[v_];
            i = e.first, k = e.second; // (i,k) = i->k
            r = sol.MatchColor[k][i];

            // cout << "add " << k << " <- " << i << endl;
            path.emplace_back(std::array<int, 3>{k, i, r});
            assert(sol.Orientation[k][r] == HA::H);
            assert(sol.Orientation[i][r] == HA::A);
        }
        if (SOURCE == p[v_]){
            break;
        }
        else{
            v_ = p[v_];
        }
    }

    ReversePath(sol, path);

    return true;
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
        while (sol.Orientation[i][c] != HA::H || (!sol.SRR && sol.MatchColor[sol.TeamColorOpp[i][c]][i] >= 0)){
            c = (c + 1)%C;
            if (c == start_color){
                if (sol.SRR){
                    throw std::runtime_error("Infinite loop in CycleBalanced!!"); 
                }
                else{
                    return {};
                }
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

vector<array<int,3>> NegativeCycle(Solution& sol){
    // See: https://www.boost.org/doc/libs/1_31_0/libs/graph/example/bellman-example.cpp

    // define a source node: goes to all the nodes!!

    int cost = 0; // if no negative cycle cost will be 0

    typedef pair<int, int>E;
    vector<E>edge_vector;
    vector<int>weight;

    const int N = sol.getNrTeams();
    int SOURCE = N;

    int v,w,h,a,r;
    for (v = 0; v < N; ++v){
        for (w = v+1; w < N; ++w){
            r = sol.MatchColor[v][w]; // SRR
            if (r < 0){
                continue;
            }
            if (sol.Orientation[v][r] == HA::H){
                h = v;
                a = w;
            }
            else{
                h = w;
                a = v;
            }
            edge_vector.push_back(E(h,a));
            weight.push_back(sol.getCostMatchRound(a,h,r)-sol.getCostMatchRound(h,a,r)); // improvement of reversing this edge
        }
    }

    for (int v = 0; v < N; ++v){
        edge_vector.push_back(E(SOURCE, v)); // create edges SOURCE --> v
        weight.push_back(0); // get weight of 0
    }

    /*
    vecS: specifies that each vertex stores outgoing vertices in a vector
    vecS: means that the vertices themselves are also stored in a vector
    directedS: specifies that the edges are directed
    no_property: vertices have no extra properties (like labels, colors, ..)
    property <boost::edge_weight_t, int>: each edge has a property called edge_weight_t, which stores an integer value
    */

    typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property <boost::edge_weight_t, int>>BGraph;

    BGraph g(N+1); // N+1 vertices, +1 bc of source node

    typedef boost::property_map<BGraph, boost::edge_weight_t>::type WeightMap;
    WeightMap weight_map = boost::get(boost::edge_weight, g);
    typedef boost::graph_traits<BGraph>::vertex_descriptor Vertex;

    for (std::size_t j = 0; j < edge_vector.size(); ++j){
        boost::add_edge(edge_vector[j].first, edge_vector[j].second, weight[j], g);
    }

    // C+1 because of source node!!!
    assert(boost::num_vertices(g) == N+1);
    vector<double> distance(N+1, std::numeric_limits<double>::max());
    vector<Vertex> predecessor(N+1, boost::graph_traits<BGraph>::null_vertex());

    distance[SOURCE] = 0; // arbitrary start node

    // cout << "start BF" << endl;

    // bool r = boost::bellman_ford_shortest_paths(g, C+1, weight_pmap, &parent[0], &distance[0], boost::closed_plus<int>(), std::less<int>(), boost::default_bellman_visitor());

    bool NC = boost::bellman_ford_shortest_paths(g, N+1, boost::weight_map(get(boost::edge_weight, g)).distance_map(&distance[0]).predecessor_map(&predecessor[0]));

    vector<array<int,3>>Cycle;
    Cycle.reserve(N);
     
    if (!NC)
    {
        Vertex cycle_vertex = boost::graph_traits<BGraph>::null_vertex();
 
        for (int i = 0; i < N+1; ++i)
        {
            for (auto e : make_iterator_range(edges(g)))
            {
                Vertex u = source(e, g), v = target(e, g);
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
            Vertex pred;
            do {
                pred = predecessor[current];
                r = sol.MatchColor[current][pred];
                Cycle.emplace_back(std::array<int, 3>{(int)current,(int)pred,r});
                // cout << "add edge " << (int)current << "<-" << (int)pred << " (round " << r << "): " << sol.getCostMatchRound(pred,current,r)-sol.getCostMatchRound(current,pred,r) << endl;
                current = pred;
            }while (current != cycle_start);
        }
    }
    else{
        // cout << "No negative cycle found!" << endl;
        assert(Cycle.empty());
    }

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
    }
    else if (sol.MatchColor[j][i] >= 0 && sol.MatchColor[j][i] != r){
        return true;
    }
    else if (!sol.isEligible(i,j)){
        return true;
    }
    if (bipartite){
        if (sol.Orientation[i][r] == sol.Orientation[j][r]){
            return true;
        }
    }
    if (sol.getLeagueTeam(i) != sol.getLeagueTeam(j)){
        assert(sol.getNrLeagues() > 1);
        return true;
    }
    return false;
}

vector<vector<pair<int,int>>> CreateAlternatingCycles(Solution& sol, const vector<int>& OpponentMatching, const int r, const bool bipartite, std::mt19937& gen){

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

    if (!bipartite && !AlternatingCycles.empty()){
        shuffle(AlternatingCycles.begin(), AlternatingCycles.end(), gen);
        AlternatingCycles = {AlternatingCycles[0]}; // only 1 alternating cycle otherwise trouble with reversing paths
    }
    return AlternatingCycles;
}

int ComputeEdgeWeightM(const int i, const int j, const int c, const bool MinCostM, const bool bipartite, Solution& sol){
    int d = 0;
    if (sol.getSetting() == Setting::CM){
        // cout << "Setting == CM" << endl;
        if (bipartite && sol.Orientation[i][c] == HA::H){
            d = sol.getCostMatchRound(i,j,c);
        }
        else if (bipartite && sol.Orientation[i][c] == HA::A){
            d = sol.getCostMatchRound(j,i,c);
        }
        else{
            assert(!bipartite);
            d = min(sol.getCostMatchRound(i,j,c), sol.getCostMatchRound(j,i,c));
        }
    }
    else if (sol.getSetting() == Setting::TTP){
        int Opp_i = sol.TeamColorOpp[i][c];
        int Opp_j = sol.TeamColorOpp[j][c];
        sol.TeamColorOpp[i][c] = j;
        sol.TeamColorOpp[j][c] = i;
        d = (sol.ComputeTravelCostTeamTTP(i)+sol.ComputeTravelCostTeamTTP(j));
        if (sol.Orientation[i][c] == sol.Orientation[j][c]){
            assert(!bipartite);
            HA Orientation_i = sol.Orientation[i][c];
            HA Orientation_j = sol.Orientation[j][c];
            sol.Orientation[i][c] = HA::H, sol.Orientation[j][c] = HA::A;
            int d1 = sol.ComputeTTPViolations(i)+sol.ComputeTTPViolations(j);
            sol.Orientation[i][c] = HA::A, sol.Orientation[j][c] = HA::H;
            int d2 = sol.ComputeTTPViolations(i)+sol.ComputeTTPViolations(j);
            d += std::min(d1,d2);
            sol.Orientation[i][c] = Orientation_i;
            sol.Orientation[j][c] = Orientation_j;
        }
        sol.TeamColorOpp[i][c] = Opp_i;
        sol.TeamColorOpp[j][c] = Opp_j;
    }
    else if (sol.getSetting() == Setting::Miao || sol.getSetting() == Setting::Hockey){
        int Opp_i = sol.TeamColorOpp[i][c];
        int Opp_j = sol.TeamColorOpp[j][c];
        d = sol.getDistanceTeams(i,j);
        /* Remove this from the soft space
        if (!sol.isEligible(i,j)){
            d += sol.NonEligibleCost;
        }
            */
        if (sol.Orientation[i][c] == sol.Orientation[j][c]){
            assert(!bipartite);
            sol.TeamColorOpp[i][c] = j;
            sol.TeamColorOpp[j][c] = i;
            HA Orientation_i = sol.Orientation[i][c];
            HA Orientation_j = sol.Orientation[j][c];
            sol.Orientation[i][c] = HA::H, sol.Orientation[j][c] = HA::A;
            int d1 = sol.ComputeHACostTeam(i)+sol.ComputeHACostTeam(j);
            if (sol.Orientation[i][c] != Orientation_i){
                d1 += sol.CostCapacityClubHapSwitchTeam(i, c);
            }
            if (sol.Orientation[j][c] != Orientation_j){
                d1 += sol.CostCapacityClubHapSwitchTeam(j, c);
            }
            sol.Orientation[i][c] = HA::A, sol.Orientation[j][c] = HA::H;
            int d2 = sol.ComputeHACostTeam(i)+sol.ComputeHACostTeam(j);
            if (sol.Orientation[i][c] != Orientation_i){
                d2 += sol.CostCapacityClubHapSwitchTeam(i, c);
            }
            if (sol.Orientation[j][c] != Orientation_j){
                d2 += sol.CostCapacityClubHapSwitchTeam(j, c);
            }
            d += std::min(d1,d2);
            sol.Orientation[i][c] = Orientation_i;
            sol.Orientation[j][c] = Orientation_j;
        }
        sol.TeamColorOpp[i][c] = Opp_i;
        sol.TeamColorOpp[j][c] = Opp_j;
    }

    return d;
}

pair<vector<pair<int,int>>,vector<int>> MoveMWPMOneLeague(Solution& sol, const int r, std::mt19937& gen, const int l){

    int N = sol.getNrTeamsLeague(l);
    int R = sol.getNrRounds();
    int SizeMatching = N/2;
    vector<bool>TeamSelected(N, true);

    typedef boost::property< boost::edge_weight_t, float, boost::property< boost::edge_index_t, int > >EdgeProperty;

    typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EdgeProperty>BGraph;
    boost::graph_traits< BGraph >::vertex_iterator vi, vi_end;

    BGraph g(N); 

    int UB_random = 100;
    std::uniform_int_distribution<>dist_int = std::uniform_int_distribution<>(0,UB_random); 

    int i, j, i_, j_;
    // vector<vector<bool>>ForbiddenEdge(sol.getNrTeams(), vector<bool>(sol.getNrTeams(), false));
    // forbidden edges: all edges in the current schedule (coloring of the current schedule must remain feasible)
    int MaxWeight = 0;
    // each edge randomly gets a cost of 0 and 1, in a perfect matching there are N/2 edges so max cost should be N/2+1
    int Weight = 0;
    bool bipartite = true;
    bool MinCostM = true;

    for (i = 0; i < N; ++i){
        i_ = sol.getGlobalIndexTeam(l,i);
        for (j = i+1; j < N; ++j){
            j_ = sol.getGlobalIndexTeam(l,j);
            if (ForbiddenEdge_CM(i_,j_,r,bipartite,TeamSelected,sol)){
                continue;
            }
            Weight = ComputeEdgeWeightM(i_, j_, r, MinCostM, bipartite, sol);
            if (Weight > MaxWeight){
                MaxWeight = Weight;
            }
        }
    }
    MaxWeight = MaxWeight*(N/2)+1;

    for (i = 0; i < N; ++i){
        i_ = sol.getGlobalIndexTeam(l,i);
        for (j = i+1; j < N; ++j){
            j_ = sol.getGlobalIndexTeam(l,j);
            if (ForbiddenEdge_CM(i_,j_,r,bipartite,TeamSelected,sol)){
                continue;
            }
            // If still here, edge can be included:
            
            int d;
            if (MinCostM){
                d = MaxWeight - ComputeEdgeWeightM(i_, j_, r, MinCostM, bipartite, sol);
            }
            else{
                d = MaxWeight - dist_int(gen);
            }
            assert(d >= 0);
            // cout << "add edge " << i << ", " << j << " with weight " << d << endl;
            boost::add_edge(i, j, EdgeProperty(d), g);
        }
    }

    // sol.print_all_rounds();
    // cout << "Bipartite matching in round " << r << endl;

    // cout << "do MWPM" << endl;
    vector<pair<int,int>>Matching;
    vector<int>OpponentMatching(sol.getNrTeams(), -1); // i.e. OpponentMatching[i] = j, then the opponent of i in the matching is j

    if (boost::num_edges(g) >= N/2){ // otherwise graph cannot contain a perfect matching
        std::vector< boost::graph_traits< BGraph >::vertex_descriptor > mate1(N);
        assert(mate1.size() == num_vertices(g));
        boost::maximum_weighted_matching(g, &mate1[0]);

        // cout << "----------" << endl;
        // cout << "Matching in league " << l << " with " << N << "teams and round " << r << ": " << endl;
        // cout << "----------" << endl;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi){
            if (mate1[*vi] != boost::graph_traits< BGraph >::null_vertex() && *vi < mate1[*vi]){
                // std::cout << "{" << *vi << ", " << mate1[*vi] << "}" << std::endl;
                i = *vi, j = mate1[*vi];
                i_ = sol.getGlobalIndexTeam(l,i);
                j_ = sol.getGlobalIndexTeam(l,j);
                OpponentMatching[i_] = j_;
                OpponentMatching[j_] = i_;
                Matching.push_back({i_,j_});
                // cout << i_ << " vs " << j_ << endl;
            }
        }
        // cout << "----------" << endl;
    }
    
    return {Matching, OpponentMatching}; // for Miao's algo: Matching, for my algo: OpponentMatching (bc I do no want full matching but alternating cycles instead)
}

pair<vector<pair<int,int>>,vector<int>> MoveMWPM(Solution& sol, const int r, const bool bipartite, const bool includeHAPs, const bool CM, std::mt19937& gen, const bool MinCostM){

    int N = sol.getNrTeams();
    int R = sol.getNrRounds();
    int SizeMatching = N/2;
    vector<bool>TeamSelected(sol.getNrTeams(), true);

    typedef boost::property< boost::edge_weight_t, float, boost::property< boost::edge_index_t, int > >EdgeProperty;

    typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EdgeProperty>BGraph;
    boost::graph_traits< BGraph >::vertex_iterator vi, vi_end;

    BGraph g(N); 

    int UB_random = 100;
    std::uniform_int_distribution<>dist_int = std::uniform_int_distribution<>(0,UB_random); 

    int i, j;
    // vector<vector<bool>>ForbiddenEdge(sol.getNrTeams(), vector<bool>(sol.getNrTeams(), false));
    // forbidden edges: all edges in the current schedule (coloring of the current schedule must remain feasible)
    int MaxWeight = 0;
    // each edge randomly gets a cost of 0 and 1, in a perfect matching there are N/2 edges so max cost should be N/2+1
    int Weight = 0;
    if (MinCostM){
        for (i = 0; i < N; ++i){
            for (j = i+1; j < N; ++j){
                if (ForbiddenEdge_CM(i,j,r,bipartite,TeamSelected,sol)){
                    continue;
                }
                Weight = ComputeEdgeWeightM(i, j, r, MinCostM, bipartite, sol);
                if (Weight > MaxWeight){
                    MaxWeight = Weight;
                }
            }
        }
        MaxWeight = MaxWeight*(N/2)+1;
    }
    else{
        MaxWeight = UB_random*(sol.getNrTeams()/2)+1;
    }

    for (i = 0; i < N; ++i){
        for (j = i+1; j < N; ++j){
            if (ForbiddenEdge_CM(i,j,r,bipartite,TeamSelected,sol)){
                continue;
            }
            // If still here, edge can be included:
            
            int d;
            if (MinCostM){
                d = MaxWeight - ComputeEdgeWeightM(i, j, r, MinCostM, bipartite, sol);
            }
            else{
                d = MaxWeight - dist_int(gen);
            }
            assert(d >= 0);
            // cout << "add edge " << i << ", " << j << " with weight " << d << endl;
            boost::add_edge(i, j, EdgeProperty(d), g);
        }
    }

    // sol.print_all_rounds();
    // cout << "Bipartite matching in round " << r << endl;

    // cout << "do MWPM" << endl;
    assert(boost::num_edges(g) > 0); // graph cannot be empty

    std::vector< boost::graph_traits< BGraph >::vertex_descriptor > mate1(N);
    assert(mate1.size() == num_vertices(g));
    boost::maximum_weighted_matching(g, &mate1[0]);

    vector<pair<int,int>>Matching;
    vector<int>OpponentMatching(sol.getNrTeams(), -1); // i.e. OpponentMatching[i] = j, then the opponent of i in the matching is j

    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi){
        if (mate1[*vi] != boost::graph_traits< BGraph >::null_vertex() && *vi < mate1[*vi]){
            // std::cout << "{" << *vi << ", " << mate1[*vi] << "}" << std::endl;
            i = *vi, j = mate1[*vi];
            OpponentMatching[i] = j;
            OpponentMatching[j] = i;
            Matching.push_back({i,j});
        }
    }
    
    return {Matching, OpponentMatching}; // for Miao's algo: Matching, for my algo: OpponentMatching (bc I do no want full matching but alternating cycles instead)
}

vector<vector<pair<int,int>>>iPRS(Solution& sol, const int r, const bool bipartite, const bool includeHAPs, const bool CM, std::mt19937& gen, const bool MinCostM){

    /*
    The difference with Miao is that she really starts from 0, while I already have to take into account the matches in the other rounds
    Now: return set of alternating cycles instead of one matching
    Then, evaluate the cycles separately
    */

    pair<vector<pair<int,int>>,vector<int>>OpponentMatching_Matching = MoveMWPM(sol, r, bipartite, includeHAPs, CM, gen, MinCostM);

    if (OpponentMatching_Matching.first.size() != sol.getNrTeams()/2){
        cout << "Matching has size " << OpponentMatching_Matching.first.size() << " in iPRS" << endl;
        std::abort();
    }

    vector<vector<pair<int,int>>>AlternatingCycles = CreateAlternatingCycles(sol, OpponentMatching_Matching.second, r, bipartite, gen); // first edge in alternating cycle: new match (so initially uncolored)

    return AlternatingCycles;

}


vector<vector<array<int,3>>>EvaluateAlternatingCycleWithPaths(Solution& sol, vector<pair<int,int>>& AlternatingCycle, const int r, const bool bipartite, const bool CM, int& delta, std::mt19937& gen, const bool MinCostP, bool NoPathDueTo2RRConstraint){

    vector<int>H_teams;
    vector<int>A_teams;

    vector<vector<array<int,3>>>Paths;

    std::uniform_real_distribution<> dist(0.0,1.0);

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
            if (sol.MatchColor[j][i] >= 0){
                A_teams.push_back(j);
            }
            else if (sol.MatchColor[i][j] >= 0){
                A_teams.push_back(i);
                std::swap(i,j);
            }
            else{
                int x = dist(gen);
                if (x < 0.5){
                    A_teams.push_back(j);
                }
                else{
                    A_teams.push_back(i);
                    std::swap(i,j);
                }
            }
        }
        else if (sol.Orientation[i][r] == sol.Orientation[j][r] && sol.Orientation[i][r] == HA::A){
            assert(!bipartite);
            if (sol.MatchColor[j][i] >= 0){
                H_teams.push_back(i);
            }
            else if (sol.MatchColor[i][j] >= 0){
                H_teams.push_back(j);
                std::swap(i,j);
            }
            else{
                int x = dist(gen);
                if (x < 0.5){
                    H_teams.push_back(i);
                }
                else{
                    H_teams.push_back(j);
                    std::swap(i,j);
                }
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
        int a;
        for (int k = 0; k < H_teams.size(); ++k){
            // path is shortest in distance, but does not take into account the costs
            // It can be that not path exists between the teams because they are in different disconnected components!
            bool PathFound = false;
            vector<array<int,3>>path;
            a = -1;
            do{
                ++a;
                if (CM || !MinCostP){
                    PathFound = FindNormalPathOneLeague(A_teams[k+a], H_teams[k], sol, path, delta, MinCostP);
                }
                else{
                    PathFound = FindPathLineGraphOneLeague(A_teams[k+a], H_teams[k], sol, path);
                }
            }
            while(k+a+1 < A_teams.size() && !PathFound);

            if (sol.SRR && !PathFound){
                cout << "No path found in M+PR" << endl;
                throw std::runtime_error("Error!!!");
            }
            else if (!sol.SRR && !PathFound){
                // This can happen because 2RR violations are forbidden->these edges are not made
                // In this case, return empty path
                NoPathDueTo2RRConstraint = true;
                return {};
            }
            // path is reversed in function!!
            Paths.push_back(path);
            std::swap(A_teams[k], A_teams[k+a]);
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

