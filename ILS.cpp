#include "ILS.h"
#include "Operators.h"
#include <assert.h>
#include <cmath>
#include <random>

ILS::ILS(const int time_limit){
    TIME_LIMIT = time_limit;
    const double n = (double)Moves.size();
    double sum = 0;
    for (const auto& movename: Moves){
        sum += Weights.at(movename);
        WeightsCumul[movename] = sum;
        NrImprov[movename] = 0;
        NrChosen[movename] = 0;
        NrChosenT[movename] = 0; 
        RewardT[movename] = 0;
    }
    assert(0.99 < WeightsCumul.at(Moves.back()) && WeightsCumul.at(Moves.back()) < 1.01);
}

ILS::~ILS(){}

void ILS::print_solution(){
    if (current_obj < best_obj){
        // best_obj = current_obj;  is set somewhere else
        std::cout << "\033[32m" << current_obj << "\033[0m" << std::endl;
    }
    else if (current_obj == best_obj){
        // std::cout << current_obj << std::endl;
    }
    else{
        std::cout << "\033[31m" << current_obj << "\033[0m" << std::endl;
    }
}

bool ILS::veto_haps(Solution& sol){
    if (!sol.ViolationHAP_allowed && sol.ComputeTotalHACost() > 0){
        return true;
    }
    else{
        return false;
    }
}

bool ILS::Accept(int& cost){
    if (cost <= 0){
        NrAcceptedMovesT++;
        return true;
    }
    else{
        double rnd = (double)rand()/RAND_MAX;
        double p = -(double)cost/(double)T;
        // cout << "rnd = " << rnd << endl;
        // cout << "delta = " << cost << endl;
        // cout << "exp(p) = " << "exp(-" << cost << "/" << T << ") = " << exp(p) << endl;
        // cin.get();
        if (rnd < exp(p) || pertube){ // if rnd < 0 because this means we never accept a move that is worse, unless perturbation
            NrAcceptedMovesT++;
            if (pertube){
                pertube = false; // do only 1 such move
            }
            return true;
        }
        else{
            return false;
        }
    }
}

vector<vector<HA>> MakeOrientationsCopy(const Solution& sol){
    vector<vector<HA>>OrientationsCopy(sol.getNrTeams(), vector<HA>(sol.getNrRounds(), HA::BYE));
    for (int t = 0; t < sol.getNrTeams(); ++t){
        for (int c = 0; c < sol.getNrRounds(); ++c){
            OrientationsCopy[t][c] = sol.Orientation[t][c];
        }
    }
    return OrientationsCopy;
}

pair<int,int>SelectTwoTeamsSameLeague(Solution& sol){
    int l = rand()%sol.getNrLeagues();
    int i_ = rand()%sol.getNrTeamsLeague(l);
    int j_ = ((i_+1)+(rand()%(sol.getNrTeamsLeague(l)-1)))%sol.getNrTeamsLeague(l);
    int i = sol.getTeamsLeague(l)[i_];  
    int j = sol.getTeamsLeague(l)[j_];
    return {i,j};
}

