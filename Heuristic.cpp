#include <assert.h>
#include <cmath>
#include <random>
#include <algorithm>

#include "Heuristic.h"
#include "Operators.h"

// LAHC<Move>(moves, weights, g)

// const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g

Heuristic::Heuristic(Solution& current_sol, std::unique_ptr<MetaBase<Move>> strategy): Operator(current_sol, strategy->gen), MetaH(std::move(strategy)){
            MetaH->Initialize(sol);
            MetaH->SetExecutor(this);

            CostBeforeTTPTeams.resize(sol.getNrTeams());
            OrientationCopy_i.resize(sol.getNrRounds());
            OrientationCopy_j.resize(sol.getNrRounds());

            // DisN and DisR come from Operators.h
            DisL = std::make_unique<Randomizer<int>>(0, sol.getNrLeagues()-1, MetaH->gen);
            DisR2 = std::make_unique<Randomizer<int>>(0, sol.getNrRounds()-2, MetaH->gen);
            for (int l = 0; l < sol.getNrLeagues(); ++l){
                DisTL1.push_back(std::make_unique<Randomizer<int>>(0, sol.getNrTeamsLeague(l)-1, MetaH->gen));
                DisTL2.push_back(std::make_unique<Randomizer<int>>(0, sol.getNrTeamsLeague(l)-2, MetaH->gen));
            }
}

Heuristic::~Heuristic(){}

pair<int,int>Heuristic::SelectTwoTeams(){
    int l = 0;
    if (sol.getNrLeagues() > 1){
        l = DisL->Sample();
    }
    int i = DisTL1[l]->Sample();
    int j = ((i+1)+(DisTL2[l]->Sample()))%sol.getNrTeamsLeague(l); 
    assert(i != j);
    return {sol.getGlobalIndexTeam(l, i), sol.getGlobalIndexTeam(l, j)};
}

array<int,3>Heuristic::SelectTwoTeamsAndColor(){
    PairOfTeams = SelectTwoTeams();
    int i = PairOfTeams.first, j = PairOfTeams.second;
    int c = DisR->Sample();
    while (c == sol.MatchColor[i][j] || c == sol.MatchColor[j][i]){
        c = (c+1)%R;
    }
    assert(i != j);
    assert(c >= 0);
    return {i,j,c};
}

pair<int,int>Heuristic::SelectTwoRounds(){
    const int r = DisR->Sample();
    const int s = ((r+1)+(DisR2->Sample()))%R;
    assert(r != s);
    return {r,s};
}

void Heuristic::SelectTS(){ // use TS for perturbation move!!
    // Andrea doet TS net zoals mij, dus enkel de HAPs van i en j veranderen!!
    // I do not use Eligiblein opponents since teams i and j do not necessarily need to be eligible..
    // For example, i can be of strength 1 and j of strength 3 if they share opponents of only strength 2..
    PairOfTeams = SelectTwoTeams();
    int i = PairOfTeams.first, j = PairOfTeams.second; 
    int cost_before;
    int delta = 0;
    int cost_after = 0;
    if (sol.getSetting() == Setting::TTP){
        delta += CostTSTeamsTTP(i, j);
    }
    else{
        delta += CostTSTeamsYSTP(i, j);
    }
    cost_after = MetaH->current_obj + delta;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    if (MetaH->Update(sol, cost_after)){
        TS(i, j); 
        MetaH->UpdateBest(sol, cost_after);
#ifndef NDEBUG
        int cost_after_sol = sol.ComputeTotalCost();
        int cost_delta = cost_before+delta;
        // cout << "cost_after = " << cost_after_sol << endl;
        // cout << "cost_delta = " << cost_delta << endl;
        assert(cost_after_sol == cost_before+delta);
#endif
        LineGraphUsefull = true;
    }
    return;
}


