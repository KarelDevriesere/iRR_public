#include "GreedyMatching.h"
#include "assert.h"
#include <algorithm>

void FindScheduleWithIP(Input& in, Solution& sol){
    GurSolver gur(in);
    const bool relax_x = false;
    gur.build_all(true, relax_x);
    gur.setTimeLimit(6000);
    gur.setBoundCapacityViolations();
    for (int i = 0; i < sol.getNrTeams(); ++i){
		gur.HapFixed[i] = true;
	}
    gur.FixHAP(sol);
    gur.AddObj(true, false);
    gur.solve();
}

// HAP operators of Li et al:

void printHAP(Solution& sol, const int i){
    cout << "Team " << i << " has HAP index " << sol.getHAPIndexTeam(i) << " : ";
    for (int r = 0; r < sol.getNrRounds(); ++r){
        if (sol.Orientation[i][r] == HA::H){
            cout << "H";
        }
        else if (sol.Orientation[i][r] == HA::A){
            cout << "A";
        }
        else{
            cout << "?";
        }
    }
    cout << endl;
}

void SwapHAPs(Solution& sol, const int i, const int j){
    // cout << "Previous:" << endl;
    // printHAP(sol, i);
    // printHAP(sol, j);
    for (int r = 0; r < sol.getNrRounds(); ++r){
        std::swap(sol.Orientation[i][r], sol.Orientation[j][r]);
    }
    int h1 = sol.getHAPIndexTeam(i);
    int h2 = sol.getHAPIndexTeam(j);
    sol.setHAPIndexTeam(i, h2);
    sol.setHAPIndexTeam(j, h1);
    // cout << "New:" << endl;
    // printHAP(sol, i);
    // printHAP(sol, j);
    // cin.get();
}

void setHAP(Solution& sol, const int i, const int h){
    for (int r = 0; r < sol.getNrRounds(); ++r){
        sol.Orientation[i][r] = sol.getModeHAPRound(h, r);
    }
    sol.setHAPIndexTeam(i, h);
}

void GreedyMatching::SetAllOpponents(Solution& sol){
    for (int i = 0; i < sol.getNrTeams(); ++i){
        for (int r = 0; r < sol.getNrRounds(); ++r){
            Opponents[i][r] = sol.TeamColorOpp[i][r];
        }
    }
}

void GreedyMatching::SetOpponentsCurrentLeague(Solution& sol){
    for (int i = 0; i < sol.getNrTeamsLeague(CurrentLeague); ++i){
        int i_ = sol.getGlobalIndexTeam(CurrentLeague, i);
        for (int r = 0; r < sol.getNrRounds(); ++r){
            Opponents[i_][r] = sol.TeamColorOpp[i_][r];
        }
    }
}

GreedyMatching::GreedyMatching(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, const int NrRounds, std::mt19937& g): HC<Move>(moves, weights, g){
    Rounds = vector<int>(NrRounds);
    for (int r = 0; r < NrRounds; ++r){
        Rounds[r] = r;
    }
}

GreedyMatching::~GreedyMatching(){}

void GreedyMatching::ReverseMove(Solution& sol){
    if (CurrentMove == Move::ComplementInsertion){
        setHAP(sol, team1, hap_index1);
        setHAP(sol, team2, hap_index2);
    }
    else if (CurrentMove == Move::HomeAwaySwap){
        swap(sol.Orientation[team1][round1], sol.Orientation[team2][round1]);
        swap(sol.Orientation[team1][round2], sol.Orientation[team2][round2]);
    }
    else{
        SwapHAPs(sol, team1, team2); // see operators
    }

    int i,j,i_,r;
    for (i_ = 0; i_ < sol.getNrTeamsLeague(CurrentLeague); ++i_){
        i = sol.getGlobalIndexTeam(CurrentLeague,i_);
        for (r = 0; r < sol.getNrRounds(); ++r){
            j = Opponents[i][r];
            sol.TeamColorOpp[i][r] = j;
            sol.MatchColor[i][j] = r;
            sol.MatchColor[j][i] = r;
        }
    }
}