int ILS::SelectTS(Solution& sol){ // use TS for perturbation move!!
    // Andrea doet TS net zoals mij, dus enkel de HAPs van i en j veranderen!!
    // TS: teams must come from the same league!!
    pair<int,int> pair = SelectTwoTeamsSameLeague(sol);
    int i = pair.first, j = pair.second; 
    if (!sol.ViolationEligibleOpponents_allowed && !TS_feasible(sol, i, j)){
        return INT_MAX;
    }
    int cost_before = current_obj;
    TS(sol, i, j);
    int cost_after = sol.ComputeTotalCost();
    int delta = cost_after - cost_before;
    if (!Accept(delta) || veto_haps(sol)){
        TS(sol, i, j); 
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else{
        current_obj += delta;
        sol.validate();
    }
    return delta;
}

int ILS::SelectPTS(Solution& sol){
    // Heel generic: create lantarn, bepaal random orientaties, reken kost uit
    // Opnieuw: teams uit zelfde leagues
    // kan het zijn dat een middle team nu meer als 2 edges heeft?????
    pair<int,int> pair = SelectTwoTeamsSameLeague(sol);
    int i = pair.first, j = pair.second;
    cout << "i : " << i << " and j = " << j << endl;
    int StartColor = rand()%sol.getNrRounds();
    while (sol.TeamColorOpp[i][StartColor] == j){
        // whilke loop bc if DRR, it can happen that i plays vs j in k but also in k+1!!
        ++StartColor %= sol.getNrRounds();
    }

    int cost_before = current_obj;
    Lantarn lantarn = CreateLantarn(sol, i, j, StartColor);
    // cout << "lantarn created" << endl;
    // PrintLantarn(sol, lantarn);
    //cin.get();
    if (!sol.ViolationEligibleOpponents_allowed && lantarn.InfeasibleOpponents || lantarn.Infeasible2RRMatch){
        // It can happen that, if we have an infeasible color, we introduce infeasible games
        return INT_MAX; // hacky..
    }
    // cout << "Original lantarn:" << endl;
    // PrintLantarn(G, lantarn);
    // int cost_travel = CostEvaluationTravelLantarn(G, sol, lantarn);

    // Make a copy of the orientations
    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy(sol);

    // int cost_HAP_before = sol.ComputeHACostLeague(l); // do this instead of teams bc we do not know what paths we will have

    // Since we do not swap everything anymore, the lantarn remains balanced
    if (sol.SRR){
        OptimizeOrientationsCyclesLantarn(sol, lantarn, OrientationsCopy);
    } 
    else{
        KeepOrientationsAllEdgesLantarn(sol, lantarn, OrientationsCopy);
    }
    // cout << "Orientations optimized" << endl;
    //cin.get();
    // Now, we have set all the orientations in the right directions, so we can proceed with the path
    SwapColorsLantarn(sol, lantarn);
    // cout << "colors lantarn swapped" << endl;
    //cin.get();

    // cout << "new lantarn: " << endl;
    // PrintLantarn(sol, lantarn);
    //cin.get();

    // cout << "New lantarn:" << endl;
    // PrintLantarn(G, lantarn);

    bool PathFound = false;
    if (lantarn.InfeasibleColor && sol.Orientation[i][lantarn.c_[j]] != sol.Orientation[j][lantarn.c_[i]]){ // bc after the swap i got c_j and j got c_i
        // find yet another path outside the lantarn
        int SOURCE,SINK;
        if (sol.Orientation[i][lantarn.c_[j]] == HA::A){
            SOURCE = i, SINK = j;
        }
        else{
            SOURCE = j, SINK = i;
        }
        PathFound = AddPathToLantarn(sol, lantarn, SOURCE, SINK);
        // cout << "path added" << endl;
        // cin.get();
    }

    // test whether the lantarn satisfies all HAP constraints
    // int cost_HAP_after = sol.ComputeHACostLeague(l); // we cannot do it only for teams in the lantarn because we possibly also use a path outside the lantarn..
    int cost_after = sol.ComputeTotalCost();
    int delta = cost_after - cost_before;
    // cout << "delta = " << delta << endl;
    if (!Accept(delta) || veto_haps(sol) || !PathFound){
        // Now, unfortunately, we violated the HAP constraints
        if (!sol.SRR && PathFound){
            // bc now we also swapped the matchcolors of the matches in the path!
            // cout << "reverse path again" << endl;
            // reverse the path because it assumed the path goes in a certain order in the function ReversePath()!!
            std::reverse(lantarn.paths[0].begin(), lantarn.paths[0].end());
            ReversePath(sol, lantarn.paths[0]);
            // cout << "done" << endl;
        }
        for (int t = 0; t < sol.getNrTeams(); ++t){
            for (int c = 0; c < sol.getNrRounds(); ++c){
                sol.Orientation[t][c] = OrientationsCopy[t][c];
            }
        }
        SwapColorsLantarn(sol, lantarn);
        assert(sol.ComputeTotalCost() == cost_before);

        /*
        cout << "old lantarn: " << endl;
        PrintLantarn(sol, lantarn);
        cin.get();
        */
    }
    else{
        current_obj += delta;
        sol.validate();
    }
    // cout << "done" << endl;
    return delta;

    // sol.setNrH_from_temp();
    // setAllWeightsBF(G, sol);
    // sol.resetNrH_temp();
}

int ILS::SelectPRS(Solution& sol){
    // rounds in current schedule
    // USE THIS SOLELY NOW FOR CAPACITY VIOLATIONS, NOT FOR TRAVEL DISTANCE
    const int r = rand()%sol.getNrRounds();
    const int s = ((r+1)+(rand()%(sol.getNrRounds()-1)))%sol.getNrRounds();
    const int StartNode = rand()%sol.getNrTeams();
    const int cost_before = current_obj;
    PRS(sol, r, s, StartNode);
    int cost_after = sol.ComputeTotalCost();
    int delta = cost_after - cost_before;
    if (!Accept(delta) || veto_haps(sol)){
        PRS(sol, r, s, StartNode);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else{
        NrImprov[move_name::PRS]++;
        current_obj += delta;
    }
    return delta;
}

int ILS::SelectMatching(const int l, Solution& sol, const bool bipartite){

    // Matching only of one league

    sol.validate();
    // cout << "before: check" << endl;

    const int r = rand()%sol.getNrRounds();

    /*
    for (int s = 0; s < sol.getNrRounds(); ++s){
        cout << "Matches in round " << s << ": " << endl;
        for (int i = 0; i < sol.getNrTeams(); ++i){
            for (int j = 0; j < sol.getNrTeams(); ++j){
                if (sol.MatchColor[i][j] == s){
                    assert(sol.TeamColorOpp[i][s] == j);
                    assert(sol.TeamColorOpp[j][s] == i);
                    assert(sol.Orientation[i][s] == HA::H);
                    assert(sol.Orientation[j][s] == HA::A);
                    cout << i << " vs " << j << " in " << s << endl;
                }
            }
        }
    }
    */

    // First, make a copy so that we can always go back if things don't work out
    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy(sol);
    vector<pair<int,int>>OriginalMatching(sol.getNrTeamsLeague(l)/2); // so that we can go back if things don't work out
    vector<bool>NodeSeen(sol.getNrTeams(), false);
    int j;
    int m = 0;
    for (auto& t: sol.getTeamsLeague(l)){
        if (!NodeSeen[t]){
            j = sol.TeamColorOpp[t][r];
            NodeSeen[j] = true;
            OriginalMatching[m++] = {t,j};
        }
    }
    int cost_before = current_obj;

    // cout << "Matching" << endl;
    bool keepHAP = true;
    vector<pair<int,int>>Matching = MoveMWPM(sol, l, r, bipartite, keepHAP); 
    SwapMatchings(sol, Matching, l, r);
    if (!bipartite && keepHAP){
        ReversePathsMatching(sol, Matching, l, r);
    }

    int cost_after = sol.ComputeTotalCost();
    int delta = cost_after - cost_before;
     
    if (!Accept(delta) || veto_haps(sol)){
        for (int t = 0; t < sol.getNrTeams(); ++t){
            for (int c = 0; c < sol.getNrRounds(); ++c){
                sol.Orientation[t][c] = OrientationsCopy[t][c];
            }
        }
        SwapMatchings(sol, OriginalMatching, l, r);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else{
        current_obj += delta;
    }

    /*
    cout << "Matches after doing everything in round " << r << ": " << endl;
    for (int s = 0; s < sol.getNrRounds(); ++s){
        cout << "Matches in round " << s << ": " << endl;
        for (int i = 0; i < sol.getNrTeams(); ++i){
            for (int j = 0; j < sol.getNrTeams(); ++j){
                if (sol.MatchColor[i][j] == s){
                    assert(sol.TeamColorOpp[i][s] == j);
                    assert(sol.TeamColorOpp[j][s] == i);
                    assert(sol.Orientation[i][s] == HA::H);
                    assert(sol.Orientation[j][s] == HA::A);
                    cout << i << " vs " << j << " in " << s << endl;
                }
            }
        }
    }
    */

    sol.validate();
    // cout << "after: check" << endl;
    return delta;
} 

int ILS::SelectBalancedCycle(const int l, Solution& sol){

    // DOES GIVE ERRORS!

    vector<int>Cycle = CycleBalanced(sol);
    int cost_before = current_obj;

    // Calculate the cost of reversing this cycle
    /*
    int cost_cycle_before = sol.ComputeCostCapacities(); // TODO: can be more efficient
    // cout << "Cycle: " << endl;
    for (int i = 0; i < Cycle.size()-1; ++i){ // the last element is also the first element!!
        cost_cycle_before += sol.ComputeHACostTeam(l, Cycle[i]);
    }
    */
    // cout << "cost before = " << cost_cycle_before << endl;

    assert(Cycle[0] == Cycle[(int)Cycle.size()-1]);
    ReversePath(sol, Cycle);

    /*
    int cost_cycle_after = sol.ComputeCostCapacities(); // TODO: can be more efficient
    for (int i = 0; i < Cycle.size()-1; ++i){
        cost_cycle_after += sol.ComputeHACostTeam(l, Cycle[i]);
    }
    */

    int cost2RR = 0;
    if (!sol.SRR){
        cost2RR = sol.ComputeCost2RRConstraint();
    }
    int cost_after = 0;
    if (cost2RR == 0){
        cost_after = sol.ComputeTotalCost();
    }
    
    int delta = cost_after - cost_before;

    if (!Accept(delta) || veto_haps(sol) || cost2RR > 0){
        // If not better: reverse the cycle again as if nothing happened
        std::reverse(Cycle.begin(), Cycle.end());
        ReversePath(sol, Cycle);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else{
        current_obj += delta;
    }
    return delta;
}

/*
void ILS::SelectNegativeCycle(Graph& G, Solution& sol){
    // USE THIS SOLELY NOW FOR CAPACITY VIOLATIONS, NOT FOR TRAVEL DISTANCE
    int cost = NegativeCycleBoost(G, sol);
    // current_obj += cost;
    if (cost < 0){
        NrImprov.at(move_name::NC)++;
    }
}
    */


void ILS::UpdateSelectionProbabilities(){
    // Comes from Andrea's proceedings paper: Reinforcement Learning for Multi-Neighborhood Local Search in Combinatorial Optimization (2023)
    double nrm = 0;
    for (const auto& movename : Moves){
        cout << "reward / NrChosen = " << RewardT[movename] << " /= " << NrChosenT[movename] << endl;
        assert(NrChosenT[movename] > 0);
        RewardT[movename] /= NrChosenT[movename];
        nrm += RewardT[movename];
    }
    double sum = 0;
    for (const auto& movename: Moves){
        cout << "weight = max((1.0-" << lambda << ")*" <<  Weights[movename] << " + " << lambda << "*" << RewardT[movename] << "/" << nrm << "), " << MinWeight << ")" << endl;
        if (nrm > 0){
            Weights[movename] = std::max((1.0-lambda)*Weights[movename] + lambda*(RewardT[movename]/nrm), MinWeight);
            sum += Weights[movename];
            WeightsCumul[movename] = sum;
            cout << "Weight " << move_name_string.at(movename) << " = " << Weights[movename] << endl;
        }
        RewardT[movename] = 0;
        NrChosenT[movename] = 0;
    }
    // cin.get();
    assert(0.9999999 <= sum && sum <= 1.000001);
}

void ILS::Update(){
    if (it % 100 == 0){
        current_time = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration time_diff = current_time - start_time;
        auto duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
        if ((int)duration_gap.count() > TIME_LIMIT){
            stop = true;
        }
    }
    if (current_obj >= previous_obj){
        ++nr_no_improv;
    }
    else if (current_obj < best_obj){
        // print_solution(obj, obj_before);
        best_obj = current_obj;
        nr_no_improv = 0;
        if (optimal_obj_provided){
            if (best_obj <= optimal_obj+0.5){
                stop = true;
            }
            double gap = (double)(current_obj-optimal_obj)/(double)current_obj;
            current_time = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::duration time_diff = current_time - start_time;
            auto duration_gap = std::chrono::duration_cast<std::chrono::seconds>(time_diff);
            for (int g = 0; g < Gaps.size(); ++g){
                if (TimesGaps[g] < 0 && gap <= Gaps[g]){
                    TimesGaps[g] = (int) duration_gap.count();
                }
            }
            time_best_obj = (int)duration_gap.count();
        }
    }
    // Update SA parameters
    if (it > 0 && it % MaxMoves == 0 /*|| NrAcceptedMovesT > gamma_johnson*MaxMoves*/){
        T *= alpha;
        NrAcceptedMovesT = 0;
        cout << "T = " << T << endl;
        // cin.get();
        if (T < T_min){
            stop = true;
        }
        // UpdateSelectionProbabilities(); -> Weird behavior?
    }
    if (nr_no_improv > MAX_NO_IMPROV){
        cout << "PERTUBE" << endl;
        pertube = true;
        nr_no_improv = 0;
    }
}

void ILS::Move(Solution& sol){
    include_travel = true;
    include_HAP = true;
    bool bipartite;
    std::random_device rd;                          
    std::mt19937 gen(rd());                         
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double rnd = dis(gen);
    move_name selected_move;
    int delta;
    if (rnd < WeightsCumul[move_name::TS]){
        cout << "TS" << endl;
        selected_move = move_name::TS;
        delta = SelectTS(sol);
    }
    else if (rnd < WeightsCumul[move_name::PTS]){
        cout << "PTS" << endl;
        selected_move = move_name::PTS;
        delta = SelectPTS(sol);
    }
    else if (rnd < WeightsCumul[move_name::PRS]){
        cout << "PRS" << endl;
        selected_move = move_name::PRS;
        delta = SelectPRS(sol);
    }
    else if (rnd < WeightsCumul[move_name::M]){
        cout << "Matching" << endl;
        // Ik weet niet of dit heel goed werkt.. lijkt enkel te werken als je random matchings neemt 
        // en niet enkel de beste qua travel distance
        selected_move = move_name::M;
        bipartite = false;
        const int l = rand()%sol.getNrLeagues();
        delta = SelectMatching(l, sol, bipartite);
    }
    else if (rnd < WeightsCumul[move_name::BM]){
        cout << "Bipartite matching" << endl;
        selected_move = move_name::BM;
        if (include_HAP){
            bipartite = true;
        }
        const int l = rand()%sol.getNrLeagues();
        delta = SelectMatching(l, sol, bipartite);
    }
    else{
        cout << "Balanced cycle" << endl;
        selected_move = move_name::C;
        const int l = rand()%sol.getNrLeagues();
        delta = SelectBalancedCycle(l, sol);
    }
    // sol.TestNrH_equal();
    // cout << current_obj << " <= " << previous_obj << endl;
    // assert(current_obj <= previous_obj);
    // cout << sol.ComputeTotalCost() << " == " << current_obj << endl;
    if (include_HAP){
        sol.validate();
        // assert(sol.ComputeCostCapacities() == 0);
        // assert(sol.ComputeTotalHACost() == 0);
    }
    if (delta < INT_MAX - 100){
        // pts: we can introduce infeasible games
        // do not punish this (all other moves are always feasible)
        NrChosen.at(selected_move)++;
        NrChosenT.at(selected_move)++;
    }
    if (delta < 0){
        RewardT[selected_move] += abs((double)delta);
    }
    assert(sol.ComputeTotalCost() == current_obj);
}

void ILS::solve(Solution& sol){

    start_time = std::chrono::high_resolution_clock::now();
    current_time = std::chrono::high_resolution_clock::now();

    current_obj = sol.ComputeTotalCost();
    previous_obj = current_obj;
    best_obj = current_obj;

    cout << "Start" << endl;

    while(!stop){
        ++it;
        int g = rand()%sol.getNrLeagues();
        Move(sol); // do random move
        print_solution();
        Update();
        previous_obj = current_obj;
    }
    cout << "ILS done " << endl;
}

void ILS::OutputNB(ofstream& output){
    output << " Output moves travel: ";

    for (const auto& mv: Moves){
        output << move_name_string.at(mv) << " = " << (double) NrImprov.at(mv) / (double) NrChosen.at(mv) << ", ";
    }

    // boxplots->with python?
}

/*
void ILS::repairHAPs(Solution& sol){
    // repair the solution!!
    cout << "Repair HAPs" << endl;
    include_travel = false;
    include_HAP = true;
    int previous_obj_r = sol.ComputeTotalHACost();
    while (sol.ComputeTotalHACost() > 0){
        const int g = rand()%sol.getNrLeagues();
        Graph& G = sol.getLeague(g);
        int rnd = rand()%10;
        cout << "HAP violations: " << sol.ComputeTotalHACost() << endl;
        if (rnd < 2){
            // cout << "PRS" << endl;
            NrChosenHAP.at(move_name::PRS)++;
            SelectPRS(G, sol);
        }
        else if (rnd < 5){
            // cout << "TS" << endl;
            // Geen effect op kost 3 consecutive HA, nr of breaks, etc, want dit verwisselt gewoon deze kosten tussen de teams!!!
            // Daarom werkt het ook zo goed: het houdt de rest in orde en kan enkel op capaciteiten focussen!!!
            NrChosenHAP.at(move_name::TS)++;
            // SelectTS(G, sol);
            SelectPTS(G, sol);
        }
        else if (rnd < 7){
            // Ik weet niet of dit heel goed werkt.. lijkt enkel te werken als je random matchings neemt 
            // en niet enkel de beste qua travel distance
            bool bipartite = false;
            SelectMatching(G, sol, bipartite);
        }
        else if (rnd < 8){
            bool bipartite = true;
            SelectMatching(G, sol, bipartite);
        }

        sol.validate();
        // cout << sol.ComputeHACost() << " <= " << previous_obj_r << endl;
        assert(sol.ComputeTotalHACost() <= previous_obj_r);
        previous_obj_r = sol.ComputeTotalHACost();
        // G.print_all_rounds();
        // cin.get();
    }
}

void ILS::Perturbation(const int g, Solution& sol){
    Graph& G = sol.getLeague(g);
    pertube = true;
    // TS does not give good solutions
    if (!include_HAP){
        for (int k = 0; k < 1; ++k){
            // do x consecutive random partial team swaps
            bool HA_only = false;
            SelectPTS(G, sol);
        }
    }
    else if (include_HAP){
        include_travel = false;
        include_HAP = false;
        SelectTS(G, sol);
        include_HAP = true;
    }
    // cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    // cout << "HA_cost = " << sol.ComputeHACost() << endl;
    // cin.get();
    pertube = false;
    if (include_HAP){
        repairHAPs(sol);
    }
    // cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    // cout << "HA_cost = " << sol.ComputeHACost() << endl;
    // cin.get();
}
*/

/*

void ILS::SelectRS(Graph& G, Solution& sol){
    // rounds in current schedule
    int r = rand()%sol.getNrRounds();
    int s = ((r+1)+(rand()%(sol.getNrRounds()-1)))%sol.getNrRounds();
    int cost = CostEvaluationRS(G, r, s, sol, include_travel, include_HAP);
    if (Accept(cost)){
        RS(G, r, s);
        current_obj += cost; 
    }
    sol.resetNrH_temp();
}
*/