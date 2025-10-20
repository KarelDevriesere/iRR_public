#include "MiaoAlgo.h"
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

// Miao's HAP operators:

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

MiaoAlgo::MiaoAlgo(const std::unordered_map<HAP_operator, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<HAP_operator, double>& weights, Input& in, std::mt19937& g): SA<HAP_operator>(moves, weights, g){
    Rounds = vector<int>(in.getNrRounds());
    for (int r = 0; r < in.getNrRounds(); ++r){
        Rounds[r] = r;
    }
}

MiaoAlgo::~MiaoAlgo(){}

void MiaoAlgo::SaveBestSequenceMatches(Solution& sol){
    int m, j, h, a;
    for (int r = 0; r < sol.getNrRounds(); ++r){
        vector<bool>TeamSeen(sol.getNrTeams(), false);
        m = 0;
        for (int i = 0; i < sol.getNrTeams(); ++i){
            if (TeamSeen[i]){
                continue;
            }
            j = sol.TeamColorOpp[i][r];
            assert(sol.isEligible(i,j));
            if (sol.Orientation[i][r] == HA::H){
                assert(sol.Orientation[j][r] == HA::A);
                h = i, a = j;
            }
            else{
                assert(sol.Orientation[j][r] == HA::H);
                assert(sol.Orientation[i][r] == HA::A);
                h = j, a = i;
            }
            TeamSeen[h] = true;
            TeamSeen[a] = true;
            BestSequenceMatches[r][m++] = {h,a};
        }
    }
}

void MiaoAlgo::ReverseMove(Solution& sol){
    if (CurrentMove == HAP_operator::ComplementInsertion){
        setHAP(sol, team1, hap_index1);
        setHAP(sol, team2, hap_index2);
    }
    else{
        SwapHAPs(sol, team1, team2); // see operators
    }
}

void MiaoAlgo::Reset(Solution& sol){
    // cout << "Reset" << endl;
    // Such that we can do the matchings again without conflicts
    // But: do not reset the orientations!!
    int j;
    for (int i = 0; i < sol.getNrTeams(); ++i){
        for (int r = 0; r < sol.getNrRounds(); ++r){
            j = sol.TeamColorOpp[i][r];
            sol.TeamColorOpp[i][r] = -1;
            if (j != -1){
                sol.MatchColor[i][j] = -1;
                sol.MatchColor[j][i] = -1;
            }
        }
    }
}

void MiaoAlgo::ReAssignHAPs(Solution& sol){
    // cout << "ReAssign HAPs" << endl;
    double rnd;
    bool MoveChosen = false;
    while(!MoveChosen){
        rnd = RandomDoubleNumber(0.0, 1.0);
        auto iterator = WeightsCumul.upper_bound(rnd); 
        CurrentMove = iterator->second;
        if (CurrentMove == HAP_operator::InterClubSwap){
            // cout << "InterClubSwap" << endl;
            MoveChosen = InterClubSwap(sol);
        }
        else if (CurrentMove == HAP_operator::IntraClubSwap){
            // cout << "IntraClubSwap" << endl;
            MoveChosen = IntraClubSwap(sol);
        }
        else if (CurrentMove == HAP_operator::RandomSwap){
            // cout << "RandomSwap" << endl;
            MoveChosen = RandomSwap(sol);
        }
        else{
            // cout << "ComplementInsertion" << endl;
            MoveChosen = ComplementInsertion(sol);
        }
        NrChosen.at(CurrentMove)++;
    }
    // cout << "Move chosen" << endl;
}

bool MiaoAlgo::SchedulePhase(Solution& sol){
    const int N = sol.getNrTeams();
    const bool bipartite = true;
    const bool includeHAPs = true;
    int h, a;
    for (int l = 0; l < sol.getNrLeagues(); ++l){
        int s = 0, count = 0;
        while (s < sol.getNrRounds()){

            const int r = Rounds[s];
            cout << "Optimize round " << r << ", try: " << count << endl;

            // cout << "find matching" << endl;
            int delta = 0;
            const bool CM = false;
            const bool keepHAP = true;
            const bool MinCostM = true;
            pair<vector<pair<int,int>>, vector<int>>Matching_OpponentMatching = MoveMWPM(sol, l, r, bipartite, keepHAP, CM, gen, MinCostM); // in the file operators
            vector<pair<int,int>>matching = Matching_OpponentMatching.first;
            if (matching.size() < N/2){
                cout << "matching failed, shuffle rounds" << endl;
                // cin.get();
                shuffle(Rounds.begin(), Rounds.end(), default_random_engine(42));
                Reset(sol);
                s = 0;
                if (count++ <= 100){
                    return false;
                }
            }
            else{
                cout << "Matching in round " << r << ":" << endl;
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
                    cout << h << ", " << a << endl;
                    SetValueCircleMethod(h, a, r, sol);
                }
                ++s;
            }
        }
    }
    // assert(sol.validate());
    cout << "Total travel cost = " << sol.ComputeTravelCost() << endl;
    cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    assert(sol.ComputeTravelCost() == sol.ComputeTotalCost());
    cin.get();
    return true;
}