void GreedyMatching::Reset(Solution& sol){
    // cout << "Reset" << endl;
    // Such that we can do the matchings again without conflicts
    // But: do not reset the orientations!!
    int i,j;
    for (int i_ = 0; i_ < sol.getNrTeamsLeague(CurrentLeague); ++i_){
        i = sol.getGlobalIndexTeam(CurrentLeague, i_);
        for (int r = 0; r < sol.getNrRounds(); ++r){
            sol.TeamColorOpp[i][r] = -1;
        }
        for (int j_ = 0; j_ < sol.getNrTeamsLeague(CurrentLeague); ++j_){
            j = sol.getGlobalIndexTeam(CurrentLeague, j_);
            sol.MatchColor[i][j] = -1;
            sol.MatchColor[j][i] = -1;
        }
    }
}

void GreedyMatching::ReAssignHAPs(Solution& sol){
    // cout << "ReAssign HAPs" << endl;
    double rnd;
    bool MoveChosen = false;
    sol.NrColouredRounds = sol.getNrRounds(); // such that capacity costs are computed correctly
    while(!MoveChosen && !STOP){
        rnd = RandomDoubleNumber(0.0, 1.0);
        auto iterator = WeightsCumul.upper_bound(rnd); 
        CurrentMove = iterator->second;
        if (CurrentMove == Move::InterClubSwap){
            MoveChosen = InterClubSwap(sol);
        }
        else if (CurrentMove == Move::IntraClubSwap){
            MoveChosen = IntraClubSwap(sol);
        }
        else if (CurrentMove == Move::RandomSwap){
            MoveChosen = RandomSwap(sol);
        }
        else if (CurrentMove == Move::HomeAwaySwap){
            MoveChosen = HomeAwaySwap(sol);
        }
        else{
            MoveChosen = ComplementInsertion(sol);
        }
        NrChosen.at(CurrentMove)++;
    }
    // cout << "Move = " << Moves.at(CurrentMove) << endl;
}

bool GreedyMatching::SchedulePhase(Solution& sol){
    assert(sol.ComputeTotalHACost() <= 0);
    const int N = sol.getNrTeamsLeague(CurrentLeague);
    const bool bipartite = true;
    int h, a;
    sol.NrColouredRounds = 0; // for computing travel cost teams in TTP

    int s = 0, count = 0;
    // cout << "Try to find schedule for league " << CurrentLeague << endl;
    while (s < sol.getNrRounds()){

        const int r = Rounds[s];
        sol.NrColouredRounds++;
        // cout << "Optimize round " << r << endl;

        // cout << "find matching" << endl;
        const bool CM = false;
        const bool keepHAP = true;
        bool MinCostM = true;
        pair<vector<pair<int,int>>, vector<int>>Matching_OpponentMatching = MoveMWPMOneLeague(sol, r, gen, CurrentLeague, bipartite, MinCostM); // in the file operators
        vector<pair<int,int>>matching = Matching_OpponentMatching.first;
        if ((int)matching.size() < N/2){
            // shuffling rounds does not seem a good idea, instead go back to the old HAP assignement and do a new HAP move
            // cout << "matching failed in round " << s << endl;
            // cout << "matching has size of only " << (int)matching.size() << endl;
            // cin.get();
            ++NrInfeasibleMatchings;
            return false;
            // The problem with reshuffling rounds is that in ComputeEdgeWeight of TTP, we assume the rounds go from 0 to R-1 to compute the cost of trips
            // Hence, it is more difficult to compute the cost of trips!!!
            // So instead of doing this, just leave and try with a new HAP
            /*
            cout << "matching failed, shuffle rounds" << endl;
            shuffle(Rounds.begin(), Rounds.end(), default_random_engine(42));
            Reset(sol);
            s = 0;
            sol.NrColouredRounds = 0;
            if (count++ <= 100){
                return false;
            }
                */
        }
        else{
            // cout << "Matching in round " << r << ":" << endl;
            for (auto& [i, j]: matching){
                if (sol.Orientation[i][r] == HA::H){
                    assert(sol.Orientation[j][r] == HA::A);
                    h = i, a = j;
                }
                else{
                    assert(sol.Orientation[i][r] == HA::A);
                    assert(sol.Orientation[j][r] == HA::H);
                    a = i, h = j;
                }
                // cout << h << ", " << a << endl;
                SetValueCircleMethod(h, a, r, sol);
            }
            ++s;
        }
    }

    ++NrSuccesfullMatchings;
    assert(sol.ComputeTotalHACost() <= 0);

    assert(sol.validate());
    // cout << "Total travel cost = " << sol.ComputeTravelCost() << endl;
    // cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    if (sol.getSetting() == Setting::Football || sol.getSetting() == Setting::Hockey){
        assert(sol.ComputeTravelCost() == sol.ComputeTotalCost());
    }
    else{
        assert(sol.ComputeTravelCostTTP() == sol.ComputeTotalCost());
    }
    return true;
}

