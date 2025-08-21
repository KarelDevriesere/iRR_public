#include "Operators.h"
#include <random>
#include <assert.h>
#include <iostream>
#include <string>
#include <array>
#include "GurSolver.h"
#include "algo.h"
#include <algorithm>

#include <boost/config.hpp>
#include <fstream>
#include <iomanip>
#include <boost/graph/edge_coloring.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/maximum_weighted_matching.hpp>
// #include <boost/graph/max_cardinality_matchinsol.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <boost/graph/graphviz.hpp> // for the dot file

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
    // k: position in middle, not the team itself!!!
    int c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
    int c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];
    if (c_j >= 0){
        sol.Orientation[i][c_j] = OrientationsCopy[j][c_j];
    }
    if (c_i >= 0){
        sol.Orientation[j][c_i] = OrientationsCopy[i][c_i];
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
    // Because we swap all the colors and orientations, the resulting HAPs of i and j will still be balanced
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

void PRS(Solution& sol, const int r, const int s, const int StartNode){
    // std::cout << "Partially swap rounds " << r << " and " << s << std::endl;
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
        if (true){
            if (sol.Orientation[i][c_i] == HA::H){
                lantarn.EdgesMatch[i].push_back({i,k});
            }
            else{
                lantarn.EdgesMatch[i].push_back({k,i});
            }
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
            if (lantarn.MatchAlreadyPresent[k].first){
                lantarn.Infeasible2RRMatch = true; // TODO, snap echt niet wat dit doet..
                cout << "infeasible 2RR match" << endl;
                cin.get();
                PrintLantarn(sol, lantarn);
                cin.get();
                return;
            }
            else{
                lantarn.MatchAlreadyPresent[k].first = true;
            }
        }
        else{
            match = {j,k};
            if (lantarn.MatchAlreadyPresent[k].second){
                lantarn.Infeasible2RRMatch = true; // TODO
                cout << "infeasible 2RR match" << endl;
                cin.get();
                PrintLantarn(sol, lantarn);
                cin.get();
                return;
            }
            else{
                lantarn.MatchAlreadyPresent[k].second = true;
            }
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
    lantarn.MatchAlreadyPresent = vector<pair<bool, bool>>(sol.getNrTeams());
    for (int k = 0; k < sol.getNrTeams(); ++k){
        lantarn.MatchAlreadyPresent[k] = {false, false};
    }
    lantarn.EdgesMatch[i] = vector<pair<int,int>>();
    lantarn.EdgesMatch[j] = vector<pair<int,int>>();
    ReplenishLantarn(sol, i, j, StartColor, lantarn);
    if (lantarn.InfeasibleColor && !lantarn.InfeasibleOpponents){
        ReplenishLantarn(sol, j, i, StartColor, lantarn);
    }
    return lantarn;
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


void OptimizeOrientationsCyclesLantarn(Solution& sol, Lantarn& lantarn, const vector<vector<HA>>& OrientationsCopy){
    vector<int>ConservativeTeams;
    vector<int>TeamsUp;
    vector<int>TeamsDown;
    const int i = lantarn.i;
    const int j = lantarn.j;
    int c_i, c_j, k;
    // cout << "Optimize cycles in lantarn" << endl;
    for (int k = 0; k < lantarn.middle.size(); ++k){
        c_i = sol.MatchColor[lantarn.EdgesMatch.at(i)[k].first][lantarn.EdgesMatch.at(i)[k].second];
        c_j = sol.MatchColor[lantarn.EdgesMatch.at(j)[k].first][lantarn.EdgesMatch.at(j)[k].second];
        if (c_i < 0){
            sol.Orientation[i][c_j] = OrientationsCopy[j][c_j];
            continue;
        }
        else if (c_j < 0){
            sol.Orientation[j][c_i] = OrientationsCopy[i][c_i];
            continue;
        }
        else if (sol.Orientation[lantarn.middle[k]][c_i] != sol.Orientation[lantarn.middle[k]][c_j]){
            if (sol.Orientation[lantarn.middle[k]][c_i] == HA::H){
                TeamsUp.push_back(k);
            }
            else{
                TeamsDown.push_back(k);
            }
        }
        else{
            ConservativeTeams.push_back(k);
        }
    }

    vector<int>*MaxTeams;
    vector<int>*MinTeams;
    if (TeamsUp.size() >= TeamsDown.size()){
        MaxTeams = &TeamsUp;
        MinTeams = &TeamsDown;
    }
    else{
        MaxTeams = &TeamsDown;
        MinTeams = &TeamsUp;
    }

    // randomly shuffle the max teams
    unsigned seed = 42;
    shuffle(MaxTeams->begin(), MaxTeams->end(), default_random_engine(seed));

    for (auto& t: ConservativeTeams){
        // keep the orientation, only switch colors
        KeepOrientation(sol, lantarn, i, t, j, OrientationsCopy);
    }
    
    int t;
    for (t = 0; t < MinTeams->size(); ++t){
        // reverse Orientation (forms a cycle) and switch color
        SwapOrientation(sol, lantarn, i, MinTeams->at(t), j, OrientationsCopy);
        SwapOrientation(sol, lantarn, i, MaxTeams->at(t), j, OrientationsCopy);
    }
    for (t = MinTeams->size(); t < MaxTeams->size(); ++t){
        // keep the orientation of these teams
        KeepOrientation(sol, lantarn, i, MaxTeams->at(t), j, OrientationsCopy);
    } 
}

typedef pair<int, int>Edge;

vector<int>FindPath(const int N, const int SOURCE, const int SINK, vector<Edge>& Edges, vector<int>& Weights){

    // TODO: test whether this works for old code!!

    typedef boost::adjacency_list< boost::listS, boost::vecS, boost::directedS, boost::no_property,
        boost::property< boost::edge_weight_t, int > >BGraph;
    typedef boost::graph_traits< BGraph >::vertex_descriptor vertex_descriptor;

    BGraph g(N);

    for (int e = 0; e < Edges.size(); ++e){
        assert(Edges[e].first < N);
        assert(Edges[e].second < N);
        boost::add_edge(Edges[e].first, Edges[e].second, Weights[e], g);
    }

    boost::property_map< BGraph, boost::edge_weight_t >::type weightmap = get(boost::edge_weight, g);
    std::vector< vertex_descriptor > p(num_vertices(g));
    std::vector< int > d(num_vertices(g));
    vertex_descriptor s = vertex(SOURCE, g);

    dijkstra_shortest_paths(g, s, 
        predecessor_map(boost::make_iterator_property_map(p.begin(), 
            get(boost::vertex_index, g))).distance_map(boost::make_iterator_property_map(d.begin(), 
                get(boost::vertex_index, g))));

    vector<int>path;
    
    int v_ = SINK;
    if (p[v_] == v_){
        return path; // this means that the vertex could not be reached
    }

    path.push_back(v_);
    while (SOURCE != p[v_]){
        path.push_back(p[v_]);
        v_ = p[v_];
    }
    path.push_back(SOURCE);

    return path;
}

void ReversePath(Solution& sol, const vector<int> path){
    int i,j,c;
    // cout << "path is" << endl;
    for (int v = 0; v < path.size()-1; ++v){
        // path is traversed from sink to source
        // sink: +A, source: +H
        i = path[v], j = path[v+1];
        c = sol.MatchColor[i][j];
        // the assert below does not work for balanced cycle, I do not understand why..
        assert(sol.Orientation[i][c] == HA::H && sol.Orientation[j][c] == HA::A);
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
        if (!sol.SRR){
            sol.MatchColor[i][j] = -1;
            sol.MatchColor[j][i] = c;
        }
        // cout << i << " <- ";
    }
    // cout << path[path.size()-1] << endl;
}

bool AddPathToLantarn(Solution& sol, Lantarn& lantarn, const int SOURCE, const int SINK){
    // Keep all teams, even from multiple clubs
    // Just calculate whether there is a path from i to j
    // If we found such a path, compute cost of capacity
    // If not satisfied: bad luck..;

    const int l = sol.getLeagueTeam(lantarn.i);
    int N = sol.getNrTeamsLeague(l);
    vector<vector<bool>>forbidden_edge(N, vector<bool>(N, false));

    // Why should we not use edges within the lantarn? -> No reason..
    // For DRR: do not change the orientation of the matches, so here it makes sense to find a path outside the lantarn

    // TODO: it can happen that if we forbid these edges no path exists..

    int i, j, k;
    if (!sol.SRR){
        i = sol.getIndexInLeague(lantarn.i);
        j = sol.getIndexInLeague(lantarn.j);
        for (auto& k_: lantarn.middle){
            k = sol.getIndexInLeague(k_);
            forbidden_edge[i][k] = true;
            forbidden_edge[k][i] = true;
            forbidden_edge[j][k] = true;
            forbidden_edge[k][j] = true;
        }
    }

    vector<Edge>Edges;
    int v,w;
    for (int v_ = 0; v_ < N; ++v_){
        for (int w_ = 0; w_ < N; ++w_){
            v = sol.getTeamsLeague(l)[v_], w = sol.getTeamsLeague(l)[w_];
            if (!forbidden_edge[v_][w_]){
                int c = sol.MatchColor[v][w];
                if (c < 0){
                    // so if game is between non-eligible opponents it will also not
                    // be selected
                    continue;
                }
                if (!sol.SRR && sol.MatchColor[w][v] >= 0){
                    // Why: we are going to reverse the arcs, so if the reversed game already happens somewhere else, we get something infeasible..
                    continue;
                }
                int h = v_, a = w_;
                if (!sol.SRR){
                    assert(sol.Orientation[v][c] == HA::H);
                    assert(sol.Orientation[w][c] == HA::A);
                }
                else{
                    if (sol.Orientation[v][c] == HA::A){
                        assert(sol.Orientation[w][c] == HA::H);
                        h = w_, a = v_;
                    }
                }
                /*
                if (sol.Orientation[v][c] == HA::A){
                    h = w_, a = v_;
                }
                    */
                Edges.push_back(Edge(a, h)); // !! Home team is team with incoming edge!!
            }
        }
    }
    vector<int>Weights(Edges.size());
    std::fill(Weights.begin(), Weights.end(), 1);

    int source_ = sol.getIndexInLeague(SOURCE), sink_ = sol.getIndexInLeague(SINK);
    // cout << "SOURCE = " << SOURCE << ", index = " << sol.getIndexInLeague(SOURCE) << endl;
    // cout << "SINK = " << SINK << ", index = " << sol.getIndexInLeague(SINK) << endl;
    vector<int> path = FindPath(N, source_, sink_, Edges, Weights);
    if (sol.SRR){
        assert(!path.empty());
    }
    bool PathFound = true;
    if (!path.empty()){
        for (v = 0; v < path.size(); ++v){
            path[v] = sol.getTeamsLeague(l)[path[v]];
        }
        ReversePath(sol, path);
    }
    else{
        PathFound = false;
    }
    lantarn.paths.push_back(path);
    return PathFound;
}


vector<int> CycleBalanced(Solution& sol){
    // In principe zou deze genoeg moeten zijn want ik kan van eender welk balanced schedule naar eender ander balanced schedule gaan
    // Do this one if we just want to find a cycle in a balanced schedule
    // Given the cycle, calculate the delta afterwards
    // We know that in a balanced schedule, if we just do a random path, we always end up at a node already in the path

    int N = sol.getNrTeams();
    int C = sol.getNrRounds();
    int i = rand()%N;
    int c;
    vector<bool>NodeVisited(N, false);
    vector<int>Cycle(1, i);
    while (!NodeVisited[i]){
        NodeVisited[i] = true;
        c = rand()%C;
        while (sol.Orientation[i][c] != HA::H){
            c = (c + 1)%C;
        }
        i = sol.TeamColorOpp[i][c];
        assert(sol.Orientation[i][c] == HA::A);
        Cycle.push_back(i);
    }
    // Throw away the part before the first appearance of the last node
    auto first_occurrence = std::find(Cycle.begin(), Cycle.end(), Cycle.back());
    Cycle.erase(Cycle.begin(), first_occurrence);

    return Cycle;
}

vector<vector<int>> ReversePathsMatching(Solution& sol, const vector<pair<int,int>> Matching, const int l, const int r){
    // cout << "Matching with paths" << endl;
    // this means that we did normal matching but didn't take into account the HAPs
    // Hence, try to restore the balance with paths (possibly using edges in the matching)
    vector<int>TeamsA; // all teams who have 1 A game too much
    vector<int>TeamsH; // all teams who have 1 H game too much
    int m, i,j;
    int H = sol.getNrRounds()/2;
    // std::cout << "Matching:" << std::endl;
    for (m = 0; m < Matching.size(); ++m){
        i = Matching[m].first, j = Matching[m].second;

        /*
        string ha_i = "H", ha_j = "H";
        if (sol.Orientation[i][r] == HA::A){
            ha_i = "A";
        }
        if (sol.Orientation[j][r] == HA::A){
            ha_j = "A";
        }
        cout << i << "(" << ha_i << "), " << j << "(" << ha_j << ")" << endl;
        */

        // We now randomly assigned orientations in SwapMatchings: some teams can be unbalanced!!

        for (auto& t: {i,j}){
            if (sol.getNrHomeTeam(t) < H){
                TeamsA.push_back(t);
            }
            else if (sol.getNrHomeTeam(t) > H){
                TeamsH.push_back(t);
            }
        } 
    }
    // Now, choose teams from TeamsA, find shortest path to one of the teams in TeamsH, untill balance is restored
    // But we want paths at least of length 2, because otherwise this is equivalent to bipartite matching? 
    // No, if we take Source, Sink, then this match is for example in the fictive color so path will have longer length
    assert(TeamsH.size() == TeamsA.size());

    const int N = sol.getNrTeams();
    vector<vector<bool>>EdgeSwapped(N, vector<bool>(N, false)); // tracks whether edge is swapped compared to original
    // We want to include edges that were swapped, because if we swap them again, we go back to the original

    // Min Cost Network Flow is also an option, but David thinks it is easier to sequentually solve
    std::random_device rd;
    std::mt19937 generator(rd());
    // std::shuffle(TeamsA.begin(), TeamsA.end(), generator);
    int SOURCE, SINK;
    int c, club, w, i_, j_;
    vector<vector<int>>Weight(N, vector<int>(N));
    // shuffle TeamsH and TeamsA!!!
    shuffle(TeamsH.begin(), TeamsH.end(), default_random_engine(42));
    shuffle(TeamsA.begin(), TeamsA.end(), default_random_engine(42));
    vector<vector<int>>PATHS;
    for (int t = 0; t < TeamsA.size(); ++t){
        SOURCE = TeamsA[t], SINK = TeamsH[t];
        // cout << "find path from " << SOURCE << " to " << SINK << endl;
        vector<Edge>Edges;
        vector<int>Weights;
        // int nr_games = 0;
        // int nr_usable_games = 0;
        // int nr_same_color = 0;
        // int nr_counterpart = 0;
        for (i_ = 0; i_ < sol.getNrTeamsLeague(l); ++i_){
            for (j_ = 0; j_ < sol.getNrTeamsLeague(l); ++j_){
                i = sol.getTeamsLeague(l)[i_], j = sol.getTeamsLeague(l)[j_];
                c = sol.MatchColor[i][j];
                if (sol.isEligible(i,j) && c >= 0){ 
                    // nr_games++;
                    // Do not reverse orientations of matches in the matching in case of 2RR!!
                    // e.g. we had (0,1) in c, now we have (1,0) in r, we cannot go back to (0,1)!
                    if (!sol.SRR && c == r){
                        // nr_same_color++;
                        continue;
                    }
                    else if (!sol.SRR && sol.MatchColor[j][i] >= 0){
                        // Because if we reverse an arc in 2RR, we can get a game that we already scheduled!
                        // Problem is that if we do not include these edges, we might not be able to find a path..
                        // nr_counterpart++;
                        continue;
                    }
                    if (sol.Orientation[i][c] != HA::H || sol.Orientation[j][c] != HA::A){
                        cout << i << "-" << j << " has color " << c << endl;
                    }
                    assert(sol.Orientation[i][c] == HA::H);
                    assert(sol.Orientation[j][c] == HA::A);
                    Edges.push_back(Edge(j,i)); // j->i, so i plays H
                    // cout << "add edge (" << j << ", " << i << ") " << endl;
                    // nr_usable_games++;

                    if (EdgeSwapped[i][j]){
                        w = 0;
                    }
                    else{
                        int club_h = sol.getTeamClub(i), club_a = sol.getTeamClub(j);
                        if (sol.ComputeCapacityClubRound(club_a, c) < sol.getCapacityClub(club_a, c)){
                            if (sol.getNrTeamsClub(club_a) == 1 || club_h == club_a){
                                w = 0;
                            }
                            else{
                                w = 1;
                            }
                        }
                        else{
                            w = 100;
                        }
                    }
                    Weight[i][j] = w;
                    Weights.push_back(w);
                }
            }
        }
        // cout << "Only " << nr_usable_games << " from the " << nr_games << " total nr of games" << endl;
        // cout << "Nr of games from same color = " << nr_same_color << endl;
        // cout << "Nr of games where counterpart already scheduled = " << nr_counterpart << endl;
        // cout << "find path" << endl;
        // This normally includes all nodes, but only edges between the nodes in league l are drawn!
        vector<int>path = FindPath(N, SOURCE, SINK, Edges, Weights);
        if (path.empty()){
            return vector<vector<int>>(); // if unable to find path, return empty vector
        }
        PATHS.push_back(path);
        // cout << "Path found: " << endl;
        for (int p = 1; p < path.size(); p++){
            i = path[p-1], j = path[p];
            c = sol.MatchColor[i][j];
            // cout << i << " <- (" << sol.MatchColor[i][j] << ") ";
            if (p == path.size()-1){
                // cout << j << endl;
                // cout << "end of path" << endl;
            }
            if (!sol.SRR){
                std::swap(sol.MatchColor[i][j], sol.MatchColor[j][i]); // should be the same
                // sol.MatchColor[i][j] = -1;
                // sol.MatchColor[j][i] = c;
            }

            assert(sol.Orientation[i][c] == HA::H && sol.Orientation[j][c] == HA::A);
            std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
            if (EdgeSwapped[i][j]){
                EdgeSwapped[i][j] = false;
                EdgeSwapped[j][i] = false;
            }
            else{
                EdgeSwapped[i][j] = true;
                EdgeSwapped[j][i] = true;
            }
            // cout << " <- (" << Weight[i][j] << ", " << c << ") " << j << "(" << sol.getTeamClub(j) << ")"; // bc the sink comes first so direction path is <-
        }
        // cout << "path was found" << endl;
    }
    // cout << "done" << endl;
    // cin.get();

    return PATHS;
}

vector<pair<int,int>> MWPM(const vector<int>& SelectedTeams, Solution& sol, const vector<vector<bool>>& ForbiddenEdge /*, const vector<bool>& TeamSelected*/){

    const int N = SelectedTeams.size();
    // Maximum Weight Perfect Matching
    typedef boost::property< boost::edge_weight_t, float, boost::property< boost::edge_index_t, int > >EdgeProperty;

    typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, EdgeProperty>BGraph;
    boost::graph_traits< BGraph >::vertex_iterator vi, vi_end;

    BGraph g(N); 

    bool IsWeightRandom = false;
    if (rand()%100 < 1){ // give a small probability for matching with random weights
        IsWeightRandom = true; // TODO: is this a reasonable number?
    }

    int i,j;
    int M = 0; // add this to all the weights: ensures we get a perfect matching
    vector<vector<int>>Weight(N, vector<int>(N));
	for (i = 0; i < N; ++i){
		for (j = 0; j < N; ++j){ // SRR: i+1
            if (!ForbiddenEdge[SelectedTeams[i]][SelectedTeams[j]]){
                if (IsWeightRandom){
                    Weight[i][j] = rand()%100;
                }
                else{
                    Weight[i][j] = sol.getDistanceTeams(SelectedTeams[i], SelectedTeams[j]);
                }
                M += Weight[i][j]; // - bc algorithm finds matching of MAXIMUM weight
            }
		}
	}

    int d;
    for (i = 0; i < N; ++i){
		for (j = 0; j < N; ++j){ // SRR: i+1
            if (!ForbiddenEdge[SelectedTeams[i]][SelectedTeams[j]]){ // TODO: random matchings or tailor costs matching to HAPs!!
                d = M - Weight[i][j]; // - bc algorithm finds matching of MAXIMUM weight
                boost::add_edge(i, j, EdgeProperty(d), g);
                // cout << "add edge " << SelectedTeams[i] << "-" << SelectedTeams[j] << endl;
            }
		}
	}

    assert(boost::num_edges(g) > 0); // graph cannot be empty

    std::vector< boost::graph_traits< BGraph >::vertex_descriptor > mate1(N);
    boost::maximum_weighted_matching(g, &mate1[0]);

    vector<pair<int,int>>Matching;

    /*
    for (int v = 0; v < N; ++v){
        if (mate1[v] == boost::graph_traits<BGraph>::null_vertex()){
            assert(!TeamSelected[v]);
        }
    }
    */

    // std::cout << "The matching is:" << std::endl;
    int cost = 0;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi){
        if (mate1[*vi] != boost::graph_traits< BGraph >::null_vertex() && *vi < mate1[*vi]){
            // std::cout << "{" << *vi << ", " << mate1[*vi] << "}" << std::endl;
            i = *vi, j = mate1[*vi];
            Matching.push_back({SelectedTeams[i],SelectedTeams[j]});
            cost += sol.getDistanceTeams(SelectedTeams[i], SelectedTeams[j]);
            // cout << sol.getDistanceTeams(i,j) << endl;
            // obj += sol.getDistanceTeams(i,j);
        }
    }
    // cout << "cost matching = " << cost << endl;
    // std::cout << std::endl;
    // cout << "obj = " << obj << endl;

    return Matching;
}


void SwapMatchings(Solution& sol, vector<pair<int,int>>Matching, const int l, const int r, const bool bipartite){
    // cout << "SWAP MATCHINGS" << endl;
    const int N = sol.getNrTeams(), R = sol.getNrRounds();
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
        }
        // Problem with normal matching and 2RR is that later we maybe change matchcolor again!!
        SetValueCircleMethod(h, a, r, sol);
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


vector<pair<int,int>>MoveMWPM(Solution& sol, const int l, const int r, const bool bipartite, const bool includeHAPs){

    /*
    The difference with Miao is that she really starts from 0, while I already have to take into account the matches in the other rounds
    */

    int N = sol.getNrTeamsLeague(l), R = sol.getNrRounds(), H = R/2;
    int SizeMatching = N/2;
    vector<bool>TeamSelected(sol.getNrTeams(), true);
    /*
    if (!bipartite){
        SizeMatching = 2+rand()%(SizeMatching/4);
        SizeMatching *= 2;
        TeamSelected = SelectTeamsMatching(sol, l, r, SizeMatching);
    }
    */
    int i_,j_, i, j;
    int m = 0;
    vector<vector<bool>>ForbiddenEdge(sol.getNrTeams(), vector<bool>(sol.getNrTeams(), false));
    // forbidden edges: all edges in the current schedule (coloring of the current schedule must remain feasible)
    for (i_ = 0; i_ < N; ++i_){
        for (j_ = 0; j_ < N; ++j_){
            i = sol.getTeamsLeague(l)[i_], j = sol.getTeamsLeague(l)[j_];
            if (!sol.isEligible(i,j)){
                ForbiddenEdge[i][j] = true;
            }
            else if (sol.MatchColor[i][j] >= 0 && r != sol.MatchColor[i][j]){
                // Dot not set matches in r as forbidden so that we can always go back to the original solution!!
                ForbiddenEdge[i][j] = true; // DRR: forbid any game from happening twice
                // cout << "forbid " << i << " vs " << j << endl;
                if (sol.SRR){
                    ForbiddenEdge[j][i] = true; // SRR: a team can see each opponent at most twice
                }
                assert(sol.TeamColorOpp[i][sol.MatchColor[i][j]] == j);
                assert(sol.TeamColorOpp[j][sol.MatchColor[i][j]] == i);
            }
            else {
                // Then i and j are elgible and their match either does not happen or happens in round r
                if (!sol.SRR){
                    if (r < H && sol.MatchColor[j][i] < H && sol.MatchColor[j][i] >= 0){
                        // forbid the game if (j,i) happended in one of the other games this half
                        ForbiddenEdge[i][j] = true;
                    }
                    else if (r >= H && sol.MatchColor[j][i] >= H){
                        ForbiddenEdge[i][j] = true;
                    }
                }
                if (bipartite){
                    if (sol.Orientation[i][r] == sol.Orientation[j][r]){ // this makes the graph bipartite!!
                        ForbiddenEdge[i][j] = true;
                        ForbiddenEdge[j][i] = true;
                    }
                    // Example: we want to construct a matching for round 5, the match (0,19) happened in round 1
                    // If now 19 plays A and 0 plays 0 in round 5, then without this forbidden edge we would select (19,0), but this game already happened!
                    // Remember: matching works with edges, not with arcs
                    if (sol.Orientation[i][r] == HA::H && sol.Orientation[j][r] == HA::A){
                        ForbiddenEdge[j][i] = true;
                    }
                    else if (sol.Orientation[j][r] == HA::H && sol.Orientation[i][r] == HA::A){
                        ForbiddenEdge[i][j] = true;
                    }
                }
                else{
                    // TeamSelected: if we do matching on the subset of the teams
                    if (!TeamSelected[i] || !TeamSelected[j]){
                        ForbiddenEdge[i][j] = true;
                        ForbiddenEdge[j][i] = true;
                    }
                }
            }

            if (sol.MatchColor[i][j] < 0 && (sol.getTeamClub(i) == sol.getTeamClub(j))){ // extra: teams can only play at max nr of times vs teams from the same club
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
                    ForbiddenEdge[i][j] = true;
                    ForbiddenEdge[j][i] = true;
                }
            }
        }
    }

    if (!sol.SRR){
        for (i_ = 0; i_ < sol.getNrTeamsLeague(l); ++i_){
            for (j_ = 0; j_ < sol.getNrTeamsLeague(l); ++j_){
                i = sol.getTeamsLeague(l)[i_], j = sol.getTeamsLeague(l)[j_];
                if (sol.MatchColor[i][j] == r && (TeamSelected[i] && TeamSelected[j])){
                    // We must always be able to go back to the original matching!!
                    // Hence, for all teams in the original matching, their edges must also be included!
                    assert(!ForbiddenEdge[i][j]);
                }
            }
        }
    }

    // sol.print_all_rounds();
    // cout << "Bipartite matching in round " << r << endl;

    vector<pair<int,int>>Matching = MWPM(sol.getTeamsLeague(l), sol, ForbiddenEdge);
    if (Matching.size() != SizeMatching){
        cout << "Failed to find perfect matching" << endl;
    }
    if (!bipartite){
        vector<bool>NodeSeen(sol.getNrTeams(), false);
        for (auto& i: sol.getTeamsLeague(l)){
            if (!TeamSelected[i] && !NodeSeen[i]){
                j = sol.TeamColorOpp[i][r];
                NodeSeen[j] = true;
                Matching.push_back({i,j});
            }
        }
    }

    // cout << "done" << endl;

    return Matching;

}

bool SwapOrientationNegativeCycle(Solution& sol, vector<pair<int,int>> cycle, const bool OneTeamPerClub){
    int c,i,j;
    int cost_before = sol.ComputeTotalHACost();
    for (auto& arc: cycle){
        j = arc.first, i = arc.second;
        c = sol.MatchColor[i][j];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
    }
    if (!OneTeamPerClub && sol.ComputeCostCapacities() != 0){ // if one team per club this information is in the weights 
        // turn back solution because capacities are not okay..
        // cout << "cycle is negative but cost of capacities = " << sol.ComputeCostCapacities() << endl;
        // cin.get();
        for (auto& arc: cycle){
            j = arc.first, i = arc.second;
            c = sol.MatchColor[i][j];
            std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
        }
        assert(sol.ComputeTotalHACost() == cost_before);
        return false;
    }
    else{
        for (auto& arc: cycle){
            j = arc.first, i = arc.second;
            c = sol.MatchColor[i][j];
            assert(sol.MatchColor[j][i] < 0); // this should have been accounted for in the weights
            sol.MatchColor[i][j] = -1;
            sol.MatchColor[j][i] = c;
        }
        int cost_after = sol.ComputeTotalHACost();
        cout << cost_before << " > " << cost_after << endl;
        assert(cost_before > cost_after);
        return true;
    }
}


int ComputeWeight(Solution& sol, const pair<int,int>arc1, const pair<int,int>arc2, const int NrNodes, const bool OneTeamPerClub){
    // Reverse orientations of arc1 and arc2 in their respective colors
    assert(arc1.second == arc2.first);
    int c, i, j;
    int total_cost_before = sol.ComputeTotalHACost();
    int cost_before = sol.ComputeHACostTeam(arc1.second);
    bool DRR_constraint_violated = false;
    for (auto& arc: {arc1, arc2}){
        j = arc.first, i = arc.second;
        c = sol.MatchColor[i][j];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
        // we do not need to do anything with MatchColor, is not used in ComputeHACostTeam()
        // BUT: for 2RR constraint we need this!!
        if (arc1.first != arc2.second){ // check if, e.g. arc1 = (1,5) and arc2 = (5,1)
            if (sol.MatchColor[j][i] >= 0){
                // If this is the case, we cannot switch the match..
                DRR_constraint_violated = true; // Note: it actually is also needed for SRR..
            }
        }
    }
    int cost_after = sol.ComputeHACostTeam(arc1.second);
    for (auto& arc: {arc1, arc2}){
        j = arc.first, i = arc.second;
        c = sol.MatchColor[i][j];
        std::swap(sol.Orientation[i][c], sol.Orientation[j][c]);
    }
    int cost_cap = 0;
    if (OneTeamPerClub){
        cost_cap = sol.CostCapacityClubHapSwitchTeam(arc1.second, sol.MatchColor[arc1.second][arc1.first]) + sol.CostCapacityClubHapSwitchTeam(arc1.second, sol.MatchColor[arc2.second][arc2.first]);
    }
    assert(total_cost_before == sol.ComputeTotalHACost()); // check if everything went well!!!
    if (cost_before == 0 && cost_after == 0 && !DRR_constraint_violated && cost_cap <= 0){
        return 0;
    }
    else if (cost_before > cost_after && !DRR_constraint_violated && cost_cap <= 0){
        return -1;
    }
    else{
        return NrNodes;
    }
}


bool NegativeCycle(Solution& sol, const int l){
    // cout << "try to find negative cycle" << endl;
    // For teams in league l
    const bool OneTeamPerClub = false; // parameter, turn this on or off
    vector<bool>ClubPresent(sol.getNrClubs(), false);
    typedef pair<int, int>E;
    vector<E>Nodes;
    int i,j,c, club_i, club_j;
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
            if (c >= 0){
                assert(sol.Orientation[i][c] == HA::H);
                assert(sol.Orientation[j][c] == HA::A);
                Nodes.push_back({j,i}); // make for each arc a node
                // remember, arc is j->i, so in arc {j,i} i is the home team
                // cout << "add the node (" << j << "," << i << ")" << endl; // add node per round?
            }
        }
    }

    // Now, evaluate for pair of nodes what the cost will be
    vector<E>edge_vector;
    vector<int>weights;

    int weight;
    for (int v = 0; v < Nodes.size(); ++v){
        for (int w = v+1; w < Nodes.size(); ++w){
            if (Nodes[v].second == Nodes[w].first){
                edge_vector.push_back(E(v, w));
                weight = ComputeWeight(sol, Nodes[v], Nodes[w], (int)Nodes.size(), OneTeamPerClub);
                // cout << "give the arc (" << Nodes[v].first << "," << Nodes[v].second << ") -> (" << Nodes[w].first << "," << Nodes[w].second << ") weight " << weight << endl;
            }
            else if (Nodes[w].second == Nodes[v].first){
                edge_vector.push_back(E(w, v));
                weight = ComputeWeight(sol, Nodes[w], Nodes[v], (int)Nodes.size(), OneTeamPerClub);
                // cout << "give the arc (" << Nodes[w].first << "," << Nodes[w].second << ") -> (" << Nodes[v].first << "," << Nodes[v].second << ") weight " << weight << endl;
            }
            else{
                continue;
            }
            weights.push_back(weight);
        }
    }

    int SOURCE = Nodes.size();
    for (int v = 0; v < Nodes.size(); ++v){
        edge_vector.push_back(E(SOURCE, v)); // create edges SOURCE --> v
        weights.push_back(0); // get weight of 0
    }

    typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::directedS, boost::no_property, boost::property <boost::edge_weight_t, int>>BGraph;

    BGraph g(Nodes.size()+1); // Nodes.size()+1 vertices, +1 bc of source node

    typedef boost::property_map<BGraph, boost::edge_weight_t>::type WeightMap;
    WeightMap weight_map = boost::get(boost::edge_weight, g);
    typedef boost::graph_traits<BGraph>::vertex_descriptor Vertex;

    for (std::size_t j = 0; j < edge_vector.size(); ++j){
        boost::add_edge(edge_vector[j].first, edge_vector[j].second, weights[j], g);
    }

    // Nodes.size()+1 because of source node!!!
    int N = boost::num_vertices(g);
    assert(N == Nodes.size()+1);
    vector<double> distance(N, std::numeric_limits<double>::max());
    vector<Vertex> predecessor(N, boost::graph_traits<BGraph>::null_vertex());

    distance[SOURCE] = 0; // arbitrary start node

    // cout << "start BF" << endl;

    // bool r = boost::bellman_ford_shortest_paths(g, C+1, weight_pmap, &parent[0], &distance[0], boost::closed_plus<int>(), std::less<int>(), boost::default_bellman_visitor());

    bool r = boost::bellman_ford_shortest_paths(g, N, boost::weight_map(get(boost::edge_weight, g)).distance_map(&distance[0]).predecessor_map(&predecessor[0]));

    // cout << "did bellman ford" << endl;

     
    if (!r)
    {
        // Find a vertex that is part of a negative cycle
        Vertex cycle_vertex = boost::graph_traits<BGraph>::null_vertex();
 
        for (int i = 0; i < N; ++i)
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

        // cout << "Recover negative cycle" << endl;
 
        // Recover the negative cycle
        vector<pair<int,int>> cycle;
        if (cycle_vertex != boost::graph_traits<BGraph>::null_vertex())
        {
            Vertex current = cycle_vertex;
            for (int i = 0; i < N; ++i){ // Move `N` steps to guarantee being inside the cycle
                if (predecessor[current] == boost::graph_traits<BGraph>::null_vertex()){
                    // cin.get();
                    return false;
                }
                current = predecessor[current];
            }
            // Track the cycle
            // cout << "track cycle" << endl;
            Vertex cycle_start = current;
            do
            {
                cycle.push_back(Nodes[current]);
                current = predecessor[current];
            } while (current != cycle_start);
 
            // cycle.push_back(Nodes[cycle_start]); // Close the cycle -> not needed because then we do something 2x with the same arc
            std::reverse(cycle.begin(), cycle.end());

            // cout << "Negative cycle detected: ";
            // for (auto& [v_in, v_out] : cycle)
                // cout << "(" << v_in << ", " << v_out << ")" << " ";
            // cout << endl;
            if (!SwapOrientationNegativeCycle(sol, cycle, OneTeamPerClub)){ // test if there is a mistake in here!!
                return false;
            }
        }
    }
    else{
        // cout << "No negative cycle found!" << endl;
        return false;
    }
    return true;

}

bool RepairHAPsWithNegativeCycles(Solution& sol, const int l){
    bool haps_repaired = false;
    bool mission_failed = false;
    do{
        // cout << "Cost HAPs = " << sol.ComputeTotalHACost() << endl;
        // cin.get();
        if (NegativeCycle(sol, l)){
            if (sol.ComputeTotalHACost() == 0){
                haps_repaired = true;
                // Here, we repaired the HAPs
                // cout << "HAPs fully repaired" << endl;
                // cin.get();
            }
        }
        else{
            mission_failed = true;
            // However, here we get stuck -> turn back the solution
            // cout << "HAps could not be restored.." << endl;
        }
    }
    while (!haps_repaired && !mission_failed);

    if (haps_repaired){
        return true;
    }
    else{
        return false;
    }
}


