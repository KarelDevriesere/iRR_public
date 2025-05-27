#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <assert.h>
#include <chrono>

#include "algo.h"
#include "Input.h"
#include "GurSolver.h"
// #include "Graph.h"
#include "Operators.h"
#include "Solution.h"
#include "ILS.h"
#include "MiaoAlgo.h"

enum class Algorithm{IP_Karel, IP_Miao, RF_Miao, Meta_Miao, FO_Karel, Meta_Karel};
unordered_map<Algorithm, string>AlgoString = {{Algorithm::IP_Karel, "IP_Karel"}, {Algorithm::IP_Miao, "IP_Miao"}, {Algorithm::RF_Miao, "RF_Miao"}, {Algorithm::Meta_Miao, "Meta_Miao"},
    {Algorithm::FO_Karel, "FO_Karel"}, {Algorithm::Meta_Karel, "Meta_Karel"}};

void MakeInputFileMatchingFormulation(Input& in){
    ofstream example;
    example.open("example22.txt");
    int N = in.getNrTeams();
    example << N << "\n";
    for (int r = 0; r < in.getNrRounds(); ++r){
        for (int i = 0; i < N; ++i){
            for (int j = 0; j < N; ++j){
                example << i << " " << j << " " << r << " " << (double)in.getDistanceTeams(i,j) << "\n";
            }
        }
    }
    example.close();
}

void SetAlgo(unordered_map<Algorithm,bool>& AlgoSelected, const int A){
    if (A == 0){
        AlgoSelected.at(Algorithm::IP_Karel) = true;
    }
    else if (A == 1){
        AlgoSelected.at(Algorithm::IP_Miao) = true;
    }
    else if (A == 2){
        AlgoSelected.at(Algorithm::RF_Miao) = true;
    }
    else if (A == 3){
        AlgoSelected.at(Algorithm::Meta_Miao) = true;
    }
    else if (A == 4){
        AlgoSelected.at(Algorithm::FO_Karel) = true;
    }
    else{
        AlgoSelected.at(Algorithm::Meta_Karel) = true;
    }
}

void RF_Miao(Input& in, Solution& sol){

	// This is not part of GurSolver since we cannot change the variable type once this is defined in Gurobi
	// Instead, we should create a new model, so it's best to define this function outside GurSolver and create seperate gur objects

    GurSolver gur_relax(in);

	// relax x_ijr variables:
	bool relax_x = true;
	gur_relax.BuildMiaoFormulation(relax_x);

	const bool min_travel = true;
	const bool min_capacity_violations = false;
	gur_relax.AddObj(min_travel, min_capacity_violations);

    cout << "solve model" << endl;
	int LB = gur_relax.solve();
    cout << "Lower bound = " << LB << endl;

    gur_relax.StoreHAPs(sol);

    GurSolver gur_fix(in);
    relax_x = false;
    gur_fix.BuildMiaoFormulation(relax_x);
    gur_fix.Fix_y_Patterns(sol);
    gur_fix.AddObj(min_travel, min_capacity_violations);

    int obj = gur_fix.solve();
    gur_fix.SaveSolution(sol);
}

void FindSolutionIP(GurSolver& gur, Solution& sol, const bool HA, const bool min_travel, const bool min_cap){
    const bool relax_x = false;
    gur.build_all(HA, relax_x);
    gur.setBoundCapacityViolations();
    gur.AddObj(min_travel, min_cap);
    int gur_obj = gur.solve();

    if (gur_obj != -1){
        gur.Store_x_value();
    }

    gur.SaveSolution(sol);
    sol.validate();
    assert(sol.ComputeTotalHACost() == 0);
}