bool GreedyMatching::HomeAwaySwap(Solution& sol){
    // pick a random league + random team
    int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    CurrentLeague = l;
    int i_ = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int i = sol.getGlobalIndexTeam(l,i_);
    // pick a random round
    int r = RandomIntegerNumber(0, sol.getNrRounds()-1);
    // find a team that plays the opposite home in round r
    int j_ = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int j = sol.getGlobalIndexTeam(l,j_);
    int start_j = j;
    while (sol.Orientation[j][r] == sol.Orientation[i][r]){
        j_ = (j_+1)%sol.getNrTeamsLeague(l);
        j = sol.getGlobalIndexTeam(l,j_);
        if (start_j == j){
            cout << "Infinite loop over j in HomeAwaySwap!!" << endl;
        }
    }
    // Now pick randomly another round s, such that in this round i plays the opposite home-away status
    int s = RandomIntegerNumber(0, sol.getNrRounds()-1);
    int start_s = s;
    while ((sol.Orientation[i][r] == sol.Orientation[i][s]) || (sol.Orientation[j][r] == sol.Orientation[j][s])){
        s = (s+1)%sol.getNrRounds();
        if (start_s == s){
            cout << "Infinite loop over s in HomeAwaySwap!!" << endl;
        }
    }
    team1 = i, team2 = j;
    round1 = r, round2 = s;
    swap(sol.Orientation[i][r], sol.Orientation[j][r]);
    swap(sol.Orientation[i][s], sol.Orientation[j][s]);

    for (int k: {i,j}){
        for (int q: {r,s}){
            for (int v = max(0, q-3); v <= q; ++v){
                int NrH = 0, NrA = 0;
                for (int w = v; w < min(sol.getNrRounds(), v+4); ++w){
                    if (sol.Orientation[k][w] == HA::H){
                        NrH++;
                    }
                    else{
                        NrA++;
                    }
                }
                if (NrH > 3 || NrA > 3){
                    swap(sol.Orientation[i][r], sol.Orientation[j][r]);
                    swap(sol.Orientation[i][s], sol.Orientation[j][s]);
                    return false;
                }
            }
        }
        assert(sol.ComputeHACostTeam(k) == 0);
    }
    return true;
}

