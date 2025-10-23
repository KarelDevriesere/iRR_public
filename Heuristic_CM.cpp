#include <assert.h>
#include <cmath>
#include <random>
#include <algorithm>

#include "Heuristic_CM.h"
#include "Operators.h"

Heuristic_CM::Heuristic_CM(const std::unordered_map<move_name_CM, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<move_name_CM, double>& weights, std::mt19937& g, const int HistoryLength, const int obj, const bool MinCostNB): LAHC<move_name_CM>(moves, weights, g){
            SetHistoryLength(HistoryLength);
            InitializeHistoricValues(obj);
            if (MinCostNB){
                MinCostP = true;
                MinCostM = true;
                MinCostPTS = true;
            }
}

Heuristic_CM::~Heuristic_CM(){}

array<int,3>Heuristic_CM::SelectTwoTeamsAndColor(Solution& sol){
    int i = RandomIntegerNumber(0, sol.getNrTeams()-1);
    int j = ((i+1)+(RandomIntegerNumber(0,sol.getNrTeams()-2)))%sol.getNrTeams(); 
    int C = sol.getNrRounds();
    int c = RandomIntegerNumber(0, C-1);
    while (c == sol.MatchColor[i][j] || c == sol.MatchColor[j][i]){
        c = (c+1)%C;
    }
    assert(i != j);
    return {i,j,c};
}

array<int,3>Heuristic_CM::SelectTwoTeamsAndColorMinCost(Solution& sol){
    int k = RandomIntegerNumber(0, sol.getNrTeams()-1);
    int i_max = -1;
    int c_max = -1;
    int d_max = -1;
    int i;
    int cost;
    vector<bool>TeamSeen(sol.getNrTeams(),false);
    for (int r = 0; r < sol.getNrRounds(); ++r){
        i = sol.TeamColorOpp[k][r];
        if (sol.getSetting() == Setting::CM && sol.Orientation[k][r] == HA::H){
            cost = sol.getCostMatchRound(k,i,r);
        }
        else if (sol.getSetting() == Setting::CM && sol.Orientation[k][r] == HA::A){
            cost = sol.getCostMatchRound(i,k,r);
        }
        else{
            assert(sol.getSetting() == Setting::TTP);
            cost = sol.getDistanceTeams(i,k);
        }
        if (cost > d_max){
            i_max = i;
            c_max = r;
            d_max = cost;
        }
        TeamSeen[i] = true;
    }
    int j_best = -1;
    int j;
    for (j = 0; j < sol.getNrTeams(); ++j){
        if (sol.getSetting() == Setting::CM && sol.Orientation[k][c_max] == HA::H){
            cost = sol.getCostMatchRound(k,j,c_max);
        }
        else if (sol.getSetting() == Setting::CM && sol.Orientation[k][c_max] == HA::A){
            cost = sol.getCostMatchRound(j,k,c_max);
        }
        else{
            assert(sol.getSetting() == Setting::TTP);
            cost = sol.getDistanceTeams(j,k);
        }
        if (cost < d_max && !TeamSeen[j]){
            j_best = j;
            d_max = cost;
        }
    }
    if (j_best == -1){
        return SelectTwoTeamsAndColor(sol);
    }
    return {i_max,j_best,c_max};
}

pair<int,int>Heuristic_CM::SelectTwoTeams(Solution& sol){
    int i = RandomIntegerNumber(0, sol.getNrTeams()-1);
    int j = ((i+1)+(RandomIntegerNumber(0,sol.getNrTeams()-2)))%sol.getNrTeams(); 
    assert(i != j);
    return {i,j};
}

pair<int,int>Heuristic_CM::SelectTwoRounds(Solution& sol){
    const int r = RandomIntegerNumber(0,sol.getNrRounds()-1);
    const int s = ((r+1)+(RandomIntegerNumber(0,sol.getNrRounds()-2)))%sol.getNrRounds();
    assert(r != s);
    return {r,s};
}

