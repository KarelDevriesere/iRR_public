#include "MiaoAlgo.h"
#include "assert.h"

MiaoAlgo::MiaoAlgo(Input& in){
    Rounds = vector<int>(in.getNrRounds());
    best_obj = INT_MAX;
    for (int r = 0; r < in.getNrRounds(); ++r){
        Rounds[r] = r;
    }
    if (in.getHAP_requirement(HAP_requirement_name::BreakLimit) == 0){
        Weights = {{HAP_operator::InterClubSwap, 2.0/5.0}, {HAP_operator::IntraClubSwap, 2.0/5.0}, {HAP_operator::RandomSwap, 1.0/5.0}, {HAP_operator::ComplementInsertion, 0.0}};
    }
    else if (in.getSingleTeamClubs().size() < 3){
        Weights = {{HAP_operator::InterClubSwap, 0.0}, {HAP_operator::IntraClubSwap, 1.0/2.0}, {HAP_operator::RandomSwap, 1.0/4.0}, {HAP_operator::ComplementInsertion, 1.0/4.0}};
    }
    double sum = 0;
    for (const auto& op: HAP_operators){
        sum += Weights.at(op);
        WeightsCumul[op] = sum;
    }
    assert(0.99 < sum && sum < 1.01);
}

MiaoAlgo::~MiaoAlgo(){}

void MiaoAlgo::Update(Solution& sol){
    if (sol.ComputeTotalCost() < best_obj){
        best_obj = sol.ComputeTotalCost();
        cout << "best obj = " << best_obj << endl;
    }
}

void MiaoAlgo::Reset(Solution& sol){
    // cout << "Reset" << endl;
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
    std::random_device rd;                          
    std::mt19937 gen(rd());                         
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double rnd;
    bool MoveChosen = false;
    while(!MoveChosen){
        rnd = dis(gen);
        if (rnd < WeightsCumul[HAP_operator::InterClubSwap]){
            cout << "InterClubSwap" << endl;
            MoveChosen = InterClubSwap(sol);
        }
        else if (rnd < WeightsCumul[HAP_operator::IntraClubSwap]){
            cout << "IntraClubSwap" << endl;
            MoveChosen = IntraClubSwap(sol);
        }
        else if (rnd < WeightsCumul[HAP_operator::RandomSwap]){
            cout << "RandomSwap" << endl;
            MoveChosen = RandomSwap(sol);
        }
        else{
            cout << "ComplementInsertion" << endl;
            MoveChosen = ComplementInsertion(sol);
        }
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
            cout << "Optimize round " << s << ", try: " << count << endl;
            const int r = Rounds[s];

            // cout << "find matching" << endl;
            vector<pair<int,int>>matching = MoveMWPM(sol, l, r, bipartite, includeHAPs);
            // cout << "matching done" << endl;

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
                    SetValueCircleMethod(h, a, r, sol);
                }
                ++s;
            }
            cin.get();
        }
    }
    sol.validate();
    cout << "Total cost = " << sol.ComputeTotalCost() << endl;
    cin.get();
    assert(sol.ComputeTravelCost() == sol.ComputeTotalCost());
    return true;
}

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
    int gur_obj = gur.solve();
}

void MiaoAlgo::solve(Input& in, Solution& sol){
    // TODO: Miao is also doing something with byes...
    GurSolver gursol(in);
    gursol.AssignHAPsToTeams(sol);

    // calculate bound if we fix the HAPs
    // **** IP ***
    GurSolver gur(in);
    const bool relax_x = false;
    gur.build_all(true, relax_x);
    gur.setTimeLimit(6000);
    gur.setBoundCapacityViolations();
    for (int i = 0; i < sol.getNrTeams(); ++i){
		gur.HapFixed[i] = true;
	}
    gur.AddObj(true, false);
    // **** IP ****

    bool ip = true;

    do{
        if (ip){
            gur.FixHAP(sol);
            int gur_obj = gur.solve();
            gur.FreeVariables();
            if (gur_obj < best_obj){
                best_obj = gur_obj;
                cout << best_obj << endl;
            }
        }
        else{
            if (SchedulePhase(sol)){
                Update(sol);
            }
        }
        ReAssignHAPs(sol);
        Reset(sol);
    }
    while(true);
}