bool GreedyMatching::InterClubSwap(Solution& sol){
    // Must they be of the same league->yes?
    // So modify code!!!

    /*
    int c1_ = RandomIntegerNumber(0,sol.getSingleTeamClubs().size()-1);
    int c1 = sol.getSingleTeamClubs()[c1_];
    int i_ = RandomIntegerNumber(0, sol.getTeamsClub(c1).size()-1);
    int c2_ = ((c1_+1)+(RandomIntegerNumber(0, sol.getSingleTeamClubs().size()-2)))%sol.getSingleTeamClubs().size();
    int c2 = sol.getSingleTeamClubs()[c2_];
    int j_ = RandomIntegerNumber(0, sol.getTeamsClub(c2).size()-1);
    */

    int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    int start_l = l;
    while (sol.getSingleTeamClubsLeague(l).size() < 2){
        l = (++l)%sol.getNrLeagues();
        if (l == start_l){
            cout << "No league with at least 2 single team clubs, adjust weights!!" << endl;
            std::abort();
        }
    }
    CurrentLeague = l;
    int c1_ = RandomIntegerNumber(0,sol.getSingleTeamClubsLeague(l).size()-1);
    int c1 = sol.getSingleTeamClubsLeague(l)[c1_];
    int i_ = RandomIntegerNumber(0, sol.getTeamsClubLeague(c1, l).size()-1);
    int c2_ = ((c1_+1)+(RandomIntegerNumber(0, sol.getSingleTeamClubsLeague(l).size()-2)))%sol.getSingleTeamClubsLeague(l).size();
    int c2 = sol.getSingleTeamClubsLeague(l)[c2_];
    int j_ = RandomIntegerNumber(0, sol.getTeamsClubLeague(c2, l).size()-1);

    int i = sol.getTeamsClubLeague(c1, l)[i_];
    int j = sol.getTeamsClubLeague(c2, l)[j_];
    team1 = i;
    team2 = j;
    // cout << "InterClubSwap: swap HAPs of teams " << i << " and " << j << " of clubs " << c1 << " and " << c2 << endl;
    // cout <<  i << " is in league " << sol.getLeagueTeam(i) << ", j is in league " << sol.getLeagueTeam(j) << endl;
    SwapHAPs(sol, i, j);
    // cout << "cost = " << sol.ComputeCostCapacities() << endl;
    if (sol.ComputeCostCapacities() > 0){
        // cout << "Reject InterClubSwap" << endl;
        SwapHAPs(sol, i, j);
        assert(sol.ComputeCostCapacities() <= 0);
        return false;
    }
    return true;
}

bool GreedyMatching::IntraClubSwap(Solution& sol){
    assert(sol.ComputeCostCapacities() <= 0);
    /*
    int c_ = RandomIntegerNumber(0, sol.getMultiTeamClubs().size()-1);
    int c = sol.getMultiTeamClubs()[c_];
    assert(sol.getTeamsClub(c).size() > 1);
    int i_ = RandomIntegerNumber(0, sol.getTeamsClub(c).size()-1);
    int j_ = ((i_+1)+(RandomIntegerNumber(0, sol.getTeamsClub(c).size()-2)))%sol.getTeamsClub(c).size();
    */

    int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    int start_l = l;
    while (sol.getMultiTeamClubsLeague(l).size() == 0){
        l = (++l)%sol.getNrLeagues();
        if (l == start_l){
            cout << "No Multi team clubs in any league for this instance, adjust weights!!" << endl;
            std::abort();
        }
    }
    CurrentLeague = l;

    int c_ = RandomIntegerNumber(0, sol.getMultiTeamClubsLeague(l).size()-1);
    int c = sol.getMultiTeamClubsLeague(l)[c_];
    assert(sol.getTeamsClubLeague(c, l).size() > 1);
    int i_ = RandomIntegerNumber(0, sol.getTeamsClubLeague(c, l).size()-1);
    int j_ = ((i_+1)+(RandomIntegerNumber(0, sol.getTeamsClubLeague(c, l).size()-2)))%sol.getTeamsClubLeague(c, l).size();

    int i = sol.getTeamsClubLeague(c, l)[i_];
    int j = sol.getTeamsClubLeague(c, l)[j_];
    team1 = i;
    team2 = j;
    // cout << "IntraClubSwap: swap HAPs of teams " << i << " and " << j << " of clubs << " << c << " and " << c << endl;

    SwapHAPs(sol, i, j);
    // cout << "cost = " << sol.ComputeCostCapacities() << endl;
    assert(sol.ComputeCostCapacities() <= 0); // This because they are from the same club!!
    /*
    if (sol.ComputeCostCapacities() > 0){
        cout << "Reject IntraClubSwap" << endl;
        SwapHAPs(sol, i, j);
        return false;
    }
        */
    return true;
}

