#include <assert.h>
#include <cmath>
#include <random>
#include <algorithm>

#include "Heuristic.h"
#include "Operators.h"

Heuristic::Heuristic(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g, const int HistoryLength, const int obj): LAHC<Move>(moves, weights, g){
            SetHistoryLength(HistoryLength); // LAHC
            InitializeHistoricValues(obj,obj,HistoryLength); // LAHC

            std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
            setStartTime(start_time);
}

Heuristic::~Heuristic(){}

pair<int,int>Heuristic::SelectTwoTeams(Solution& sol){
    int l = 0;
    if (sol.getNrLeagues() > 1){
        l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    }
    int i = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int j = ((i+1)+(RandomIntegerNumber(0,sol.getNrTeamsLeague(l)-2)))%sol.getNrTeamsLeague(l); 
    assert(i != j);
    return {sol.getGlobalIndexTeam(l, i), sol.getGlobalIndexTeam(l, j)};
}

array<int,3>Heuristic::SelectTwoTeamsAndColor(Solution& sol){
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

pair<int,int>Heuristic::SelectTwoRounds(Solution& sol){
    const int r = RandomIntegerNumber(0,sol.getNrColouredRounds()-1);
    const int s = ((r+1)+(RandomIntegerNumber(0,sol.getNrColouredRounds()-2)))%sol.getNrColouredRounds();
    assert(r != s);
    return {r,s};
}

void Heuristic::SelectTS(Solution& sol){ // use TS for perturbation move!!
    // Andrea doet TS net zoals mij, dus enkel de HAPs van i en j veranderen!!
    // I do not use Eligible opponents since teams i and j do not necessarily need to be eligible..
    // For example, i can be of strength 1 and j of strength 3 if they share opponents of only strength 2..
    pair<int,int> pair = SelectTwoTeams(sol);
    int i = pair.first, j = pair.second; 
    int cost_before;
    int delta = 0;
    int cost_after = 0;
    if (sol.getSetting() == Setting::TTP){
        delta += CostTSTeamsTTP(i, j, sol);
    }
    else{
        delta += CostTSTeamsYSTP(i, j, sol);
    }
    cost_after = current_obj + delta;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    if (Update(sol, cost_after)){
        TS(sol, i, j); 
        UpdateBest(sol, cost_after);
#ifndef NDEBUG
        int cost_after_sol = sol.ComputeTotalCost();
        int cost_delta = cost_before+delta;
        // cout << "cost_after = " << cost_after_sol << endl;
        // cout << "cost_delta = " << cost_delta << endl;
        assert(cost_after_sol == cost_before+delta);
#endif
    }
    return;
}


void Heuristic::TeamSwapper(Input& in, Solution& sol){
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

void Heuristic::SelectiPTS(Solution& sol){

    array<int,3>triple = SelectTwoTeamsAndColor(sol);
    int i = triple[0], j = triple[1], StartColor = triple[2];
    // cout << "i : " << i << " and j = " << j << ", start color = " << StartColor << endl;

#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
#endif

    /*
    cout << "Cost before:" << endl;
    for (int t = 0; t < sol.getNrTeams(); ++t){
        cout << "Cost of team " << t << ": " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    */

    // int cost_before = current_obj;
    int delta = 0;
    Lantarn lantarn = CreateLantarn(sol, i, j, StartColor, delta);
    // cout << "lantarn created" << endl;
    // PrintLantarn(sol, lantarn);

    if (CurrentMove == Move::PTS && lantarn.InfeasibleColor){
        return; // In base algo: PTS only if feasible colours!!
    }
    else if (lantarn.InfeasibleOpponents){
        return;
    }

    // Delta for computing the current cost
    // Only works if we do not need a path reversal!! Otherwise we would have needed to include the arc going to the home and from the away teams

    if (sol.getSetting() == Setting::TTP){
        if (!lantarn.PathReversalNeeded){
            int SizeLantern = (int)lantarn.middle.size();
            if (lantarn.InfeasibleColor){
                --SizeLantern;
            }
            ColoredRoundsLantern.resize(SizeLantern, -1);
            int k_ = 0;
            for (int k: lantarn.middle){
                if (sol.MatchColor[i][k] > -1){
                    ColoredRoundsLantern[k_++] = sol.MatchColor[i][k];
                } 
            }
            std::sort(ColoredRoundsLantern.begin(), ColoredRoundsLantern.end());
            for (int t: {i,j}){
                delta -= PTSCurrentTravelDelta(ColoredRoundsLantern, t, sol);
            }
        }
        else{
            delta -= (sol.ComputeTravelCostTeamTTP(i)+sol.ComputeTravelCostTeamTTP(j));
        }
    }

    // First swap colors because path may use an edge in the lantern
    SwapColorsLantarn(sol, lantarn, OrientationCopy_i, OrientationCopy_j);
    // PrintLantarn(sol, lantarn);

    // Repair Orientations
    // cout << "Repair orientations!!" << endl;
    vector<array<int,3>>path; // always try to find a path between i and j!! 
    bool BalanceRepaired = RepairOrientationsEdgesLantarn(sol, lantarn, path, MinCostPR, delta, gen);
    if (!BalanceRepaired){
        throw std::runtime_error("Could not repair imbalance in PTS!");
    }

    // add new cost
    if (!lantarn.PathReversalNeeded){
        for (int t: {i,j}){
            delta += PTSCurrentTravelDelta(ColoredRoundsLantern, t, sol);
        }
    }
    else{
        delta += (sol.ComputeTravelCostTeamTTP(i)+sol.ComputeTravelCostTeamTTP(j));
    }
    delta += ((sol.ComputeTTPViolations(i)+sol.ComputeTTPViolations(j))*sol.getCostTTPViolation());

    // delta += (sol.ComputeTotalCostTeamTTP(i)+sol.ComputeTotalCostTeamTTP(j));

    /*
    cout << "Cost after:" << endl;
    for (int t = 0; t < sol.getNrTeams(); ++t){
        cout << "Cost of team " << t << ": " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    */

    int cost_after;
    if (sol.getSetting() == Setting::TTP){
        cost_after = current_obj + delta;
    }
    else{
        cost_after = sol.ComputeTotalCost();
    }

#ifndef NDEBUG
    for (int i = 0; i < sol.getNrTeams(); ++i){
        assert(sol.IsTeamBalanced(i));
    }
    if (sol.getSetting() == Setting::TTP){
        // cout << "cost_before = " << cost_before << endl;
        int cost_after_sol = sol.ComputeTotalCost();
        // cout << "cost_after = " << cost_after << endl;
        int cost_delta = cost_before + delta;
        // cout << "cost_delta = " << cost_delta << endl;
        assert(cost_delta == cost_after_sol);
    }
#endif
    
    if (!Update(sol, cost_after)){
        // first, set back all orientations
        ReversePath(sol, path, true, true);
        // Then, swap back the colors
        SwapColorsLantarn(sol, lantarn, OrientationCopy_i, OrientationCopy_j);
        assert(sol.ComputeTotalCost() == cost_before);

        /*
        cout << "old lantarn: " << endl;
        PrintLantarn(sol, lantarn);
        cin.get();
        */
    }
    else{
        UpdateBest(sol, cost_after);
    }
    return;
}

void Heuristic::SelectRS(Solution& sol){
    // rounds in current schedule
    pair<int,int>pair = SelectTwoRounds(sol);
    int r = pair.first, s = pair.second;
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
        // cout << "Travel before = " << sol.ComputeTravelCostTTP() << endl;
        // cout << "HA cost before = " << sol.ComputeTotalCostTTPViolations() << endl;
#endif

    int delta = 0;
    int cost_after = 0;
    if (sol.getSetting() == Setting::TTP){
        for (int i = 0; i < sol.getNrTeams(); ++i){
            delta += CostRoundSwapTeamiTTP(i, r, s, sol);
        }
        cost_after = current_obj + delta;
    }
    else{
        RS(sol, r, s);
        cost_after = sol.ComputeTotalCost();
    }

    if (sol.getSetting() != Setting::TTP && !Update(sol, cost_after)){
        RS(sol, r, s);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else if (sol.getSetting() == Setting::TTP && Update(sol, cost_after)){
        RS(sol, r, s); 
        UpdateBest(sol, cost_after);
#ifndef NDEBUG
        int cost_after_sol = sol.ComputeTotalCost();
        int cost_after_delta = cost_before + delta;
        // cout << "cost_after = " << cost_after << ", cost_delta = " << cost_after_delta << endl;
        // cout << "Travel after = " << sol.ComputeTravelCostTTP() << endl;
        // cout << "HA cost after = " << sol.ComputeTotalCostTTPViolations() << endl;
        assert(cost_after_sol == cost_after_delta);
#endif
    }
    return;
}

void Heuristic::SelectPRS(Solution& sol){
    // rounds in current schedule
    pair<int,int>pair = SelectTwoRounds(sol);
    int r = pair.first, s = pair.second;
    const int StartNode = RandomIntegerNumber(0,sol.getNrTeams()-1);
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    int delta = 0;
    int cost_after = 0;
    if (sol.getSetting() == Setting::TTP){
        delta = DeltaPRS_TTP(sol, r, s, StartNode);
    }
    else{
        delta = DeltaPRS_YSTP(sol, r, s, StartNode);
    }
    cost_after = current_obj + delta;
#ifndef NDEBUG
    PRS(sol, r, s, StartNode);
    assert(cost_after == sol.ComputeTotalCost());
    PRS(sol, r, s, StartNode);
#endif

    if (Update(sol, cost_after)){
        PRS(sol, r, s, StartNode);
        UpdateBest(sol, cost_after);
#ifndef NDEBUG
        int cost_after_sol = sol.ComputeTotalCost();
        int cost_after_delta = cost_before + delta;
        assert(cost_after_sol == cost_after_delta);
#endif
    }
    return;
}

void Heuristic::SelectiPRS(Solution& sol, const bool bipartite){

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

    vector<vector<pair<int,int>>>AlternatingCycles;
    if (!MinCostAC){ 
        if (sol.getNrRounds() <= sol.getNrTeams()/2){
            AlternatingCycles = AlternatingCycleBM(sol, r, bipartite, gen); // Greedy is fun but maybe best to use same function for everything!!
            if (AlternatingCycles.empty()){
                cout << "No alternating cycle" << endl;
                std::abort();
            }
        }
        else {
            AlternatingCycles = AlternatingCycleBM(sol, r, bipartite, gen);
        }
    }
    else{
        assert(bipartite);
        // cout << "FindMinCostBalancedCycle" << endl;
        AlternatingCycles = FindMinCostBalancedACycle(sol, r, gen);
    }

    vector<int>CostBeforeTTPTeams;
    if (!bipartite){
        CostBeforeTTPTeams.resize(sol.getNrTeams());
        for (int t = 0; t < sol.getNrTeams(); ++t){
            CostBeforeTTPTeams[t] = -sol.ComputeTotalCostTeamTTP(t);
        }
    }
    
    for (auto& AlternatingCycle: AlternatingCycles){
        // cout << "AlternatingCycle found!!" << endl;
        // a_cycle contains only initially uncoloured edges!!!
#ifndef NDEBUG
        int cost_before = sol.ComputeTotalCost();
        // cout << "cost_before = " << cost_before << endl;
#endif 
        bool NoPathDueTo2RRConstraint = false;

        int delta = 0;
        vector<vector<array<int,3>>>Paths = EvaluateAlternatingCycleWithPaths(sol, AlternatingCycle, r, bipartite, delta, gen, MinCostPR, NoPathDueTo2RRConstraint);

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
        if (bipartite){
            assert(sol.ComputeTotalHACost() == 0);
        }
#endif

        if (!bipartite && sol.getSetting() == Setting::TTP){
            assert(delta == 0);
            vector<bool>NodeSeen(sol.getNrTeams(), false);
            int t1,t2,e;
            for (e = 1; e < AlternatingCycle.size(); e+=2){
                t1 = AlternatingCycle[e].first, t2 = AlternatingCycle[e].second;
                delta += (CostBeforeTTPTeams[t1] + sol.ComputeTotalCostTeamTTP(t1));
                delta += (CostBeforeTTPTeams[t2] + sol.ComputeTotalCostTeamTTP(t2));
                NodeSeen[t1] = true, NodeSeen[t2] = true;
            }
            for (auto& path: Paths){
                for (e = 0; e < path.size(); ++e){
                    t1 = path[e][0], t2 = path[e][1];
                    if (!NodeSeen[t1]){
                        delta += (CostBeforeTTPTeams[t1] + sol.ComputeTotalCostTeamTTP(t1));
                        NodeSeen[t1] = true;
                    }
                    if (!NodeSeen[t2]){
                        delta += (CostBeforeTTPTeams[t2] + sol.ComputeTotalCostTeamTTP(t2));
                        NodeSeen[t2] = true;
                    }
                }
            }
        }

        int cost_after;
        if (sol.getSetting() == Setting::TTP){
            cost_after = current_obj+delta;
        }
        else if (bipartite){
            cost_after = current_obj+delta;
        }
        else {
            cost_after = sol.ComputeTotalCost();
        }

#ifndef NDEBUG
        // cout << "cost_after = " << cost_after << ", total_cost = " << sol.ComputeTotalCost() << endl;
        assert(cost_after == sol.ComputeTotalCost());
#endif  

        if (!Update(sol, cost_after)){
            // reverse paths again
            /*
            IMPORTANT: first reverse the paths
            Why? Bc path can contain an edge from the new matching, so the color of this edge in the path is WRONG if we go back to the old matching
            However, this is not an issue if we first reverse the paths, bc afterwards we can go overwite everything in round r with GoBackToOldCycle!
            */
            for (auto& path: Paths){
                ReversePath(sol, path, true, false);
            }
            // take back the old matching
            GoBackToOldCycle(sol, AlternatingCycle, r);
#ifndef NDEBUG
            assert(cost_before == sol.ComputeTotalCost());
#endif
        }
        else{
            UpdateBest(sol, cost_after);
        }
    }
    if (AlternatingCycles.empty()){
        // do this here because update is not called when cycles are empty
        Update(sol, sol.ComputeTotalCost());
    }
    assert(sol.validate());
    return;
} 

void Heuristic::SelectBalancedCycle(Solution& sol){

    bool NegativeCycleFound = false;
    int CostReversingCycle = 0;

    vector<vector<pair<int,int>>>ForbiddenPairs;

    vector<array<int,3>>Cycle = CycleBalanced(sol, gen);
    assert(!Cycle.empty());
 
    int cost_before;
#ifndef NDEBUG
    cost_before = sol.ComputeTotalCost();
    // cout << "Cost before = " << cost_before << endl;
#endif

    // assert(Cycle[0][0] == Cycle[(int)Cycle.size()-1][1]);
    // cout << "Reverse path" << endl;
    int delta = ReversePath(sol, Cycle, false, true);
    // cout << "Cost after = " << sol.ComputeTotalCost() << endl;
    // cout << "Cost delta = " << cost_before + delta << endl;
    // cout << "done" << endl;

    int cost_after;
    if (sol.getSetting() == Setting::TTP){
        cost_after = current_obj+delta;
#ifndef NDEBUG
        assert(cost_after == sol.ComputeTotalCost());
#endif
    }
    else{
        cost_after = sol.ComputeTotalCost();
    }

    if (!Update(sol, cost_after)){
        // If not better: reverse the cycle again as if nothing happened
        ReversePath(sol, Cycle, false, false);
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    else{
        UpdateBest(sol, cost_after);
    }
    return;
}

void Heuristic::DoMove(Solution& sol){
    bool bipartite;
    CurrentMove = SelectNB();
    auto beg = std::chrono::high_resolution_clock::now();
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
    NrChosen.at(CurrentMove)++;
#ifndef NDEBUG
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
#endif
    if (CurrentMove == Move::TS){
        SelectTS(sol);
    }
    else if (CurrentMove == Move::PTS){
        MinCostPR = false;
        SelectiPTS(sol);
    }
    else if (CurrentMove == Move::iPTS_Random_PR){
        MinCostPR = false;
        SelectiPTS(sol);
    }
    else if (CurrentMove == Move::RS){
        SelectRS(sol);
    }
    else if (CurrentMove == Move::PRS){
        SelectPRS(sol);
    }
    else if (CurrentMove == Move::Random_M_Random_PR){
        MinCostPR = false;
        MinCostAC = false;
        bipartite = false;
        SelectiPRS(sol, bipartite);
    }   
    else if (CurrentMove == Move::MinCost_BM){
        MinCostAC = true;
        bipartite = true;
        SelectiPRS(sol, bipartite);
    }
    else if (CurrentMove == Move::Random_BM){
        MinCostAC = false;
        bipartite = true;
        SelectiPRS(sol, bipartite);
    }
    else{
        MinCostC = false;
        SelectBalancedCycle(sol);
    }
    // LAHC
    /*
    if (ResetSolutionAfterMove){
        SaveBestSolution(sol);
        assert(best_obj == sol.ComputeTotalCost());
        ResetSolutionAfterMove = false;
    }
    */
    // cout << "done" << endl;
    // auto end = std::chrono::high_resolution_clock::now();
    // auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
    // ExecutionTimes.at(CurrentMove).push_back(dur);
    assert(sol.validate());
#ifndef NDEBUG
    for (int i = 0; i < sol.getNrTeams(); ++i){
        if (!sol.IsTeamBalanced(i)){
            cout << "Team " << i << " not balanced after doing move " << Moves.at(CurrentMove) << endl;
            return;
        }
    }
#endif
    UpdateListLength(sol);
}

 // ILS:
 /*
void Heuristic::Perturbe(Solution& sol){
    // cout << "--------------------" << endl;
    // cout << "Perturbe" << endl;
    // cout << "Current obj = " << current_obj << endl;
    // cout << "Best obj = " << best_obj << endl;
    PERTURBE = true;
    it_accepted_perturbation = 0;
    cout << sol.ComputeTotalCost() << endl;
    SaveBestSolution(sol);
    current_obj = best_obj;
    while (it_accepted_perturbation < 2){
        DoMove(sol);
    }
    it_accepted_perturbation = 0;
    it_idle = 0;
    it_idle_best = 0;
    PERTURBE = false;
    cout << "New current obj = " << current_obj << " - " << sol.ComputeTotalCost() << endl;
    // cout << "--------------------" << endl;
}
*/

void Heuristic::solve(Input& in, Solution& sol){

    BestOrientation = vector<vector<HA>>(sol.getNrTeams(), vector<HA>(sol.getNrRounds()));
    BestTeamColorOpp = vector<vector<int>>(sol.getNrTeams(), vector<int>(sol.getNrRounds()));
    OrientationCopy_i.resize(sol.getNrRounds());
    OrientationCopy_j.resize(sol.getNrRounds());
    cout << "start solve" << endl;
    Reset();
    best_obj = sol.ComputeTotalCost();
    current_obj = best_obj;
    UpdateBestSolution(sol);
    cout << "updated best solution" << endl;
    // cout << "current_obj = " << current_obj << ", best_obj = " << best_obj << endl;

    // cout << "Start" << endl;

    do {
        DoMove(sol); // do random move
        /*
        if (it_idle > 100000){ // ILS
            // perturbe
            Perturbe(sol);
        }
            */
    }
    while(!STOP);
    SaveBestSolution(sol);
    sol.validate();
}