void Heuristic::TeamSwapper(){
    bool stop = false;
    int nrTeams  = sol.getNrTeams();
    int cost_before = sol.ComputeTotalCost();
    std::cout << "Initial cost: " << cost_before << std::endl;
    int cost_after;
    while(!stop){
	    stop = true;
	    for (int i = 0; i < nrTeams; ++i) {
	    	for (int j = 0; j < nrTeams; ++j) {
    			TS(i, j);
			cost_after = sol.ComputeTotalCost();
			if(cost_after > cost_before){
				// Reset
        			TS(i, j); 
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

void Heuristic::SelectiPTS(){

    PairOfTeamsAndColor = SelectTwoTeamsAndColor();
    int i = PairOfTeamsAndColor[0], j = PairOfTeamsAndColor[1], StartColor = PairOfTeamsAndColor[2];
    // cout << "i : " << i << " and j = " << j << ", start color = " << StartColor << endl;

#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
    if (cost_before != MetaH->current_obj){std::abort();}
#endif

    /*
    cout << "Cost before:" << endl;
    for (int t = 0; t < sol.getNrTeams(); ++t){
        cout << "Cost of team " << t << ": " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    */

    // int cost_before = current_obj;
    int delta = 0; // DELTA
    CreateLantarn(i, j, StartColor);

    if (MetaH->CurrentMove == Move::PTS && lantarn.InfeasibleColor){
        return; // In base algo: PTS only if feasible colours!!
    }
    else if (lantarn.InfeasibleOpponents){
        return;
    }

    /*
    cout << "lantarn created" << endl;
    cout << "----------" << endl;
    PrintLantarn();
    cout << "----------" << endl;
    */
    // cin.get();

    // Delta for computing the current cost
    // Only works if we do not need a path reversal!! Otherwise we would have needed to include the arc going to the home and from the away teams

    if (MetaH->CurrentMove == Move::iPTS_Random_PR_CR){
        clearSwapColorLantarn();
        int SizeUp = lantarn.up.size();
        int SizeDown = lantarn.down.size();
        int MinSize = std::min(SizeUp, SizeDown);
        if (MinSize > 0){
            int v_up = rand()%SizeUp; // ensure some randomness
            int v_down = rand()%SizeDown;
            for (int v = 0; v < MinSize; ++v){
                SwapColorLantarn[lantarn.up[v_up++ % SizeUp]] = 0;
                SwapColorLantarn[lantarn.down[v_down++ % SizeDown]] = 0;
                // cout << " do not swap " << lantarn.up[v] << " and " << lantarn.down[v] << endl;
            }
        }
    }

    /*
    cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    cout << "-------------" << endl;
    cout << "cost of " << i << " = " << sol.ComputeTotalCostTeamTTP(i) << endl;
    cout << "cost of " << j << " = " << sol.ComputeTotalCostTeamTTP(j) << endl;
    for (auto& k: lantarn.middle){
        cout << "cost of " << k << " = " << sol.ComputeTotalCostTeamTTP(k) << endl;
    }
    cout << "-------------" << endl;
    */

    // subtract old cost
    if (sol.getSetting() == Setting::TTP){
        DeltaLantarn(delta);
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
                delta -= PTSCurrentTravelDelta(ColoredRoundsLantern, t);
                // cout << "delta of " << t << " -= " << PTSCurrentTravelDelta(ColoredRoundsLantern, t) << endl;
            }
        }
        else{
            delta -= (sol.ComputeTravelCostTeamTTP(i)+sol.ComputeTravelCostTeamTTP(j));
            // cout << "delta of " << i << " -= " << sol.ComputeTravelCostTeamTTP(i) << endl;
            // cout << "delta of " << j << " -= " << sol.ComputeTravelCostTeamTTP(j) << endl;
        }
    }

    // cout << "-------------" << endl;

    // First swap colors because path may use an edge in the lantern
    SwapColorsLantarn(OrientationCopy_i, OrientationCopy_j);
    // cout << "new lantarn" << endl;
    // cout << "----------" << endl;
    // PrintLantarn();
    // cout << "----------" << endl;

    // add new cost // DELTA
    /*
    if (sol.getSetting() == Setting::TTP){
        if (!lantarn.PathReversalNeeded){
            delta += PTSCurrentTravelDelta(ColoredRoundsLantern, i) + PTSCurrentTravelDelta(ColoredRoundsLantern, j);
            cout << "delta of " << i << " += " << PTSCurrentTravelDelta(ColoredRoundsLantern, i) << endl;
            cout << "delta of " << j << " += " << PTSCurrentTravelDelta(ColoredRoundsLantern, j) << endl;
        }
        else{
            delta += (sol.ComputeTravelCostTeamTTP(i)+sol.ComputeTravelCostTeamTTP(j));
        }
        delta += ((sol.ComputeTTPViolations(i,0,R-1)+sol.ComputeTTPViolations(j,0,R-1))*sol.getCostTTPViolation());
        cout << "delta of " << i << " += " << sol.ComputeTTPViolations(i,0,R-1)*sol.getCostTTPViolation() << endl;
        cout << "delta of " << j << " += " << sol.ComputeTTPViolations(j,0,R-1)*sol.getCostTTPViolation() << endl;
    }
    */

    // cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    // cout << "-------------" << endl;
    // cout << "cost of " << i << " = " << sol.ComputeTotalCostTeamTTP(i) << endl;
    // cout << "cost of " << j << " = " << sol.ComputeTotalCostTeamTTP(j) << endl;
    // for (auto& k: lantarn.middle){
    //     cout << "cost of " << k << " = " << sol.ComputeTotalCostTeamTTP(k) << endl;
    // }
    // cout << "-------------" << endl;

    if (lantarn.PathReversalNeeded){
        // Repair Orientations
        // cout << "Repair orientations!!" << endl;
        clearPath(); // always try to find a path between i and j!! 
        bool BalanceRepaired = RepairOrientationsEdgesLantarn(MinCostPR, delta);
        if (!BalanceRepaired && !MinCostPR){
            throw std::runtime_error("Could not repair imbalance in PTS!");
        }
        // cout << "Path: ";
        // for (auto& edge: path){
        //     cout << edge[0] << "->";
        // }
        // cout << path.back()[1] << endl;
    }

    if (sol.getSetting() == Setting::TTP){
        if (!lantarn.PathReversalNeeded){
            delta += PTSCurrentTravelDelta(ColoredRoundsLantern, i) + PTSCurrentTravelDelta(ColoredRoundsLantern, j);
            // cout << "delta of " << i << " += " << PTSCurrentTravelDelta(ColoredRoundsLantern, i) << endl;
            // cout << "delta of " << j << " += " << PTSCurrentTravelDelta(ColoredRoundsLantern, j) << endl;
        }
        else{
            delta += (sol.ComputeTravelCostTeamTTP(i)+sol.ComputeTravelCostTeamTTP(j));
        }
        delta += ((sol.ComputeTTPViolations(i,0,R-1)+sol.ComputeTTPViolations(j,0,R-1))*sol.getCostTTPViolation());
        // cout << "delta of " << i << " += " << sol.ComputeTTPViolations(i,0,R-1)*sol.getCostTTPViolation() << endl;
        // cout << "delta of " << j << " += " << sol.ComputeTTPViolations(j,0,R-1)*sol.getCostTTPViolation() << endl;
    }

    /*
    cout << "costs after path reversal:" << endl;
    for (int t = 0; t < sol.getNrTeams(); ++t){
        cout << "cost of " << t << " = " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    */

    /*
    cout << "-------------" << endl;
    cout << "cost of " << i << " = " << sol.ComputeTotalCostTeamTTP(i) << endl;
    cout << "cost of " << j << " = " << sol.ComputeTotalCostTeamTTP(j) << endl;
    for (auto& k: lantarn.middle){
        cout << "cost of " << k << " = " << sol.ComputeTotalCostTeamTTP(k) << endl;
    }
    cout << "-------------" << endl;
    */

    // delta += (sol.ComputeTotalCostTeamTTP(i)+sol.ComputeTotalCostTeamTTP(j));

    /*
    cout << "Cost after:" << endl;
    for (int t = 0; t < sol.getNrTeams(); ++t){
        cout << "Cost of team " << t << ": " << sol.ComputeTotalCostTeamTTP(t) << endl;
    }
    */

    int cost_after;
    if (sol.getSetting() == Setting::TTP){
        if (!lantarn.PathReversalNeeded){
            cost_after = MetaH->current_obj+delta;
        }
        else{
            // cost_after = sol.ComputeTotalCost();
            cost_after = MetaH->current_obj+delta;
        }
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
        if (sol.ConstraintViolationAllowed){
            assert(cost_delta == cost_after_sol);
        }
    }
#endif
    
    if (!MetaH->Update(sol, cost_after)){
        // first, set back all orientations
        if (lantarn.PathReversalNeeded){
            ReversePath(true, false);
        }
        // Then, swap back the colors
        SwapColorsLantarn(OrientationCopy_i, OrientationCopy_j);
        assert(sol.ComputeTotalCost() == cost_before);

        /*
        cout << "old lantarn: " << endl;
        PrintLantarn(sol, lantarn);
        cin.get();
        */
    }
    else{
        MetaH->UpdateBest(sol, cost_after);
        LineGraphUsefull = true;
    }

    return;
}

void Heuristic::SelectRS(){
    // rounds in current schedule
    PairOfRounds = SelectTwoRounds();
    int r = PairOfRounds.first, s = PairOfRounds.second;
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
        // cout << "Travel before = " << sol.ComputeTravelCostTTP() << endl;
        // cout << "HA cost before = " << sol.ComputeTotalCostTTPViolations() << endl;
#endif

    int delta = 0;
    int cost_after = 0;
    if (sol.getSetting() == Setting::TTP){
        for (int i = 0; i < N; ++i){
            delta += CostRoundSwapTeamiTTP(i, r, s);
        }
        cost_after = MetaH->current_obj + delta;
    }
    else{
        RS(r, s);
        cost_after = sol.ComputeTotalCost();
    }

    if (sol.getSetting() != Setting::TTP && !MetaH->Update(sol, cost_after)){
        RS(r, s);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else if (sol.getSetting() == Setting::TTP && MetaH->Update(sol, cost_after)){
        RS(r, s); 
        MetaH->UpdateBest(sol, cost_after);
#ifndef NDEBUG
        int cost_after_sol = sol.ComputeTotalCost();
        int cost_after_delta = cost_before + delta;
        // cout << "cost_after = " << cost_after << ", cost_delta = " << cost_after_delta << endl;
        // cout << "Travel after = " << sol.ComputeTravelCostTTP() << endl;
        // cout << "HA cost after = " << sol.ComputeTotalCostTTPViolations() << endl;
        assert(cost_after_sol == cost_after_delta);
#endif
        LineGraphUsefull = true;
    }
    return;
}

void Heuristic::SelectPRS(){
    // rounds in current schedule
    pair<int,int>pair = SelectTwoRounds();
    int r = pair.first, s = pair.second;
    const int StartNode = DisN->Sample();
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    int delta = 0;
    int cost_after = 0;
    if (sol.getSetting() == Setting::TTP){
        delta = DeltaPRS_TTP(r, s, StartNode);
    }
    else{
        delta = DeltaPRS_YSTP(r, s, StartNode);
    }
    cost_after = MetaH->current_obj + delta;
#ifndef NDEBUG
    // The following does not work anymore because of early exit out of PRS!!
    // PRS(r, s, StartNode);
    // assert(cost_after == sol.ComputeTotalCost());
    // PRS(r, s, StartNode);
#endif

    if (MetaH->Update(sol, cost_after)){
        PRS(r, s, StartNode);
        MetaH->UpdateBest(sol, cost_after);
#ifndef NDEBUG
        int cost_after_sol = sol.ComputeTotalCost();
        int cost_after_delta = cost_before + delta;
        assert(cost_after_sol == cost_after_delta);
#endif
        LineGraphUsefull = true;
    }
    return;
}

void Heuristic::SelectiPRS(const bool bipartite){

    // Matching only of one league
    // bipartite matching: throw away the matching in round r but keep orientations of the teams. Then, find a new matching
    // this will always succeed because we can always go back to the old matching

    assert(sol.validate());
    // cout << "Travel cost before: " << sol.ComputeTravelCost() << endl;

    const int l = DisL->Sample(); // Chose a random league
    const int r = DisR->Sample(); // Chose a random round to do the matching
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

    clearAlternatingCycle();
    if (!MinCostAC){ 
        AlternatingCycleBM(l, r, bipartite);
        if (!bipartite && AlternatingCycle.empty()){
            cout << "No alternating cycle" << endl;
            std::abort();
        }
    }
    else{
        assert(bipartite);
        // cout << "FindMinCostBalancedCycle" << endl;
        if (!FindMinCostBalancedACycle(r)){
            MinCostAC = true;
            SelectiPRS(bipartite);
            return;
        }
    }

    /*
    if (!bipartite){
        for (int t = 0; t < N; ++t){
            CostBeforeTTPTeams[t] = -sol.ComputeTotalCostTeamTTP(t);
        }
    }
    */
    
        // cout << "AlternatingCycle found!!" << endl;
        // a_cycle contains only initially uncoloured edges!!!
#ifndef NDEBUG
        int cost_before = sol.ComputeTotalCost();
        // cout << "cost_before = " << cost_before << endl;
#endif 
        bool ConstrViolationAllowedCopy = sol.ConstraintViolationAllowed;
        sol.ConstraintViolationAllowed = true;
        int delta = 0;
        EvaluateAlternatingCycleWithPaths(r, bipartite, delta, MinCostPR);
        // for TTP: only teams whose orientations did not change are included so far in delta

#ifndef NDEBUG
        for (int i = 0; i < N; ++i){
            assert(sol.IsTeamBalanced(i));
        }
        if (bipartite){
            assert(sol.ComputeTotalHACost() == 0);
        }
#endif 

    int cost_after;
    if (sol.getSetting() == Setting::TTP){
        cost_after = MetaH->current_obj+delta;
    }
    else if (bipartite){
        cost_after = MetaH->current_obj+delta;
    }
    else {
        cost_after = sol.ComputeTotalCost();
    }

#ifndef NDEBUG
    // cout << "cost_after = " << cost_after << ", total_cost = " << sol.ComputeTotalCost() << endl;
    if (sol.ConstraintViolationAllowed){
        // cout << cost_after << " == " << sol.ComputeTotalCost() << endl;
        assert(cost_after == sol.ComputeTotalCost());
    }
#endif  

    if (!MetaH->Update(sol, cost_after)){
        // reverse paths again
        /*
        IMPORTANT: first reverse the paths
        Why? Bc path can contain an edge from the new matching, so the color of this edge in the path is WRONG if we go back to the old matching
        However, this is not an issue if we first reverse the paths, bc afterwards we can go overwite everything in round r with GoBackToOldCycle!
        */
        for (auto& current_path: PathsAC){
            path = current_path;
            ReversePath(true, false);
        }
        // take back the old matching
        GoBackToOldCycle(r);
#ifndef NDEBUG
        assert(cost_before == sol.ComputeTotalCost());
#endif
    }
    else{
        MetaH->UpdateBest(sol, cost_after);
        LineGraphUsefull = true;
    }

    if (AlternatingCycle.empty()){
        // do this here because update is not called when cycles are empty
        MetaH->Update(sol, MetaH->current_obj);
    }
    sol.ConstraintViolationAllowed = ConstrViolationAllowedCopy;
    assert(sol.validate());
    return;
} 

void Heuristic::SelectBalancedCycle(){

    bool Success = false;
    if (MinCostC){
        /*
        Either do MinCost cycle or normal cycle
        Normal cycle are also in MinCost cycles if none is found
        */
        // First try to find a negative cycle
        if (LineGraphUsefull){
            if (FindCycleLineGraph(MetaH->current_obj, true)){
                Success = true;
            }
            LineGraphUsefull = false;
        }

        // it does not seem that this works better!!!

        /*
        if (!Success && HistoryLength > 1){
            if (FindCycleLineGraph(current_obj, false)){
                Success = true;
            }
        }
            */
        // if no negative cycle found, do normal cycle

        Success = CycleBalanced();
    }
    else{
        Success = CycleBalanced();
    }
    if (sol.ConstraintViolationAllowed){
        assert(!path.empty());
    }
 
    int cost_before;
#ifndef NDEBUG
    cost_before = sol.ComputeTotalCost();
    // cout << "Cost before = " << cost_before << endl;
#endif

    // assert(Cycle[0][0] == Cycle[(int)Cycle.size()-1][1]);
    // cout << "Reverse path" << endl;
    int delta;
    if (Success){
        delta = ReversePath(false, true);
    }
    else{
        delta = sol.getCostTTPViolation();
    }
    // cout << "Cost after = " << sol.ComputeTotalCost() << endl;
    // cout << "Cost delta = " << cost_before + delta << endl;
    // cout << "done" << endl;

    int cost_after;
    if (sol.getSetting() == Setting::TTP){
        cost_after = MetaH->current_obj+delta;
#ifndef NDEBUG
        // cout << cost_after << " = " << sol.ComputeTotalCost() << endl;
        // assert(cost_after == sol.ComputeTotalCost()); // Does not work anymore because of early exit out of BalancedCycle!
#endif
    }
    else{
        cost_after = sol.ComputeTotalCost();
    }

    if (!MetaH->Update(sol, cost_after)){
        // If not better: reverse the cycle again as if nothing happened
        if (Success){
            ReversePath(false, false);
        }
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    else{
        MetaH->UpdateBest(sol, cost_after);
        LineGraphUsefull = true;
    }
    return;
}

void Heuristic::SelectGPTS(){
    pair<int,int>pair = SelectTwoRounds();
    int r = pair.first, s = pair.second;
    int i = DisN->Sample();
    int cost_before;
#ifndef NDEBUG
        cost_before = sol.ComputeTotalCost();
#endif
    if (GPTS(i,r,s)){
        SwapColorsGPTSLantarn(false);
    }
    else{
        // cout << "No GPTS found" << endl;
        return;
    }
    int cost_after = sol.ComputeTotalCost();
    if (!MetaH->Update(sol, cost_after)){
        SwapColorsGPTSLantarn(true);
#ifndef NDEBUG
        // cout << sol.ComputeTotalCost() << " == " << cost_before << endl;
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    else{
        MetaH->UpdateBest(sol, cost_after);
    }
    assert(sol.validate());
    return;
}

void Heuristic::PushLocalOptimum(){
    if (MetaH->LocalOptimum){
        // cout << "Push local optimum" << endl;
        // cout << "Previous objective = " << sol.ComputeTotalCost() << endl;
        // TODO: only do this if we found a better solution when we increased the history length or decreased the temperature
        int start_obj = MetaH->current_obj+10;
        int it = -1;
        while (start_obj > MetaH->current_obj){
            ++it;
            start_obj = MetaH->current_obj;
            MinCostC = true;
            SelectBalancedCycle();
        }
        if (it > 0){
            cout << it << " negative cycles found" << endl;
        }
        for (int k = 0; k < R; ++k){
            MinCostAC = true;
            SelectiPRS(true);
            if (start_obj > MetaH->current_obj){
                cout << " MinCostAC found!" << endl;
                start_obj = MetaH->current_obj;
            }
        }
        MetaH->LocalOptimum = false;
        // cout << "New objective = " << sol.ComputeTotalCost() << endl;
        // cout << "done" << endl;
        // cin.get();
    }
}

void Heuristic::DoMove(){
    // auto beg = std::chrono::high_resolution_clock::now();
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
    // NrChosen.at(CurrentMove)++;
    bool bipartite;
#ifndef NDEBUG
    // cout << "Select " << Moves.at(CurrentMove) << endl; 
#endif
    if (MetaH->CurrentMove == Move::TS){
        SelectTS();
    }
    else if (MetaH->CurrentMove == Move::PTS){
        MinCostPR = false;
        SelectiPTS();
    }
    else if (MetaH->CurrentMove == Move::iPTS_Random_PR || MetaH->CurrentMove == Move::iPTS_Random_PR_CR){
        MinCostPR = false;
        SelectiPTS();
    }
    else if (MetaH->CurrentMove == Move::iPTS_MinCost_PR){
        MinCostPR = true;
        SelectiPTS();
    }
    else if (MetaH->CurrentMove == Move::RS){
        SelectRS();
    }
    else if (MetaH->CurrentMove == Move::PRS){
        SelectPRS();
    }
    else if (MetaH->CurrentMove == Move::Random_M_Random_PR){
        MinCostPR = false;
        MinCostAC = false;
        bipartite = false;
        SelectiPRS(bipartite);
    }   
    else if (MetaH->CurrentMove == Move::MinCost_BM){
        MinCostAC = true;
        bipartite = true;
        SelectiPRS(bipartite);
    }
    else if (MetaH->CurrentMove == Move::Random_BM){
        MinCostAC = false;
        bipartite = true;
        SelectiPRS(bipartite);
    }
    else if (MetaH->CurrentMove == Move::C){
        MinCostC = false;
        SelectBalancedCycle();
    }
    else if (MetaH->CurrentMove == Move::NC){
        MinCostC = true;
        SelectBalancedCycle();
    }
    else if (MetaH->CurrentMove == Move::GPTS){
        SelectGPTS();
    }
    else{
        cout << "Current selected move = " << MetaH->Moves.at(MetaH->CurrentMove) << " not known" << endl;
        std::abort();
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
    for (int i = 0; i < N; ++i){
        if (!sol.IsTeamBalanced(i)){
            cout << "Team " << i << " not balanced after doing move " << MetaH->Moves.at(MetaH->CurrentMove) << endl;
            return;
        }
    }
#endif
    if (false){
        // If enabled, if stuck we do min cost neighborhoods to check whether
        // we have really hit the bottom
        PushLocalOptimum();
    }

}