bool MiaoAlgo::InterClubSwap(Solution& sol){
    // Must they be of the same league->yes?
    // So modify code!!!
    int c1_ = rand()%sol.getSingleTeamClubs().size();
    int c1 = sol.getSingleTeamClubs()[c1_];
    int i_ = rand()%sol.getTeamsClub(c1).size();
    int c2_ = ((c1_+1)+(rand()%(sol.getSingleTeamClubs().size()-1)))%sol.getSingleTeamClubs().size();
    int c2 = sol.getSingleTeamClubs()[c2_];
    int j_ = rand()%sol.getTeamsClub(c2).size();

    int i = sol.getTeamsClub(c1)[i_];
    int j = sol.getTeamsClub(c2)[j_];
    team1 = i;
    team2 = j;
    // cout << "InterClubSwap: swap HAPs of teams " << i << " and " << j << " of clubs " << c1 << " and " << c2 << endl;
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

bool MiaoAlgo::IntraClubSwap(Solution& sol){
    assert(sol.ComputeCostCapacities() <= 0);
    int c_ = rand()%sol.getMultiTeamClubs().size();
    int c = sol.getMultiTeamClubs()[c_];
    assert(sol.getTeamsClub(c).size() > 1);
    int i_ = rand()%sol.getTeamsClub(c).size();
    int j_ = ((i_+1)+(rand()%(sol.getTeamsClub(c).size()-1)))%sol.getTeamsClub(c).size();

    int i = sol.getTeamsClub(c)[i_];
    int j = sol.getTeamsClub(c)[j_];
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

bool MiaoAlgo::RandomSwap(Solution& sol){
    int c1 = rand()%sol.getNrClubs();
    assert(sol.getTeamsClub(c1).size() > 0);
    int i_ = rand()%sol.getTeamsClub(c1).size();
    int c2 = ((c1+1)+(rand()%(sol.getNrClubs()-1)))%sol.getNrClubs();
    int j_ = rand()%sol.getTeamsClub(c2).size();
    assert(sol.getTeamsClub(c2).size() > 0);

    int i = sol.getTeamsClub(c1)[i_];
    int j = sol.getTeamsClub(c2)[j_];
    team1 = i;
    team2 = j;
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

bool MiaoAlgo::ComplementInsertion(Solution& sol){
    // TODO
    // Also fill TeamsHAP!!
    // Given two teams with complementary HAPs, replace their patterns with a newly chosen pair of 
    // complementary HAPs from H. 
    int i = rand()%sol.getNrTeams();
    int h = sol.getHAPIndexTeam(i);
    int hc = sol.getComplementIndexHAP(h);
    // cout << "h = " << h << ", hc = " << hc << endl;
    int j = 0;
    while (sol.getHAPIndexTeam(j) != hc){
        // cout << "hap of " << j << "  = " << sol.getHAPIndexTeam(j) << endl;
        ++j;
        // Does this complemantray HAP always exists? No, of course not!!
        if (j >= sol.getNrTeams()){
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
    team1 = i;
    team2 = j;
    hap_index1 = h; // hap_index1 and hap_index2 are needed to return to original haps when rejected by metropolis criterion
    hap_index2 = hc;
    // draw new haps for i and j that are complementary
    int hc_i = rand()%sol.getNrHAPs();
    int hc_j = sol.getComplementIndexHAP(hc_i);
    // cout << "ComplementInsertaion: swap HAPs of " << i << " and " << j << " for " << hc_i << " and " << hc_j << endl;

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

void MiaoAlgo::solve(Input& in, Solution& sol){

    StartTime = std::chrono::high_resolution_clock::now();
    // TODO: Miao is also doing something with byes...

    if (!InitialSolutionGiven){
        GurSolver gursol(in);
        // Assign HAPs such that we respect v+
        gursol.AssignHAPsToTeams(sol);
        CurrentMove = HAP_operator::Initial;
    }
    else{
        best_obj = sol.ComputeTotalCost();
        current_obj = best_obj;
        UpdateBestSolution(sol);
        ReAssignHAPs(sol); // do a HAP move
        Reset(sol);
    }

    // Initial solution is infeasible for tiny!!!

    do{
        // cout << "solve matchings with following haps" << endl;
        // for (int t = 0; t < sol.getNrTeams(); ++t){
        //    printHAP(sol, t);
        //}
        if (SchedulePhase(sol)){ // solve sequence of matching problems (round per round)
            if (!Update(sol, sol.ComputeTotalCost())){ // update best objective
                // if solution not accepted: reverse
                ReverseMove(sol);
            } 
            else{
                if (InitialOnly){
                    STOP = true;
                }
            }
        }
        // cout << "reassign haps" << endl;
        ReAssignHAPs(sol); // do a HAP move
        Reset(sol); // this deletes all the matchups in the rounds
    }
    while(!STOP);

    // cout << "Miao Algo done" << endl;
    // save into solution

    SaveBestSolution(sol);
}

void MiaoAlgo::SolveGivenSeqeuence(Input& in, Solution& sol){
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
            
            if (sol.getHAP(h).size() != sol.getNrRounds()){
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