bool GreedyMatching::RandomSwap(Solution& sol){
    int i,j,c1,c2;
    if (sol.getNrLeagues() <= 1){
        c1 = RandomIntegerNumber(0, sol.getNrClubs()-1);
        assert(sol.getTeamsClub(c1).size() > 0);
        int i_ = RandomIntegerNumber(0, sol.getTeamsClub(c1).size()-1);
        c2 = ((c1+1)+(RandomIntegerNumber(0, sol.getNrClubs()-2)))%sol.getNrClubs();
        int j_ = RandomIntegerNumber(0, sol.getTeamsClub(c2).size()-1);
        assert(sol.getTeamsClub(c2).size() > 0);

        i = sol.getTeamsClub(c1)[i_];
        j = sol.getTeamsClub(c2)[j_];
    }
    else{
        int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
        CurrentLeague = l;
        int i_ = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
        int j_ = -1;
        do{
            j_ = ((i_+1)+(RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-2)))%sol.getNrTeamsLeague(l);
        }
        while (sol.getTeamClub(sol.getGlobalIndexTeam(l,i_)) == sol.getTeamClub(sol.getGlobalIndexTeam(l,j_)));

        i = sol.getGlobalIndexTeam(l,i_);
        j = sol.getGlobalIndexTeam(l,j_);
        c1 = sol.getTeamClub(sol.getGlobalIndexTeam(l,i_));
        c2 = sol.getTeamClub(sol.getGlobalIndexTeam(l,j_));
    }

    team1 = i;
    team2 = j;
    if (sol.getSetting() == Setting::Football){
        assert(sol.getTeamClub(i) != sol.getTeamClub(j));
    }
    // cout << "RandomSwap: swap HAPs of teams " << i << " and " << j << " of clubs << " << c1 << " and " << c2 << endl;
    SwapHAPs(sol, i, j);
    // cout << "cost = " << sol.ComputeCostCapacities() << endl;
    if (sol.ComputeCostCapacities() > 0){
        // cout << "reject RandomSwap" << endl;
        SwapHAPs(sol, i, j);
        assert(sol.ComputeCostCapacities() <= 0);
        return false;
    }
    return true;
}

bool GreedyMatching::ComplementInsertion(Solution& sol){
    // TODO
    // Also fill TeamsHAP!!
    // Given two teams with complementary HAPs, replace their patterns with a newly chosen pair of 
    // complementary HAPs from H. 
    int l = RandomIntegerNumber(0, sol.getNrLeagues()-1);
    CurrentLeague = l;
    if (sol.getNrLeagues()==1 && sol.getNrTeamsLeague(0) != sol.getNrTeams()){
        cout << "sol.getNrTeamsLeague(0) != sol.getNrTeams() in ComplementInsertion()" << endl;
        std::abort();
    }
    int i_ = RandomIntegerNumber(0, sol.getNrTeamsLeague(l)-1);
    int i = sol.getGlobalIndexTeam(l,i_);
    int h = sol.getHAPIndexTeam(i);
    int hc = sol.getComplementIndexHAP(h);
    // cout << "i = " << i << ", h = " << h << ", hc = " << hc << endl;
    int j_ = 0;
    while (sol.getHAPIndexTeam(sol.getGlobalIndexTeam(l,j_)) != hc){
        // cout << "hap of " << j << "  = " << sol.getHAPIndexTeam(j) << endl;
        ++j_;
        // Does this complemantray HAP always exists? No, of course not!!
        if (j_ >= sol.getNrTeamsLeague(l)){
            // cout << "no complement hap found for team " << i << endl;
            return false;
            /*
            cout << "Chosen team = " << i << endl;
            cout << "HAPs: " << endl;
            for (int t = 0; t < sol.getNrTeams(); ++t){
                printHAP(sol, t);
            }
            cout << "but complentary hap of " << i << ": ";
            vector<HA>HAP = sol.getHAP(hc);
            for (auto& ha: HAP){
                if (ha == HA::H){
                    cout << "H";
                }
                else if (ha == HA::A){
                    cout << "A";
                }
                else{
                    cout << "?";
                }
            }
            cout << endl;
            assert(j < sol.getNrTeams());
            */
        }
    }

    int j = sol.getGlobalIndexTeam(l,j_);
    if (sol.getNrLeagues()==1 && ((i != i_) || (j != j_))){
        cout << "(i != i_) || (j != j_) in ComplementInsertion()" << endl;
        std::abort();
    }
    team1 = i;
    team2 = j;
    hap_index1 = h; // hap_index1 and hap_index2 are needed to return to original haps when rejected by metropolis criterion
    hap_index2 = hc;
    // draw new haps for i and j that are complementary
    int hc_i = RandomIntegerNumber(0, sol.getNrHAPs()-1);
    int hc_j = sol.getComplementIndexHAP(hc_i);

    setHAP(sol, i, hc_i);
    setHAP(sol, j, hc_j);
    // cout << "cost = " << sol.ComputeCostCapacities() << endl;
    if (sol.ComputeCostCapacities() > 0){
        // cout << "reject ComplementInsertion" << endl;
        setHAP(sol, i, h);
        setHAP(sol, j, hc);
        assert(sol.ComputeCostCapacities() <= 0);
        return false;
    }
    return true;
}