int main(int argc, char** argv){

    // std::string file_path_base = "C:\\Users\\kardvrie\\C++\\VSprojects\\iRR\\Instances";
    std::string file_path_base = "Instances";

    int count = 0;
    int A;
    int yn;
    bool stop = false;

    unordered_map<Algorithm, bool>AlgoSelected = {{Algorithm::IP_Karel, false}, {Algorithm::IP_Miao, false}, {Algorithm::RF_Miao, false},
         {Algorithm::Meta_Miao, false}, {Algorithm::FO_Karel, false}, {Algorithm::Meta_Karel, false}};

    if (argc < 2){
        cout << "Which algorithm?" << endl;
        cout << "---------------------------" << endl;
        cout << "0: full IP Karel" << endl;
        cout << "1: full IP Miao" << endl;
        cout << "2: Relax and Fix Miao" << endl;
        cout << "3: Metaheuristic Miao" << endl;
        cout << "4: Fix-and-Optimize Karel" << endl;
        cout << "5: Metaheuristic Karel" << endl;
        cout << "---------------------------" << endl;
        do{
            cin >> A;
            if (A != 0 && A != 1 && A != 2 && A != 3 && A != 4 && A != 5){
                count++;
                if (count > 3){
                    cout << "Abort, please specify an available algorithm next time" << endl;
                }
                cout << "Please enter an available algorithm (0-5)" << endl;
            }
            else{
                SetAlgo(AlgoSelected, A);
                cout << "Run also another algorithm? (No: 0, Yes: 1)" << endl;
                cin >> yn;
                if (!yn){
                    stop = true;
                }
                else{
                    cout << "New algorithm: " << endl;
                }
            }
        }
        while (!stop);
    }
    else{
        A = std::stoi(argv[1]);
        SetAlgo(AlgoSelected, A);
    }

    unordered_map<Algorithm, int>Objectives = {{Algorithm::IP_Karel, 0}, {Algorithm::IP_Miao, 0}, {Algorithm::RF_Miao, 0},
         {Algorithm::Meta_Miao, 0}, {Algorithm::FO_Karel, 0}, {Algorithm::Meta_Karel, 0}};

    int Inst;
    if (argc < 2){
        cout << "Instances Miao (0) or Hockey (1)?" << endl;
        cin >> Inst;
    }
    else{
        Inst = std::stoi(argv[2]);
    }
    try{
        if (Inst != 0 && Inst != 1){
            throw (Inst);
        }
        else{
            if (Inst == 0){
                file_path_base += "\\Miao";
            }
            else{
                file_path_base += "\\Hockey";
            }
        }
    }
    catch (char num){
        cout << "Input should be 0 (Miao) or 1 (Hockey)!" << endl;
    }

    int ex;
    stop = false;
    if (Inst == 0){
        if (argc < 2){
            cout << "Which instance?" << endl;
            cout << "1: S" << endl;
            cout << "2: U13" << endl;
            cout << "3: U15" << endl;
            cout << "4: U17" << endl;
            cout << "5: U21" << endl;
            cout << "6: M" << endl;
            do{
                cin >> ex;
                if (++count > 3){
                    cout << "Please specify available instance next time" << endl;
                    return 0;
                }
                else if (ex != 1 && ex != 2 && ex != 3 && ex != 4 && ex != 5 && ex != 6){
                    cout << "Please enter an available instance (1-6)" << endl;
                }
                else{
                    stop = true;
                }
            }
            while(!stop);
        }
        else{
            ex = std::stoi(argv[3]);
        }
    }
    else{
        if (argc < 2){
            cout << "Which instance?" << endl;
            cout << "1: U8_indoor_23_24" << endl;
            cout << "2: U8_indoor_24_25" << endl;
            do{
                cin >> ex;
                if (++count > 3){
                    cout << "Please specify available instance next time" << endl;
                    return 0;
                }
                else if (ex != 1 && ex != 2){
                    cout << "Please enter an available instance (1-6)" << endl;
                }
                else{
                    stop = true;
                }
            }
            while(!stop);
        }
        else{
            ex = std::stoi(argv[3]);
        }
    }
    count = 0;
    stop = false;

    vector<int>Instances;
    Instances = {ex};

    int NrBreaksPerTeam = 100; // unlimited for hockey
    int Cap;

    if (Inst == 0){
        if (argc < 2){
            cout << "Max nr of Breaks per team? (0, 1, 2 or 3)" << endl;
            do{
                cin >> NrBreaksPerTeam;
                if (++count > 3){
                    cout << "Please specify available Nr of breaks next time" << endl;
                    return 0;
                }
                else if (NrBreaksPerTeam != 0 && NrBreaksPerTeam != 1 && NrBreaksPerTeam != 2 && NrBreaksPerTeam != 3){
                    cout << "Nr of breaks should be between 0 and 3" << endl;
                }
                else{
                    stop = true;
                }
            }
            while(!stop);
        }
        else{
            NrBreaksPerTeam = std::stoi(argv[4]);
        }
        count = 0;
        stop = false;

        if (argc < 2){
            cout << "Constant (0) or Variable (1) capacity?" << endl;
            do{
                cin >> Cap;
                if (++count > 3){
                    cout << "Please specify Constant or Variable next time" << endl;
                    return 0;
                }
                else if (Cap != 0 && Cap != 1){
                    cout << "Please enter 0 or 1" << endl;
                }
                else{
                    stop = true;
                }
            }
            while(!stop);
        }
        else{
            Cap = std::stoi(argv[5]);
        }
        count = 0;
        stop = false;

        if (Cap == 0){
            file_path_base += "\\ConstantCapacity";
        }
        else{
            file_path_base += "\\VariableCapacity";
        }
        
        if (Cap == 1){
            int Setting;

            if (argc < 2){
                do{
                    cout << "Setting? (1 or 2) " << endl;
                    cin >> Setting;
                    if (++count > 3){
                        cout << "Please specify Setting 1 or 2 next time" << endl;
                        return 0;
                    }
                    else if (Setting != 1 && Setting != 2){
                        cout << "Please enter 1 or 2" << endl;
                    }
                    else{
                        stop = true;
                    }
                }
                while(!stop);
            }
            else{
                Setting = std::stoi(argv[6]);
            }

            if (Setting == 1){
                file_path_base += "\\Setting1\\NoSurplus";
            }
            else{
                file_path_base += "\\Setting2\\NoSurplus";
            }
        }
    }

    int TIME_LIMIT = 2*3600;
            

    /*
    try{
        double Surplus = std::stod(argv[3]);
        if (Surplus != 0 && Surplus != 0.1 && Surplus != 0.2 && Surplus != 0.3){
            throw (Surplus);
        }
    }
    catch (double num){
        cout << "In case of variable capacity, the third argument is surplus, which is 0, 0.1, 0.2 or 0.3" << endl;
    }
    */

    // TODO: add that a team can see at most MAX nr of teams from same club -> done
    // TODO: allow 2RR
        // PRS -> done (nothing changes)
        // BalancedCycle -> done (nothing changes)
        // Bipartite matching -> done
        // Normal matching -> done
        // TS -> done, also stays the same
        // PTS
    // TODO: calculate minimum capacity violations for each instance and store them in a file
    // Check whether these values are the same as Miao!!!
    // TODO: test code for multiple leagues

    std::cout << "Start" << std::endl;

    // Just provide complete path --> needed for comp David

    ofstream output;
    output.open("Results_ils_multi1");
    output << "Instance N R OPT Time 0.05 0.02 0.01 0.005 Best Time \n";

    // std::string file_path_base = "C:\\Users\\kardvrie\\C++\\VSprojects\\test2\\Instances\\ConstantCapacity\\Multi\\i";
    // std::string file_path_base = "C:\\Users\\kardvrie\\C++\\VSprojects\\test2\\Instances\\VariableCapacity\\Setting2\\b" + to_string(NrBreaksPerTeam) + "\\i";
    std::string file_path;

    for (auto& inst: Instances){

        if (inst < 10){
            file_path = file_path_base + "\\i0" + to_string(inst);
        }
        else{
            file_path = file_path_base + "\\i" + to_string(inst);
        }
        file_path += ".txt";
        // cout << file_path << endl;
        
        // std::string file_path = (string) argv[1];
        Input in;
        if (!in.read(file_path)){
            cout << "Could not read " << file_path << endl;
            return 0;
        }
        in.setHAP_requirements(true, true, true, true, NrBreaksPerTeam);

        if (Inst != 1 && !in.read_HAPs()){ // do not use HAPs for hockey for now
            cout << "Problem with reading HAP files" << endl;
            return 0;
        }

        int N = in.getNrTeams();
        output << inst << " " << N << " " << in.getNrRounds();

        if (AlgoSelected.at(Algorithm::IP_Miao)){
            GurSolver gur_miao(in);
            Solution sol(in);
            bool relax_x = false;
            gur_miao.BuildMiaoFormulation(relax_x);
            gur_miao.AddObj(true, false);
            gur_miao.solve();
            gur_miao.SaveSolution(sol);
            sol.validate();
            Objectives.at(Algorithm::IP_Miao) = sol.ComputeTotalCost();
        }
        if (AlgoSelected.at(Algorithm::Meta_Miao)){
            Solution sol(in);
            MiaoAlgo miao_algo(in);
            miao_algo.solve(in, sol);
            sol.validate();
            Objectives.at(Algorithm::Meta_Miao) = sol.ComputeTotalCost();
        }
        if (AlgoSelected.at(Algorithm::RF_Miao)){
            Solution sol(in);
            RF_Miao(in, sol);
            sol.validate();
            Objectives.at(Algorithm::RF_Miao) = sol.ComputeTotalCost();
        }

        // MakeInputFileMatchingFormulation(in);

        // GreedyPerfectMatchings();

        /*
        Problems with cyclic structure: if R = N/2, then bipartite matching can only go back to
        original matching!!
        if I try it with R < N/2, then it finds improvements, but only bipartite matching quickly gets stuck
        The algorithm quickly gets stuck and fails to find good solutions, it's almost double that of gurobi in the end!!
        => How do PTS and TS react on the 2 alternating HAP's?
        In TS, the 2 teams change their HAPs, so we still end up with those 2 haps
        In PTS, they swap part of their HAP, so now the swapped teams may have a break
        */

        /*
        bool cyclic_succes = false;

        Graph& G = sol.getLeague(0);
        if (HA && sol.getNrLeagues() == 1){ // TODO: construction for multiple leagues? 
            // TODO: do cyclic only to use the 2 complementary break-free HAPs
            // cyclic_succes = CyclicConstruction(G, sol);
        }
        if (!HA || !cyclic_succes){
            for (int l = 0; l < sol.getNrLeagues(); ++l){
                G = sol.getLeague(l);
                int obj_greedy = GreedyConstruction(G, sol);
                if (obj_greedy < 0){
                    VizingConstruction(G, sol);
                    // setAllWeightsBF(G, sol);
                }
            }
        }
        */

        // cout << "Compute total cost" << endl;
        // int HA_cost = sol.ComputeTotalHACost();
        // cout << "total cost = " << HA_cost << endl;
        // cin.get();

        // sol.resetNrH_temp();

        // sol.validate();
        // cin.get();

        const bool HA = true;
        bool min_travel = false, min_cap = false;

        if (AlgoSelected.at(Algorithm::IP_Karel)){
            GurSolver gur(in);
            Solution sol(in);
            min_travel = true;
            FindSolutionIP(gur, sol, HA, min_travel, min_cap);
            gur.SaveSolution(sol);
            sol.validate();
            Objectives.at(Algorithm::IP_Karel) = sol.ComputeTotalCost();
        }

        auto start = std::chrono::high_resolution_clock::now();


        // Save initial solutions in file in map?

        if (AlgoSelected.at(Algorithm::FO_Karel)){
            GurSolver gur(in);
            Solution sol(in);
            min_travel = false;
            FindSolutionIP(gur, sol, HA, min_travel, min_cap);
            gur.AddObj(true, false);
            gur.FO(sol);
            gur.SaveSolution(sol);
            sol.validate();
            Objectives.at(Algorithm::FO_Karel) = sol.ComputeTotalCost();
        } 

        /*
        bool min_travel_gurobi = true;
        bool min_cap_viol = false;
        gur.AddObj(min_travel_gurobi, min_cap_viol);

        cout << "FO" << endl;
        gur.FO(sol);
        cin.get();

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration_gur = duration_cast<std::chrono::seconds>(stop - start);
        std::cout << "Optimal solution of GUROBI = " << gur_obj << " after " << duration_gur.count() << " seconds" << std::endl;
        if (gur.getMipGap() > 0.01){
            output << " TL, gap = " << gur.getMipGap() << " ";
            int bound = gur.getBestBound();
            ils.setOptimalObj(bound);
            cout << "bound = " << bound << endl;
        }
        else{
            ils.setOptimalObj(gur_obj);
        }
        output << " " << gur_obj << " " << duration_gur.count();
        */

        /*
        ILS: PTS is een heel grote toevoeging op TS+M, lijkt onmisbaar te zijn!!
        */

        if (AlgoSelected.at(Algorithm::Meta_Karel)){

            GurSolver gur(in);
            Solution sol(in);
            FindSolutionIP(gur, sol, HA, min_travel, min_cap);
            // RF_Miao(in, sol); -> does not find improvements..

            ILS ils(TIME_LIMIT);
            ils.setParamatersSA(0.01, 0.001, 10000, 0.75, 0.9); // low T -> pure local search
            ils.solve(sol);
            sol.validate();
            Objectives.at(Algorithm::Meta_Karel) = sol.ComputeTotalCost();

            // std::cout << "Solution of GUROBI = " << gur_obj << ", found after " << duration_gur.count() << " seconds" << std::endl;
            for (int g = 0; g < ils.getGaps().size(); ++g){
                cout << "gap of " << ils.getGaps()[g] << " found after " << ils.getTimeGap(g) << " seconds" << endl;
                output << " " << ils.getTimeGap(g);
            }
            cout << " best obj = " << ils.getBestObj() << ", time = " << ils.getTimeBestObj() << endl;
            output << " " << ils.getBestObj() << " " << ils.getTimeBestObj();

            ils.OutputNB(output);

            output << "\n";
        }

        cout << "******************************" << endl;
        cout << "Solutions found by algorithms: " << endl;
        cout << "------------------------------" << endl;
        for (auto&[algo, obj]: Objectives){
            if (AlgoSelected.at(algo)){
                cout << AlgoString.at(algo) << ": " << obj << endl;
            }
        } 
        cout << "******************************" << endl;
    }

    return 1;
}