void Heuristic_CM::SelectTS_CM(Solution& sol){ // use TS for perturbation move!!
    // Andrea doet TS net zoals mij, dus enkel de HAPs van i en j veranderen!!
    // I do not use Eligible opponents since teams i and j do not necessarily need to be eligible..
    // For example, i can be of strength 1 and j of strength 3 if they share opponents of only strength 2..
    pair<int,int> pair = SelectTwoTeams(sol);
    int i = pair.first, j = pair.second; 
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    TS(sol, i, j);
    assert(sol.IsTeamBalanced(i));
    assert(sol.IsTeamBalanced(j));
    if (!Update(sol, sol.ComputeTotalCost())){
        // ResetOrientations(sol, OrientationsCopy); // can be outcommented when not repairing haps in veto_haps() // TODO not efficient
        // ResetMatchColorCopy(sol, MatchColorCopy); // can be outcommented when not repairing haps in veto_haps()
        // ResetTeamColorOpp(sol, TeamColorOppCopy); // can be outcommented when not repairing haps in veto_haps()
        TS(sol, i, j); // put puck when other things are outcommented to go back to original
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    else{
        // current_obj += delta;
        assert(sol.validate());
    }
    return;
}

vector<vector<HA>> MakeOrientationsCopy_CM(const Solution& sol){ // also in ILS: maybe define in Algo?
    vector<vector<HA>>OrientationsCopy(sol.getNrTeams(), vector<HA>(sol.getNrRounds(), HA::BYE));
    for (int t = 0; t < sol.getNrTeams(); ++t){
        for (int c = 0; c < sol.getNrRounds(); ++c){
            OrientationsCopy[t][c] = sol.Orientation[t][c];
        }
    }
    return OrientationsCopy;
}

void ResetOrientations_CM(Solution& sol, const vector<vector<HA>>OrientationsCopy){ // also in ILS: maybe define in Algo?
    for (int t = 0; t < sol.getNrTeams(); ++t){
        for (int c = 0; c < sol.getNrRounds(); ++c){
            sol.Orientation[t][c] = OrientationsCopy[t][c];
        }
    }
}

void Heuristic_CM::SelectPTS_CM(Solution& sol){

    array<int,3>triple;
    if (/*MinCostPTS*/ false){ // at first sight, does not seem to work
        triple = SelectTwoTeamsAndColorMinCost(sol);
    }
    else{
        triple = SelectTwoTeamsAndColor(sol);
    }
    int i = triple[0], j = triple[1], StartColor = triple[2];
    // cout << "i : " << i << " and j = " << j << endl;

#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
#endif

    // int cost_before = current_obj;
    // cout << "Create lantern" << endl;
    Lantarn lantarn = CreateLantarn(sol, i, j, StartColor);
    // cout << "lantarn created" << endl;
    // PrintLantarn(sol, lantarn);

    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy_CM(sol);
    KeepOrientationsAllEdgesLantarn(sol, lantarn, OrientationsCopy);

    // First swap colors because path may use an edge in the lantern
    SwapColorsLantarn(sol, lantarn);
    // cout << "New lantarn: " << endl;
    // PrintLantarn(sol, lantarn);

    vector<array<int,3>>path; // always try to find a path between i and j!! 
    const bool CM = true;
    // Repair Orientations
    bool BalanceRepaired = RepairOrientationsEdgesLantarn_CM(sol, lantarn, OrientationsCopy, path, MinCostP, CM);
    if (!BalanceRepaired){
        throw std::runtime_error("Could not repair imbalance in PTS!");
    }

#ifndef NDEBUG
    for (int i = 0; i < sol.getNrTeams(); ++i){
        assert(sol.IsTeamBalanced(i));
    }
#endif
    if (!Update(sol, sol.ComputeTotalCost())){
        // first, set back all orientations
        ResetOrientations_CM(sol, OrientationsCopy);
        // Then, swap back the colors
        SwapColorsLantarn(sol, lantarn);
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif

        /*
        cout << "old lantarn: " << endl;
        PrintLantarn(sol, lantarn);
        cin.get();
        */
    }
    assert(sol.validate());
    return;
}

void Heuristic_CM::SelectRS_CM(Solution& sol){
    // rounds in current schedule
    pair<int,int>pair = SelectTwoRounds(sol);
    int r = pair.first, s = pair.second;
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    RS(sol, r, s);
    if (!Update(sol, sol.ComputeTotalCost())){
        RS(sol, r, s); // should be put back if other things are outcommented for going back to original
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    return;
}

void Heuristic_CM::SelectPRS_CM(Solution& sol){
    // rounds in current schedule
    pair<int,int>pair = SelectTwoRounds(sol);
    int r = pair.first, s = pair.second;
    const int StartNode = RandomIntegerNumber(0,sol.getNrTeams()-1);
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    PRS(sol, r, s, StartNode);
    if (!Update(sol, sol.ComputeTotalCost())){
        PRS(sol, r, s, StartNode); // should be put back if other things are outcommented for going back to original
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    return;
}

void Heuristic_CM::SelectMatching_CM(Solution& sol, const bool bipartite){

    // Matching only of one league
    // bipartite matching: throw away the matching in round r but keep orientations of the teams. Then, find a new matching
    // this will always succeed because we can always go back to the old matching

    assert(sol.validate());
    // cout << "Travel cost before: " << sol.ComputeTravelCost() << endl;

    const int r = RandomIntegerNumber(0,sol.getNrRounds()-1); // Chose a random round to do the matching
    // int cost_before = current_obj;

    // cout << "Matching" << endl;
    bool includeHAPs = true;

    /*
    cout << "-----------" << endl;
    cout << "Original M:" << endl;
    cout << "-----------" << endl;
    vector<bool>NodeSeen(sol.getNrTeams(), false);
    for (int i = 0; i < sol.getNrTeams(); ++i){
        if (!NodeSeen[i]){
            int j = sol.TeamColorOpp[i][r];
            if (sol.Orientation[i][r] == HA::H){
                cout << i << "<-" << j << endl;
            }
            else{
                cout << i << "->" << j << endl;
            }
            NodeSeen[j] = true;
        }
    }
    cout << "-----------" << endl;
    */

    // cout << "Rounds before matching in round " << r << endl;
    // sol.PrintAllRoundsLeague(l);

    // cout << "do MoveMWPM" << endl;
    int l = 0; // All teams in first league!!
    const bool CM = true;
    // I only do 1 alternating cycle in case of M+PR, because path can use edge of other cycle, did not want to deal with this
    vector<vector<pair<int,int>>>AlternatingCycles = iPRS(sol, l, r, bipartite, includeHAPs, CM, gen, MinCostM);

    if (MinCostM && AlternatingCycles.empty()){
        // if empty, no new matching could be found -> repeat but now random matching (otherwise matchings will prematurely become useless)
        AlternatingCycles = iPRS(sol, l, r, bipartite, includeHAPs, CM, gen, false);
    }
    
    int delta;
    for (auto& AlternatingCycle: AlternatingCycles){
        // cout << "AlternatingCycle found!!" << endl;
        // a_cycle contains only initially uncoloured edges!!!
#ifndef NDEBUG
        int cost_before = sol.ComputeTotalCost();
#endif
        delta = 0;  
        vector<vector<array<int,3>>>Paths = EvaluateAlternatingCycleWithPaths(sol, AlternatingCycle, r, bipartite, CM, delta, gen, MinCostP);
#ifndef NDEBUG
        for (int i = 0; i < sol.getNrTeams(); ++i){
            assert(sol.IsTeamBalanced(i));
        }
        // cout << cost_before << "+" << delta << " == " << sol.ComputeCostGeneralMatrix() << endl;
        if (sol.getSetting() == Setting::CM){
            assert(cost_before+delta == sol.ComputeTotalCost());
        }
#endif

        if (!Update(sol, sol.ComputeTotalCost())){
            // reverse paths again
            /*
            IMPORTANT: first reverse the paths
            Why? Bc path can contain an edge from the new matching, so the color of this edge in the path is WRONG if we go back to the old matching
            However, this is not an issue if we first reverse the paths, bc afterwards we can go overwite everything in round r with GoBackToOldCycle!
            */
            for (auto& path: Paths){
                ReversePath(sol, path);
            }
            // take back the old matching
            GoBackToOldCycle(sol, AlternatingCycle, r);
#ifndef NDEBUG
            assert(cost_before == sol.ComputeTotalCost());
#endif
        }
    }

    assert(sol.validate());
    return;
} 

void Heuristic_CM::SelectBalancedCycle_CM(Solution& sol){

    assert(!MinCostC);
    vector<array<int,3>>Cycle;
    if (!MinCostC){
        Cycle = CycleBalanced(sol, gen);
    }
    int cost_before;
#ifndef NDEBUG
    cost_before = sol.ComputeTotalCost();
#endif

    assert(Cycle[0][0] == Cycle[(int)Cycle.size()-1][1]);
    // cout << "Reverse path" << endl;
    ReversePath(sol, Cycle);
    // cout << "done" << endl;

    if (!Update(sol, sol.ComputeTotalCost())){
        // If not better: reverse the cycle again as if nothing happened
        ReversePath(sol, Cycle);
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    return;
}

void Heuristic_CM::Move(Solution& sol){
    bool bipartite;
    double rnd = RandomDoubleNumber(0.0, 1.0);
    auto iterator = WeightsCumul.upper_bound(rnd);
    CurrentMove = iterator->second;
    auto beg = std::chrono::high_resolution_clock::now();
#ifndef NDEBUG
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
#endif
    if (CurrentMove == move_name_CM::TS){
        SelectTS_CM(sol);
    }
    else if (CurrentMove == move_name_CM::PTS){
        const bool MinCostP = true;
        SelectPTS_CM(sol);
    }
    else if (CurrentMove == move_name_CM::RS){
        SelectRS_CM(sol);
    }
    else if (CurrentMove == move_name_CM::PRS){
        SelectPRS_CM(sol);
    }
    else if (CurrentMove == move_name_CM::M){
        // Ik weet niet of dit heel goed werkt.. lijkt enkel te werken als je random matchings neemt 
        // en niet enkel de beste qua travel distance
        // Komt misschien ook omdat teveel paden moeten reversed worden!!
        bipartite = false;
        SelectMatching_CM(sol, bipartite);
    }
    else if (CurrentMove == move_name_CM::BM){
        bipartite = true;
        SelectMatching_CM(sol, bipartite);
    }
    else{
        const bool MinCostC = false; // MinCost cycles not supported yet (only negative cycles)
        SelectBalancedCycle_CM(sol);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
    ExecutionTimes.at(CurrentMove).push_back(dur);
    NrChosen.at(CurrentMove)++;
    assert(sol.validate());
}

void Heuristic_CM::solve(Input& in, Solution& sol){
    cout << "start solve" << endl;
    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
    setStartTime(start_time);
    best_obj = sol.ComputeTotalCost();
    current_obj = best_obj;
    UpdateBestSolution(sol);
    cout << "updated best solution" << endl;

    // cout << "Start" << endl;

    do {
        Move(sol); // do random move
    }
    while(!STOP);
    SaveBestSolution(sol);
    sol.validate();
}