void AssignsHAPsToTeamsBasedOnSol(Solution& sol){
    // Find the HAP of teams:
    int i,h,r,index;
    for (i = 0; i < sol.getNrTeams(); ++i){
        for (h = 0; h < sol.getNrHAPs(); ++h){
            for (r = 0; r < sol.getNrRounds(); ++r){
                if (sol.Orientation[i][r] != sol.getModeHAPRound(h,r)){
                    break;
                }
            }
            if (r == sol.getNrRounds()){
#ifdef PRINT
#if PRINT == 1
                cout << "Team " << i << " is assigned HAP " << h << endl;
#endif
#endif
                sol.setHAPIndexTeam(i, h);
                break;
            }
        }
        if (h == sol.getNrHAPs()){
            // When r > n/2, we start from Vizing, but it can be that, if we included only promising HAPs, this HAP is not found!!
#ifdef PRINT
#if PRINT == 1
            cout << "HAP of " << i << " not found when assign HAPs to teams based on sol" << endl;
#endif
#endif
            // This can be because we did not include all HAPs!!
            vector<HA>NewHAP = vector<HA>(sol.getNrRounds());
            vector<HA>NewHAP_c = vector<HA>(sol.getNrRounds());
            for (r = 0; r < sol.getNrRounds(); ++r){
                if (sol.Orientation[i][r] == HA::H){
#ifdef PRINT
#if PRINT == 1
                    cout << "H";
#endif
#endif
                    NewHAP[r] = HA::H;
                    NewHAP_c[r] = HA::A;
                }
                else{
#ifdef PRINT
#if PRINT == 1
                    cout << "A";
#endif
#endif
                    NewHAP[r] = HA::A;
                    NewHAP_c[r] = HA::H;
                }
            }
#ifdef PRINT
#if PRINT == 1
            cout << endl;
#endif
#endif

            if (sol.HAP_satisfies_all_requirements(NewHAP)){
                sol.AddHAPWithComplement(NewHAP, NewHAP_c);
#ifdef PRINT
#if PRINT == 1
                cout << "HAP added" << endl;
#endif
#endif
            }
            else{
#ifdef PRINT
#if PRINT == 1
                cout << "Infeasible HAP" << endl;
#endif
#endif
                std::abort();
            }
        }
    }
}

