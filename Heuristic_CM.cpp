#include <assert.h>
#include <cmath>
#include <random>
#include <algorithm>

#include "Heuristic_CM.h"
#include "Operators.h"

Heuristic_CM::Heuristic_CM(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g, const int HistoryLength, const int obj): LAHC<Move>(moves, weights, g){
            SetHistoryLength(HistoryLength);
            InitializeHistoricValues(obj);

            std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
            setStartTime(start_time);
}

Heuristic_CM::~Heuristic_CM(){}

pair<int,int>Heuristic_CM::SelectTwoTeams(Solution& sol){
    int l = 0;
    if (sol.getNrLeagues() > 1){
        l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    }
    int i = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int j = ((i+1)+(RandomIntegerNumber(0,sol.getNrTeamsLeague(l)-2)))%sol.getNrTeamsLeague(l); 
    assert(i != j);
    return {sol.getGlobalIndexTeam(l, i), sol.getGlobalIndexTeam(l, j)};
}

array<int,3>Heuristic_CM::SelectTwoTeamsAndColor(Solution& sol){
    pair<int,int>teams = SelectTwoTeams(sol);
    int i = teams.first, j = teams.second;
    int C = sol.getNrColouredRounds();
    int c = RandomIntegerNumber(0, C-1);
    while (c == sol.MatchColor[i][j] || c == sol.MatchColor[j][i]){
        c = (c+1)%C;
    }
    assert(i != j);
    return {i,j,c};
}

pair<int,int>Heuristic_CM::SelectTwoRounds(Solution& sol){
    const int r = RandomIntegerNumber(0,sol.getNrColouredRounds()-1);
    const int s = ((r+1)+(RandomIntegerNumber(0,sol.getNrColouredRounds()-2)))%sol.getNrColouredRounds();
    assert(r != s);
    return {r,s};
}

