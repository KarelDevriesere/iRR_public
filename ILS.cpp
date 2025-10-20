#include "ILS.h"

#include "Operators.h"
#include <assert.h>
#include <cmath>
#include <random>
#include <algorithm>

ILS::ILS(const std::unordered_map<move_name, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<move_name, double>& weights, std::mt19937& g): SA<move_name>(moves, weights, g){
            for (auto& [move, name]: moves){
                if (FailureReasonAll.count(move)){
                    FailureReason[move] = FailureReasonAll.at(move);
                }
            }
}

ILS::~ILS(){}

vector<vector<int>> MakeTeamColorOppCopy(const Solution& sol){
    vector<vector<int>>TeamColorOppCopy(sol.getNrTeams(), vector<int>(sol.getNrRounds(), -1));
    for (int t = 0; t < sol.getNrTeams(); ++t){
        for (int c = 0; c < sol.getNrRounds(); ++c){
            TeamColorOppCopy[t][c] = sol.TeamColorOpp[t][c];
        }
    }
    return TeamColorOppCopy;
}

void ResetTeamColorOpp(Solution& sol, const vector<vector<int>>TeamColorOppCopy){
    for (int t = 0; t < sol.getNrTeams(); ++t){
        for (int c = 0; c < sol.getNrRounds(); ++c){
            sol.TeamColorOpp[t][c] = TeamColorOppCopy[t][c];
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

void ResetOrientations(Solution& sol, const vector<vector<HA>>OrientationsCopy){
    for (int t = 0; t < sol.getNrTeams(); ++t){
        for (int c = 0; c < sol.getNrRounds(); ++c){
            sol.Orientation[t][c] = OrientationsCopy[t][c];
        }
    }
}

vector<vector<int>> MakeMatchColorCopy(const Solution& sol){
    vector<vector<int>>MatchColorCopy(sol.getNrTeams(), vector<int>(sol.getNrTeams(), -1));
    for (int i = 0; i < sol.getNrTeams(); ++i){
        for (int j = 0; j < sol.getNrTeams(); ++j){
            MatchColorCopy[i][j] = sol.MatchColor[i][j];
        }
    }
    return MatchColorCopy;
}

void ResetMatchColorCopy(Solution& sol, const vector<vector<int>>MatchColorCopy){
    for (int i = 0; i < sol.getNrTeams(); ++i){
        for (int j = 0; j < sol.getNrTeams(); ++j){
            sol.MatchColor[i][j] = MatchColorCopy[i][j];
        }
    }
}

bool ILS::RepairHAPs(Solution& sol){
    // Be economical: only try to repair the HAPs when we know it will deliver something!!
    /*
    if (sol.ComputeTravelCost() < best_obj){
        NrTimesRepairHapChosen++;
        // first, try to repair the HAPs by finding negative cycles
        const int l = sol.getNrLeagues()-1; // TODO pass the league as input
        // cout << "try to repair haps" << endl;
        if (RepairHAPsWithNegativeCycles(sol, l)){
            NrTimesRepairHapSuccesfull++;
            Update(sol, sol.ComputeTotalCost());
            // cout << "succesfull" << endl;
            return true;
        }
        else{
            // We need this resetting because in the function RepairHAPsWithNegativeCycles() we make incremental changes but if things do not work out
            // In the end we need to revert everything..
            FailureReason.at(CurrentMove).at(failure::HAPs)++;
            // cout << "unsuccesfull" << endl;
            return false;
        }
    }
    else{
        return false;
    }
    */
   return false;
}

bool ILS::veto_haps(Solution& sol){
    // Repairing the HAPs does not seem to have a lot of impact..
    // Repairing haps seems to be very slow, almost 15x more moves without repairing haps..
    // TODO: should not be possible that it takes so long???
    // NrTimesRepairHapChosen++;
    if (!sol.ViolationHAP_allowed && sol.ComputeTotalHACost() > 0){
        return true; 
    }
    else{
        return false;
    }
}

pair<int,int>ILS::SelectTwoTeamsSameLeague(Solution& sol){
    int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    int i_ = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int i = sol.getTeamsLeague(l)[i_];
    int j_ = ((i+1)+(RandomIntegerNumber(0,sol.getNrTeamsLeague(l)-2)))%sol.getNrTeamsLeague(l); 
    int j = sol.getTeamsLeague(l)[j_];
    assert(sol.getLeagueTeam(i) == l && sol.getLeagueTeam(j) == l);
    return {i,j};
}

array<int,3>ILS::SelectTwoTeamsSameLeagueAndColor(Solution& sol){
    int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    int k_ = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int k = sol.getTeamsLeague(l)[k_]; 
    int i_max = -1;
    int c_max = -1;
    int d_max = -1;
    int i;
    vector<int>TeamSeen(sol.getNrTeams(),0);
    for (int r = 0; r < sol.getNrRounds(); ++r){
        i = sol.TeamColorOpp[k][r];
        if (sol.getDistanceTeams(i,k) > d_max){
            i_max = i;
            c_max = r;
            d_max = sol.getDistanceTeams(i,k);
        }
        TeamSeen[i]++;
    }
    int j_best = -1;
    int j_;
    for (j_ = 0; j_ < sol.getNrTeamsLeague(l); ++j_){
        int j = sol.getTeamsLeague(l)[j_];
        if (sol.isEligible(j,k) && sol.getDistanceTeams(k,j) < d_max && TeamSeen[j] < 2){
            j_best = j;
        }
    }
    if (j_best == -1){
        j_ = ((k_+1)+(RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-2)))%sol.getNrTeamsLeague(l); 
        j_best = sol.getTeamsLeague(l)[j_];
        int StartColor = RandomIntegerNumber(0, sol.getNrRounds()-1);
        while (sol.TeamColorOpp[i][StartColor] == j_best){
            // while loop bc if DRR, it can happen that i plays vs j in k but also in k+1!!
            ++StartColor %= sol.getNrRounds();
        }
    }
    assert(sol.getLeagueTeam(i) == l && sol.getLeagueTeam(j_best) == l);
    return {i_max,j_best,c_max};
}

void ILS::SelectTS(Solution& sol){ // use TS for perturbation move!!
    // Andrea doet TS net zoals mij, dus enkel de HAPs van i en j veranderen!!
    // I do not use Eligible opponents since teams i and j do not necessarily need to be eligible..
    // For example, i can be of strength 1 and j of strength 3 if they share opponents of only strength 2..
    pair<int,int> pair = SelectTwoTeamsSameLeague(sol);
    int i = pair.first, j = pair.second; 
    if (!sol.ViolationEligibleOpponents_allowed && !TS_feasible(sol, i, j)){
        FailureReason.at(move_name::TS).at(failure::InfeasibleOpponents)++;
        return;
    }
#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
#endif
    // vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    // vector<vector<int>>MatchColorCopy = MakeMatchColorCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    // vector<vector<int>>TeamColorOppCopy = MakeTeamColorOppCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    // int cost_before = current_obj;
    TS(sol, i, j);
    assert(sol.IsTeamBalanced(i));
    assert(sol.IsTeamBalanced(j));
    if (veto_haps(sol) || !Update(sol, sol.ComputeTotalCost())/*!AcceptMove*/){
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

void ILS::SelectPTS(Solution& sol){
    // Heel generic: create lantarn, bepaal random orientaties, reken kost uit
    // Opnieuw: teams uit zelfde leagues
    // kan het zijn dat een middle team nu meer als 2 edges heeft?????
    array<int,3>triple = SelectTwoTeamsSameLeagueAndColor(sol);
    int i = triple[0], j = triple[1], StartColor = triple[2];
    // cout << "i : " << i << " and j = " << j << endl;

    // vector<vector<int>>MatchColorCopy = MakeMatchColorCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    // vector<vector<int>>TeamColorOppCopy = MakeTeamColorOppCopy(sol); // can be outcommented when not repairing haps in veto_haps()

#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
#endif

    // int cost_before = current_obj;
    Lantarn lantarn = CreateLantarn(sol, i, j, StartColor);
    // cout << "lantarn created" << endl;
    // PrintLantarn(sol, lantarn);
    // cin.get();
    if (!sol.ViolationEligibleOpponents_allowed && lantarn.InfeasibleOpponents){
        // It can happen that, if we have an infeasible color, we introduce infeasible games
        // e.g. i plays vs k, k has fictive color vs j, but j cannot play vs k..
        FailureReason.at(move_name::PTS).at(failure::InfeasibleOpponents)++;
        return; 
    }
    if (MaxSameClubViolated(sol, lantarn)){
        FailureReason.at(move_name::PTS).at(failure::MaxSameClub)++;
        return;
    }
    // cout << "Original lantarn:" << endl;
    // PrintLantarn(G, lantarn);
    // int cost_travel = CostEvaluationTravelLantarn(G, sol, lantarn);

    // Make a copy of the orientations
    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy(sol);
    vector<array<int,3>>path; // fill the path so that we know 
    const bool CostMinP = false;
    const bool CM = false;
    if (!SetOrientationsEdgesLantarn(sol, lantarn, OrientationsCopy, path, CostMinP, CM)){
        // if not, orientations could not be set such that balance is kept
        FailureReason.at(move_name::PTS).at(failure::NoPathFound)++;
        return;
    }
    if (!path.empty()){
        if (path.size() == 2){
            NrSingleEdgePathChosen++;
        }
        else{
            NrMultiEdgePathChosen++;
        }
    }
    // cout << "Orientations optimized" << endl;
    //cin.get();
    // Now, we have set all the orientations in the right directions, so we can proceed with the path
    SwapColorsLantarn(sol, lantarn);
    //cin.get();

#ifndef NDEBUG
    for (int i = 0; i < sol.getNrTeams(); ++i){
        assert(sol.IsTeamBalanced(i));
    }
#endif

    bool haps_ok = true;
    if (veto_haps(sol)){
        haps_ok = false;
        FailureReason.at(move_name::PTS).at(failure::HAPs)++;
    }
    else{
        // cout << "obj = " << sol.ComputeTotalCost() << ", current obj = " << current_obj << endl;
        // cin.get();
    }
    if (!haps_ok || !Update(sol, sol.ComputeTotalCost())/*!AcceptMove*/){
        // Now, unfortunately, we violated the HAP constraints
        ResetOrientations(sol, OrientationsCopy); 
        SwapColorsLantarn(sol, lantarn);
        if (!path.empty()){
            ReversePath(sol, path);
        }
        // ResetMatchColorCopy(sol, MatchColorCopy); // can be outcommented when not repairing haps in veto_haps()
        // ResetTeamColorOpp(sol, TeamColorOppCopy); // can be outcommented when not repairing haps in veto_haps()
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif

        /*
        cout << "old lantarn: " << endl;
        PrintLantarn(sol, lantarn);
        cin.get();
        */
    }
    else if (!path.empty()){
        if (path.size() == 2){
            NrSingleEdgePathSuccesful++;
        }
        else{
            NrMultiEdgePathSuccesful++;
        }
    }
    assert(sol.validate());
    return;
}

void ILS::SelectPRS(Solution& sol){
    // rounds in current schedule
    // USE THIS SOLELY NOW FOR CAPACITY VIOLATIONS, NOT FOR TRAVEL DISTANCE
    const int r = RandomIntegerNumber(0,sol.getNrRounds()-1);
    const int s = ((r+1)+(RandomIntegerNumber(0,sol.getNrRounds()-2)))%sol.getNrRounds();
    const int StartNode = RandomIntegerNumber(0,sol.getNrTeams()-1);
    int cost_before = sol.ComputeTotalCost();
    int cost_HA_before = sol.ComputeTotalHACost();
    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    vector<vector<int>>MatchColorCopy = MakeMatchColorCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    vector<vector<int>>TeamColorOppCopy = MakeTeamColorOppCopy(sol); // can be outcommented when not repairing haps in veto_haps()
    // int cost_before = current_obj;
    PRS(sol, r, s, StartNode);
    int cost_after = sol.ComputeTotalCost();
    int delta = cost_after - cost_before;
    /*
    bool AcceptMove = false;
    if (veto_haps(sol) || !Update(sol, sol.ComputeTotalCost())){
        if (RepairHAPs(sol)){
            AcceptMove = true;
        }
    }
    else{
        AcceptMove = true;
    }
    */
    if (veto_haps(sol) || !Update(sol, sol.ComputeTotalCost()) /*!AcceptMove*/){
        // TODO not efficient
        ResetOrientations(sol, OrientationsCopy); // can be outcommented when not repairing haps in veto_haps()
        ResetMatchColorCopy(sol, MatchColorCopy); // can be outcommented when not repairing haps in veto_haps()
        ResetTeamColorOpp(sol, TeamColorOppCopy); // can be outcommented when not repairing haps in veto_haps()
        // PRS(sol, r, s, StartNode); // should be put back if other things are outcommented for going back to original
        assert(sol.ComputeTotalHACost() == cost_HA_before);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    return;
}

void ILS::SelectMatching(const int l, Solution& sol, const bool bipartite){

    // Matching only of one league
    // bipartite matching: throw away the matching in round r but keep orientations of the teams. Then, find a new matching
    // this will always succeed because we can always go back to the old matching

    assert(sol.validate());
    // cout << "Travel cost before: " << sol.ComputeTravelCost() << endl;

    const int r = RandomIntegerNumber(0,sol.getNrRounds()-1); // Chose a random round to do the matching

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

    // I will make it myself easy: also make a copy of MatchColor:

    /*
    vector<vector<int>>MatchColorCopy = MakeMatchColorCopy(sol);
    vector<vector<HA>>OrientationsCopy = MakeOrientationsCopy(sol);
    */
    vector<pair<int,int>>OriginalMatching(sol.getNrTeamsLeague(l)/2); // so that we can go back if things don't work out
    vector<bool>NodeSeen(sol.getNrTeams(), false);
    int j;
    int m = 0;
    int delta = 0;
    for (auto& t: sol.getTeamsLeague(l)){
        if (!NodeSeen[t]){
            j = sol.TeamColorOpp[t][r];
            NodeSeen[j] = true;
            OriginalMatching[m++] = {t,j};
            delta -= sol.getDistanceTeams(t,j);
            // cout << t << "-" << j << endl;
        }
    }
#ifndef NDEBUG
    int cost_before = sol.ComputeTotalCost();
#endif
    // int cost_before = current_obj;

    // cout << "Matching" << endl;
    bool keepHAP = true;

    // cout << "Rounds before matching in round " << r << endl;
    // sol.PrintAllRoundsLeague(l);

    // cout << "do MoveMWPM" << endl;
    const bool CM = false;
    const bool CostMinM = true;
    pair<vector<pair<int,int>>,vector<int>>Matching_MatchingOpponent = MoveMWPM(sol, l, r, bipartite, keepHAP, CM, gen, CostMinM); // in the file operators
    // cout << "MoveMWPM done" << endl;
    vector<pair<int,int>>Matching = Matching_MatchingOpponent.first;
    SwapMatchings(sol, Matching, l, r, bipartite);
    bool unable_to_find_reversed_paths = false;
    /*
    if (!bipartite && keepHAP){
        Paths = ReversePathsMatching(sol, Matching, l, r);
        if (Paths.empty()){
            unable_to_find_reversed_paths = true;
            FailureReason.at(move_name::M).at(failure::NoPathFound)++;
        }
    }

    if (!unable_to_find_reversed_paths){
        assert(sol.ComputeCost2RRConstraint() == 0);
        assert(sol.ComputeCostNonEligibleOpponents() == 0);
        assert(sol.ComputeCostSameClub() == 0);
    }
        */

    /*
    cout << "cost 3HA = " << sol.ComputeCostThreeConsecutive() << endl;
    cout << "cost break limit = " << sol.ComputeCostBreakLimit() << endl;
    cout << "cost break beginning end = " << sol.ComputeCostBreakBeginningEnd() << endl;
    cout << "cost quarter balanced = " << sol.ComputeCostQuarterBalanced() << endl;
    */

    // int cost_after = sol.ComputeTotalCost();
    // int delta = cost_after - cost_before;
    // cout << "cost travel = " << sol.ComputeTravelCost() << endl;
    // cout << "cost capacities = " << sol.ComputeCostCapacities() << endl;
    // cout << "delta = " << delta << endl;
    // cin.get();

    /*
   bool AcceptMove = false;
   if (!unable_to_find_reversed_paths){
        if (veto_haps(sol) || !Update(sol, sol.ComputeTotalCost())){
            if (RepairHAPs(sol)){
                AcceptMove = true;
            }
        }
        else{
            AcceptMove = true;
        }
    }
    */

    /*
    if (veto_haps(sol) || unable_to_find_reversed_paths || !Update(sol, current_obj+delta)){
        // ResetOrientations(sol, OrientationsCopy);
        // cout << "swap matchings back" << endl;
        // De MatchColors van de paden moeten ook terug goed gezet worden!!!!!
        SwapMatchings(sol, OriginalMatching, l, r, bipartite);
        for (int i = 0; i < sol.getNrTeams(); ++i){
            for (int j = 0; j < sol.getNrTeams(); ++j){
                sol.MatchColor[i][j] = MatchColorCopy[i][j];
            }
        }
        for (auto&[i,j]: OriginalMatching){
            sol.TeamColorOpp[i][r] = j;
            sol.TeamColorOpp[j][r] = i;
        }
        // cout << "Rounds after swapping to original matching" << endl;
        // sol.PrintAllRoundsLeague(l);
#ifndef NDEBUG
        assert(sol.ComputeTotalCost() == cost_before);
#endif
    }
    */

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

    Update(sol, current_obj+delta);

#ifndef NDEBUG
    assert(cost_before >= sol.ComputeTotalCost());
#endif

    assert(sol.validate());
    // cout << "after: check" << endl;
    return;
} 

void ILS::SelectBalancedCycle(const int l, Solution& sol){

    // DOES GIVE ERRORS!

    vector<array<int,3>>Cycle = CycleBalanced(sol,gen);
    int cost_before = sol.ComputeTotalCost();
    // int cost_before = current_obj;

    // Calculate the cost of reversing this cycle
    /*
    int cost_cycle_before = sol.ComputeCostCapacities(); // TODO: can be more efficient
    // cout << "Cycle: " << endl;
    for (int i = 0; i < Cycle.size()-1; ++i){ // the last element is also the first element!!
        cost_cycle_before += sol.ComputeHACostTeam(l, Cycle[i]);
    }
    */
    // cout << "cost before = " << cost_cycle_before << endl;

    assert(Cycle[0][0] == Cycle[(int)Cycle.size()-1][1]);
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

    if (veto_haps(sol) || cost2RR > 0 || !Update(sol, sol.ComputeTotalCost())){
        // If not better: reverse the cycle again as if nothing happened
        if (cost2RR > 0){
            FailureReason.at(move_name::C).at(failure::DRR)++;
        }
        ReversePath(sol, Cycle);
        assert(sol.ComputeTotalCost() == cost_before);
    }
    else{
        // current_obj += delta;
    }
    return;
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

void ILS::Move(Solution& sol){
    include_travel = true;
    include_HAP = true;
    bool bipartite;
    double rnd = RandomDoubleNumber(0.0, 1.0);
    auto iterator = WeightsCumul.upper_bound(rnd);
    CurrentMove = iterator->second;
    // cout << Moves.at(CurrentMove) << endl;
    auto beg = std::chrono::high_resolution_clock::now();
    if (CurrentMove == move_name::TS){
        SelectTS(sol);
    }
    else if (CurrentMove == move_name::PTS){
        SelectPTS(sol);
    }
    else if (CurrentMove == move_name::PRS){
        SelectPRS(sol);
    }
    else if (CurrentMove == move_name::M){
        // Ik weet niet of dit heel goed werkt.. lijkt enkel te werken als je random matchings neemt 
        // en niet enkel de beste qua travel distance
        bipartite = false;
        const int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
        SelectMatching(l, sol, bipartite);
    }
    else if (CurrentMove == move_name::BM){
        if (include_HAP){
            bipartite = true;
        }
        const int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
        SelectMatching(l, sol, bipartite);
    }
    else{
        const int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
        SelectBalancedCycle(l, sol);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - beg);
    ExecutionTimes.at(CurrentMove).push_back(dur);
    NrChosen.at(CurrentMove)++;
    // sol.TestNrH_equal();
    // cout << current_obj << " <= " << previous_obj << endl;
    // assert(current_obj <= previous_obj);
    // cout << sol.ComputeTotalCost() << " == " << current_obj << endl;
    if (include_HAP){
        assert(sol.validate());
        // assert(sol.ComputeCostCapacities() == 0);
        // assert(sol.ComputeTotalHACost() == 0);
    }
}

void ILS::SaveResultsFailures(std::string file_path_results_base, int inst, int seed){
    std::string file_path_results_failures = file_path_results_base + std::string(PATHSEP) + "Failures" + std::string(PATHSEP) + to_string(inst) + "_" + to_string(seed) + ".txt";
    std::ofstream file(file_path_results_failures);
    file << "Instance,Seed,TS-InfOpp,TS-HAP,PTS-InfOpp,PTS-HAP,PTS-DRR,PTS-NoPath,PRS-HAP,M-HAP,M-NoPath,C-DRR\n";
    file << inst << "," << seed << ",";
#ifdef PRINT
#if PRINT == 1
    cout << "Nr times we tried to repair HAPs = " << NrTimesRepairHapChosen << endl;
    cout << "Nr times this was actually succesfull = " << NrTimesRepairHapSuccesfull << endl;
    cout << "Nr times we found single edge path: " << NrSingleEdgePathChosen << endl;
    cout << "Nr of times this was effective: " << NrSingleEdgePathSuccesful << endl;
    cout << "Nr times we found multi edge path: " << NrMultiEdgePathChosen << endl;
    cout << "Nr of times this was effective: " << NrMultiEdgePathSuccesful << endl;
    cout << "Analysing why moves failed:" << endl;
#endif
#endif
    int cnt1 = 0, cnt2 = 0;
    for (const auto& [move, FailureCount]: FailureReason){
#ifdef PRINT
#if PRINT == 1
        cout << "Move: " << Moves.at(move) << endl;
#endif
#endif
        cnt1++;
        cnt2 = 0;
        for (const auto& [failure, count]: FailureCount){
#ifdef PRINT
#if PRINT == 1
            cout << Failures.at(failure) << ": " << count << endl;
#endif
#endif
            cnt2++;
            if (cnt1 ==  FailureReason.size() && cnt2 ==  FailureCount.size()){
                file << count << "\n";
            }
            else{
                file << count << ",";
            }
        }
    }
    file.close();
}

void ILS::solve(Input& in, Solution& sol){

    // start_time = std::chrono::high_resolution_clock::now(); Take time to find initial solution into the duration
    best_obj = sol.ComputeTotalCost();
    current_obj = best_obj;
    UpdateBestSolution(sol);

    // cout << "Start" << endl;

    do {
        Move(sol); // do random move
    }
    while(!STOP);
    SaveBestSolution(sol);
    sol.validate();
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