void GreedyMatching::solve(Input& in, Solution& sol){

    StartTime = std::chrono::high_resolution_clock::now();

    Opponents = vector<vector<int>>(sol.getNrTeams(), vector<int>(sol.getNrRounds(), -1));

    if (!InitialSolutionGiven){
        // Assign HAPs such that we respect v+
        cout << "Assign HAPs to teams.." << endl;
        if (in.getNrRounds() > in.getNrTeams()/2){
            GurSolver gursol(in);
            // gursol.AssignHAPsToTeams(sol);
            const bool relax_x = false, min_travel = false, min_capacity_violations = false;
            // gursol.BuildMiaoFormulation(relax_x, min_travel, min_capacity_violations);
            gursol.BuildPatternFormulation(); // just assign HAPs to teams without objective!!
            gursol.solve();
            gursol.StoreHAPs(sol);
            // Use Benders here to find feasible opponent schedule?
            ReAssignHAPs(sol);
        }
        else{
            // We know that this always produces an initial solution!!
            // If I do not do this, it can be that I never find anything, even if, for example, n=40 and r=20
            // So random complementary HAPs!!!
            int hc_i = RandomIntegerNumber(0, sol.getNrHAPs()-1);
            int hc_j = sol.getComplementIndexHAP(hc_i);
            for (int i = 0; i < in.getNrTeams()/2; ++i){
                setHAP(sol, i, hc_i);
            }
            for (int j = in.getNrTeams()/2; j < in.getNrTeams(); ++j){
                setHAP(sol, j, hc_j);
            }
            sol.NrColouredRounds = sol.getNrRounds();
            auto it = Moves.begin();
            CurrentMove = it->first;
        }
        cout << "patterns assigned" << endl;
        /*
        // This works if r <= n/2
        GurSolver gur_x(in);
        gur_x.build_all(false, false);
        for (int i = 0; i < gur_x.HapFixed.size(); ++i){
            gur_x.HapFixed[i] = true;
        }
        gur_x.FixHAP(sol);
        gur_x.solve();
        gur_x.SaveSolution(sol);
        AssignsHAPsToTeamsBasedOnSol(in, sol);
        UpdateBestSolution(sol);
        cout << "opponent schedule found" << endl;
        */
        // Full Formulation takes too long!!!
    }
    else{
        best_obj = sol.ComputeTotalCost();
        cout << "Cost HAPs = " << sol.ComputeTotalHACost() << endl;
        current_obj = best_obj;
        cout << "Assign Haps to teams" << endl;
        AssignsHAPsToTeamsBasedOnSol(sol);
        UpdateBestSolution(sol);
        SetAllOpponents(sol);
        cout << "Ready" << endl;
        sol.validate();
        ReAssignHAPs(sol);
        cout << "Travel cost = " << sol.ComputeTravelCostTTP() << endl;
    }

    // Check if the starting solution contains at leats one pair of complementary HAPs. If not: set this move to 0!!!

    bool PairFound = true;

    for (int i = 0; i < sol.getNrTeams(); ++i){
        int l = sol.getLeagueTeam(i);
        // cout << "l = " << l << endl;
        int h = sol.getHAPIndexTeam(i);
        int hc = sol.getComplementIndexHAP(h);
        // cout << "i = " << i << ", h = " << h << ", hc = " << hc << endl;
        int j_ = 0;
        while (sol.getHAPIndexTeam(sol.getGlobalIndexTeam(l,j_)) != hc){
            // cout << "hap of " << sol.getGlobalIndexTeam(l,j_)<< "  = " << sol.getHAPIndexTeam(sol.getGlobalIndexTeam(l,j_)) << endl;
            ++j_;
            // Does this complemantray HAP always exists? No, of course not!!
            if (j_ >= sol.getNrTeamsLeague(l)){
                // cout << "no complement hap found for team " << i << endl;
                PairFound = false;
                break;
            }
        }
        if (PairFound){
            cout << "Pair of complementary haps found!!!!" << endl;
            break;
        }
    }

    if (!PairFound){
        cout << "Not one pair of complementary HAPs!!" << endl;
        if (Moves.size() == 1 && Moves.begin()->first == Move::ComplementInsertion){
            STOP = true; // in this case, our algorithm is stuck!!
            cout << "But this is the only move, so we are stuck!!" << endl;
        }
        else{
            cout << "Only swaps, set size of this move to 0" << endl;
            for (auto it = WeightsCumul.begin(); it != WeightsCumul.end(); ++it) {
                if (it->second == Move::ComplementInsertion) {
                    WeightsCumul.erase(it);
                    break;   // erase only one
                }
            }
            map<Move, double>Weights;
            for (auto it = WeightsCumul.begin(); it != WeightsCumul.end(); ++it) {
                Weights[it->second] = 1.0 / (double)WeightsCumul.size();
            }

            double sum = 0.0;
            WeightsCumul.clear();
            for (auto& [move, weight]: Weights){
                sum += weight;
                WeightsCumul[sum] = move;
            }
            for (auto it = WeightsCumul.begin(); it != WeightsCumul.end(); ++it) {
                cout << "Weight = " << it->first << ", move = " << Moves.at(it->second) << endl;
            }
        }
    }

    Reset(sol);

    while(!STOP){
        // cout << "solve matchings with following haps" << endl;
        // for (int t = 0; t < sol.getNrTeams(); ++t){
        //    printHAP(sol, t);
        //}
        if (SchedulePhase(sol)){ // solve sequence of matching problems (round per round)
            if (!Update(sol, sol.ComputeTotalCost())){ // update best objective
                // if solution not accepted: reverse
                ReverseMove(sol);
                // Problem is that with multi leagues, if we do not accept the HAP move, in the next iteration we might choose a HAP operator from another league. Then the new opponent schedule is not compatible again with the reversed HAPs for the previous league. Hence we also have to go back to the old matching
            } 
            else{
                SetOpponentsCurrentLeague(sol);
                if (InitialOnly){
                    STOP = true;
                }
            }
        }
        else{
            Update(sol, INT_MAX); // infeasible solution but still update values
            ReverseMove(sol); // if schedule phase not succesful: reverse move, and try with new HAP move
        }
        ReAssignHAPs(sol); // do a HAP move
        assert(sol.ComputeTotalHACost() <= 0);
        Reset(sol); // this deletes all the matchups in the rounds of the league chosen by the HAP operator
    }

    // cout << "done" << endl;
    // save into solution
    if (NrSuccesfullMatchings >= 1 || InitialSolutionGiven){
        cout << "NrSuccesfullMatchings = " << NrSuccesfullMatchings << endl;
        SaveBestSolution(sol);
    }
}