void Heuristic_CM::SelectTS(Solution& sol){ // use TS for perturbation move!!
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
    if (!sol.IsBaseAlgo()){
        assert(sol.IsTeamBalanced(i));
        assert(sol.IsTeamBalanced(j));
    }
    if (!Update(sol, sol.ComputeTotalCost())){
        TS(sol, i, j); 
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


void Heuristic_CM::TeamSwapper(Input& in, Solution& sol){
    bool stop = false;
    int nrTeams  = sol.getNrTeams();
    int cost_before = sol.ComputeTotalCost();
    std::cout << "Initial cost: " << cost_before << std::endl;
    int cost_after;
    while(!stop){
	    stop = true;
	    for (int i = 0; i < nrTeams; ++i) {
	    	for (int j = 0; j < nrTeams; ++j) {
    			TS(sol, i, j);
			cost_after = sol.ComputeTotalCost();
			if(cost_after > cost_before){
				// Reset
        			TS(sol, i, j); 
			} else if(cost_after < cost_before) {
				std::cout << "New cost: " << cost_after << std::endl;	
				cost_before = cost_after;
				stop = false;
			}
		}	
	    }
    }
    std::cout << "Final cost: " << sol.ComputeTotalCost() << std::endl;
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

void Heuristic_CM::SelectPTS(Solution& sol){

    array<int,3>triple = SelectTwoTeamsAndColor(sol);
    int i = triple[0], j = triple[1], StartColor = triple[2];
    // cout << "i : " << i << " and j = " << j << ", start color = " << StartColor << endl;

#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
#endif

    // int cost_before = current_obj;
    // cout << "Create lantern" << endl;
    Lantarn lantarn = CreateLantarn(sol, i, j, StartColor);
    // cout << "lantarn created" << endl;
    // PrintLantarn(sol, lantarn);

    if (CurrentMove == Move::PTS && lantarn.InfeasibleColor){
        return; // In base algo: PTS only if feasible colours!!
    }

    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy_CM(sol);
    KeepOrientationsAllEdgesLantarn(sol, lantarn, OrientationsCopy);

    // Check if 2RR constraint is satisfied in 2RR, if not, return
    if (!sol.SRR && !Check2RRConstraintsPTS(sol, lantarn)){
        ResetOrientations_CM(sol, OrientationsCopy);
        return;
    }

    // First swap colors because path may use an edge in the lantern
    SwapColorsLantarn(sol, lantarn);
    // cout << "New lantarn: " << endl;
    // PrintLantarn(sol, lantarn);

    // Repair Orientations
    // cout << "Repair orientations!!" << endl;
    vector<array<int,3>>path; // always try to find a path between i and j!! 
    bool BalanceRepaired = RepairOrientationsEdgesLantarn_CM(sol, lantarn, OrientationsCopy, path, MinCostPR, CM, gen);
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

void Heuristic_CM::SelectRS(Solution& sol){
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

void Heuristic_CM::SelectPRS(Solution& sol){
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

void Heuristic_CM::SelectMatching(Solution& sol, const bool bipartite){

    // Matching only of one league
    // bipartite matching: throw away the matching in round r but keep orientations of the teams. Then, find a new matching
    // this will always succeed because we can always go back to the old matching

    assert(sol.validate());
    // cout << "Travel cost before: " << sol.ComputeTravelCost() << endl;

    const int l = RandomIntegerNumber(0, sol.getNrLeagues()-1); // Chose a random league
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
    // I only do 1 alternating cycle in case of M+PR, because path can use edge of other cycle, did not want to deal with this
    vector<vector<pair<int,int>>>AlternatingCycles;
    if (/*sol.getSetting() == Setting::TTP && sol.getNrRounds() <= sol.getNrTeams() && !bipartite && !MinCostM*/ false){ // TEST Alternating cycle!!
        AlternatingCycles = GreedyAlternatingCycle(sol, r, gen);
    }
    else{
        AlternatingCycles = iPRS(sol, r, l, bipartite, includeHAPs, CM, gen, MinCostM);
    }
    
    int delta;
    for (auto& AlternatingCycle: AlternatingCycles){
        // cout << "AlternatingCycle found!!" << endl;
        // a_cycle contains only initially uncoloured edges!!!
#ifndef NDEBUG
        int cost_before = sol.ComputeTotalCost();
#endif
        delta = 0;  
        bool NoPathDueTo2RRConstraint = false;
        vector<vector<array<int,3>>>Paths = EvaluateAlternatingCycleWithPaths(sol, AlternatingCycle, r, bipartite, CM, delta, gen, MinCostPR, NoPathDueTo2RRConstraint);

        if (!bipartite && NoPathDueTo2RRConstraint){
            assert(!sol.SRR);
            GoBackToOldCycle(sol, AlternatingCycle, r);
#ifndef NDEBUG
            assert(cost_before == sol.ComputeTotalCost());
#endif
            assert(sol.validate());
            return;
        }

#ifndef NDEBUG
        for (int i = 0; i < sol.getNrTeams(); ++i){
            assert(sol.IsTeamBalanced(i));
        }
        if (MinCostM && bipartite){
            assert(cost_before >= sol.ComputeTotalCost());
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
        else{
            break;
        }
    }
    if (AlternatingCycles.empty()){
        // do this here because update is not called when cycles are empty
        Update(sol, sol.ComputeTotalCost());
    }

    assert(sol.validate());
    return;
} 

void Heuristic_CM::SelectBalancedCycle(Solution& sol){

    bool NegativeCycleFound = false;
    int CostReversingCycle = 0;

    vector<array<int,3>>Cycle;
    if (MinCostC){
        cout << "NC" << endl;
        Cycle = NegativeCycle(sol);
        if (Cycle.empty()){
            cout << "No NC, try Floyd-Warshall" << endl;
            cin.get();
            Cycle = NonIncreasingCycle(sol); // If no negative cycle, find a non-increasing cyclce
            assert(!Cycle.empty()); // must always return a cycle!!
        }
        else{
            NegativeCycleFound = true;
        }
    }    
    else {
        Cycle = CycleBalanced(sol, gen);
    }
    
    if (Cycle.empty()){
        assert(MinCostC || !sol.SRR);
        return;
    }
 
    int cost_before;
#ifndef NDEBUG
    cost_before = sol.ComputeTotalCost();
#endif
    cost_before = sol.ComputeTotalCost();

    // assert(Cycle[0][0] == Cycle[(int)Cycle.size()-1][1]);
    // cout << "Reverse path" << endl;
    ReversePath(sol, Cycle);
    // cout << "done" << endl;

    // cout << "cost before = " << cost_before << ", cost now = " << sol.ComputeTotalCost() << " = " << sol.ComputeTotalHACost() << endl;
    // cin.get();

#ifndef NDEBUG
    if (NegativeCycleFound){
        cout << cost_before << " > " << sol.ComputeTotalCost() << endl;
        assert(cost_before > sol.ComputeTotalCost());
    }
#endif

    if (!Update(sol, sol.ComputeTotalCost())){
        // If not better: reverse the cycle again as if nothing happened
        ReversePath(sol, Cycle);
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
        assert(!(MinCostC && NegativeCycleFound)); // If a negative cycle was found, it cannot be that we not accept the solution
#endif
    }
    return;
}

void Heuristic_CM::DoMove(Solution& sol){
    bool bipartite;
    double rnd = RandomDoubleNumber(0.0, 1.0);
    auto iterator = WeightsCumul.upper_bound(rnd);
    CurrentMove = iterator->second;
    auto beg = std::chrono::high_resolution_clock::now();
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
#ifndef NDEBUG
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
#endif
    if (CurrentMove == Move::TS){
        SelectTS(sol);
    }
    else if (CurrentMove == Move::PTS){
        MinCostPR = false;
        SelectPTS(sol);
    }
    else if (CurrentMove == Move::iPTS_MinCost_PR){
        MinCostPR = true;
        SelectPTS(sol);
    }
    else if (CurrentMove == Move::iPTS_Random_PR){
        MinCostPR = false;
        SelectPTS(sol);
    }
    else if (CurrentMove == Move::RS){
        SelectRS(sol);
    }
    else if (CurrentMove == Move::PRS){
        SelectPRS(sol);
    }
    else if (CurrentMove == Move::MinCost_M_MinCost_PR){
        MinCostPR = true;
        MinCostM = true;
        bipartite = false;
        SelectMatching(sol, bipartite);
    }
    else if (CurrentMove == Move::MinCost_M_Random_PR){
        MinCostPR = false;
        MinCostM = true;
        bipartite = false;
        SelectMatching(sol, bipartite);
    }
    else if (CurrentMove == Move::Random_M_MinCost_PR){
        MinCostPR = true;
        MinCostM = false;
        bipartite = false;
        SelectMatching(sol, bipartite);
    }
    else if (CurrentMove == Move::Random_M_Random_PR){
        MinCostPR = false;
        MinCostM = false;
        bipartite = false;
        SelectMatching(sol, bipartite);
    }   
    else if (CurrentMove == Move::MinCost_BM){
        MinCostM = true;
        bipartite = true;
        SelectMatching(sol, bipartite);
    }
    else if (CurrentMove == Move::Random_BM){
        MinCostM = false;
        bipartite = true;
        SelectMatching(sol, bipartite);
    }
    else if (CurrentMove == Move::NC) {
        MinCostC = true; // TODO: NC in line graph for TTP!!
        SelectBalancedCycle(sol);
    }
    else{
        MinCostC = false;
        SelectBalancedCycle(sol);
    }
    if (ResetSolutionAfterMove){
        SaveBestSolution(sol);
        assert(best_obj == sol.ComputeTotalCost());
        ResetSolutionAfterMove = false;
    }
    // cout << "done" << endl;
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
    ExecutionTimes.at(CurrentMove).push_back(dur);
    NrChosen.at(CurrentMove)++;
    assert(sol.validate());
#ifndef NDEBUG
    for (int i = 0; i < sol.getNrTeams(); ++i){
        if (!sol.IsTeamBalanced(i)){
            cout << "Team " << i << " not balanced after doing move " << Moves.at(CurrentMove) << endl;
            return;
        }
    }
#endif
}

void Heuristic_CM::solve(Input& in, Solution& sol){
    cout << "start solve" << endl;
    Reset();
    best_obj = sol.ComputeTotalCost();
    current_obj = best_obj;
    UpdateBestSolution(sol);
    cout << "updated best solution" << endl;

    if (sol.getSetting() == Setting::CM){
        cout << "Cost Minimization!!" << endl;
        CM = true;
    }

    // cout << "Start" << endl;

    do {
        DoMove(sol); // do random move
    }
    while(!STOP);
    SaveBestSolution(sol);
    sol.validate();
}