void GreedyMatching::SolveGivenSeqeuence(Input& in, Solution& sol){
    cout << "Solve given sequence" << endl;
    string file_path = "Instances" + std::string(PATHSEP) + "Miao";

    if (in.IsCapacityConstant()){
        file_path += (std::string(PATHSEP) + "SolutionsFoundByMiao" + std::string(PATHSEP) + "ConstantCapacity");
    }
    else if (in.getMiaoHAPSetting() == 1){
        file_path += (std::string(PATHSEP) + "SolutionsFoundByMiao" + std::string(PATHSEP) + "VariableCapacity" + std::string(PATHSEP) + "Setting1");
    }
    else if (in.getMiaoHAPSetting() == 2){
        file_path += (std::string(PATHSEP) + "SolutionsFoundByMiao" + std::string(PATHSEP) + "VariableCapacity" + std::string(PATHSEP) + "Setting2");
    }

    if (in.getNrTeams() == 184){
        file_path += (std::string(PATHSEP) + "i02.txt");
    }
    else if (in.getNrTeams() == 216){
        file_path += (std::string(PATHSEP) + "i03.txt");
    }
    else if (in.getNrTeams() == 144){
        file_path += (std::string(PATHSEP) + "i04.txt");
    }
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file " << file_path;
        return;
    }

    std::string line;
    cout << "Read HAP assignment" << endl;
    // First read the HAP assignment
    int nr = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        if (nr < in.getNrTeams()){
            int i, h;
            ss >> i;
            ss >> h;
            
            if ((int)sol.getHAP(h).size() != sol.getNrRounds()){
                cout << "HAP " << h << " has size " << sol.getHAP(h).size() << " but there are " << sol.getNrRounds() << " rounds " << endl;
            }
            assert(sol.getHAP(h).size() == sol.getNrRounds());
            setHAP(sol, i, h);
            // cout << "Team " << i << " is assigned HAP " << h << endl;
        }
        else{
            int k = 0;
            int r;
            std::stringstream ss(line);
            while(ss >> r){
                Rounds[k++] = r-1; // rounds start from 1 in the files of Miao
            }
        }
        ++nr;
    }
    file.close();

    Reset(sol);
    SchedulePhase(sol);
}
