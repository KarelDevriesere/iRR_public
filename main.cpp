#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <numeric>
#include <algorithm>
#include <random>
#include <assert.h>
#include <chrono>
#include <sstream>

#include "Algo.h"
#include "Input.h"
#include "GurSolver.h"
// #include "Graph.h"
#include "Operators.h"
#include "Solution.h"
#include "ILS.h"
#include "MiaoAlgo.h"
#include "FO.h"
#include "Analyze.h"

enum class Algorithm{IP_Karel, IP_Miao, RF_Miao, Meta_Miao, FO_Karel, Meta_Karel};
const unordered_map<Algorithm, string>AlgoString = {{Algorithm::IP_Karel, "IP_Karel"}, {Algorithm::IP_Miao, "IP_Miao"}, {Algorithm::RF_Miao, "RF_Miao"}, {Algorithm::Meta_Miao, "Meta_Miao"},
    {Algorithm::FO_Karel, "FO_Karel"}, {Algorithm::Meta_Karel, "Meta_Karel"}};

const std::unordered_map<HAP_operator, string>Miao_operators = {{HAP_operator::InterClubSwap, "InterClubSwap"}, {HAP_operator::IntraClubSwap, "IntraClubSwap"}, {HAP_operator::RandomSwap, "RandomSwap"}, {HAP_operator::ComplementInsertion, "ComplementInsertion"}, {HAP_operator::Initial, "Initial"}};
const std::unordered_map<HAP_operator, double>Miao_weights = {{HAP_operator::InterClubSwap, 1.0/4.0}, {HAP_operator::IntraClubSwap, 1.0/4.0}, {HAP_operator::RandomSwap, 1.0/4.0}, {HAP_operator::ComplementInsertion, 1.0/4.0}, {HAP_operator::Initial, 0.0}};
const std::unordered_map<HAP_operator, double>Miao_weights_zero_breaks = {{HAP_operator::InterClubSwap, 1.0/2.5}, {HAP_operator::IntraClubSwap, 1.0/2.5}, {HAP_operator::RandomSwap, 1.0/5.0}, {HAP_operator::ComplementInsertion, 0.0}, {HAP_operator::Initial, 0.0}};
const std::unordered_map<HAP_operator, double>Miao_weights_Merged = {{HAP_operator::InterClubSwap, 1.0/2.5}, {HAP_operator::IntraClubSwap, 1.0/2.5}, {HAP_operator::RandomSwap, 0.0}, {HAP_operator::ComplementInsertion, 1.0/5.0}, {HAP_operator::Initial, 0.0}};

const unordered_map<move_name, string>Karel_operators = {{move_name::TS, "TS"}, {move_name::PTS, "PTS"}, {move_name::PRS, "PRS"}, {move_name::M, "M"}, {move_name::BM, "BM"}, {move_name::Initial, "Initial"}}; // {move_name::C, "C"}
// Cycle gives problems!!
const unordered_map<move_name, double>Karel_weights = {{move_name::TS, 1.0/5.0}, {move_name::PTS, 1.0/5.0}, {move_name::PRS, 1.0/5.0}, {move_name::M, 1.0/5.0}, {move_name::BM, 1.0/5.0}, {move_name::Initial, 0.0}};

const unordered_map<FO_move, string>FO_operators = {{FO_move::R1, "R1"}, {FO_move::R2C, "R2C"}, {FO_move::R3C, "R3C"}, {FO_move::R2NC, "R2NC"}, {FO_move::R3NC, "R3NC"}, {FO_move::T, "T"}};
const unordered_map<FO_move, double>FO_weights = {{FO_move::R1, 1.0/6.0}, {FO_move::R2C, 1.0/6.0}, {FO_move::R3C, 1.0/6.0}, {FO_move::R2NC, 1.0/6.0}, {FO_move::R3NC, 1.0/6.0}, {FO_move::T, 1.0/6.0}};

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

void ReadSolution(const string path, Solution& sol){

    // cout << "read the file " << path << endl;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file " << path;
        return;
    }

    std::string line;

    // Skip header line
    std::getline(file, line);

    // Read the rest
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int i, j, r, value;
        char comma; // to consume commas

        if (ss >> i >> comma >> j >> comma >> r >> comma >> value) {
            if (value > 0.1){
                SetValueCircleMethod(i,j,r,sol);
                sol.Orientation[i][r] = HA::H;
                sol.Orientation[j][r] = HA::A;
            }
        }
    }
    file.close();
    sol.validate();
}

void SaveSolution(const string path, Solution& sol){
    string file_name = path;
    std::ofstream file(file_name);
    file << "i,j,r,value\n";
    for (int i = 0; i < sol.getNrTeams(); ++i){
        for (int j = 0; j < sol.getNrTeams(); ++j){
            for (int r = 0; r < sol.getNrRounds(); ++r){
                file << i << "," << j << "," << r << ",";
                if (sol.MatchColor[i][j] == r){
                    file << "1";
                }
                else{
                    file << "0";
                }
                file << "\n";
            }
        }
    }
    file.close();
}

void FindSolutionIP(GurSolver& gur, Solution& sol, const bool HA, const bool min_travel, const bool min_cap){
    const bool relax_x = false;
    gur.build_all(HA, relax_x);
    gur.setBoundCapacityViolations();
    gur.AddObj(min_travel, min_cap);
    gur.WarmStart(sol);
    gur.solve();
    gur.SaveSolution(sol);
    assert(sol.ComputeTotalHACost() == 0);
}

void FindInitialSolution(Input& in, Solution& sol, const int seed){
    bool miao = true;
    if (miao){
        MiaoAlgo miao_algo(Miao_operators, Miao_weights, in, seed);
        miao_algo.InitialOnly = true;
        miao_algo.solve(in, sol);
    }
    else{
        const bool min_travel = false, min_cap = false, HA = true;
        GurSolver gur(in);
        FindSolutionIP(gur, sol, HA, min_travel, min_cap);
    }
    sol.validate();
}

int RF_Miao(Input& in, Solution& sol, int& lb){

	// This is not part of GurSolver since we cannot change the variable type once this is defined in Gurobi
	// Instead, we should create a new model, so it's best to define this function outside GurSolver and create seperate gur objects

    const int TimeLimitRF = 20;
    int dur = 0;
    GurSolver gur_relax(in);

	// relax x_ijr variables:
	bool relax_x = true;
	gur_relax.BuildMiaoFormulation(relax_x);
    gur_relax.WarmStart(sol); 

	const bool min_travel = true;
	const bool min_capacity_violations = false;
	gur_relax.AddObj(min_travel, min_capacity_violations);
    gur_relax.setTimeLimit(TimeLimitRF);
    gur_relax.TrackTimeBestSolution = true;

    // cout << "solve model" << endl;
	lb = gur_relax.solve();
    if (lb < 0){ // no solution found during assigning HAPs
        return -1;
    }
    dur += gur_relax.getTimeTillBestSolution();
    // cout << "Lower bound = " << lb << endl;

    // other alternative: just find a feasible hap assignment
    // gur_relax.AssignHAPsToTeams(sol); -> but then infeasible in next stage!

    gur_relax.StoreHAPs(sol);

    GurSolver gur_fix(in);
    relax_x = false;
    gur_fix.BuildMiaoFormulation(relax_x);
    gur_fix.Fix_y_Patterns(sol);
    gur_fix.AddObj(min_travel, min_capacity_violations); // turn back
    gur_fix.setTimeLimit(TimeLimitRF);
    gur_fix.TrackTimeBestSolution = true;

    gur_fix.solve();
    gur_fix.SaveSolution(sol);
    dur += gur_fix.getTimeTillBestSolution();
    // cout << "solution found!!" << endl;

    return dur;
}

int main(int argc, char** argv){

    std::string file_path_base = "Instances";
    std::string file_path_initial_solution_base = "InitialSolutions";
    std::string file_path_results_base = "Results";

    int count = 0;
    int A;
    int yn;
    bool stop = false;
    int seed = 42;
    const array<int,5>MetaSeeds = {5, 42, 139, 526, 1008};
    // const array<int,1>MetaSeeds = {5};

    unordered_map<Algorithm, bool>AlgoSelected = {{Algorithm::IP_Karel, false}, {Algorithm::IP_Miao, false}, {Algorithm::RF_Miao, false},
         {Algorithm::Meta_Miao, false}, {Algorithm::FO_Karel, false}, {Algorithm::Meta_Karel, false}};

    cout << "Why is Matching taking so much time?" << endl;
    cout << "Segmentation fault on HPC?" << endl;
    int answer = 0;
    if (argc < 2){
        cout << "Run expirements (0), analyze results (1) or show arguments (2)?" << endl;
        cin >> answer;
        if (answer == 1){
            const int InstanceSet = 0;
            const bool ConstantCpacity = false;
            for (int Setting = 1; Setting < 3; Setting++){
                AnalyzeResults(InstanceSet, ConstantCpacity, Setting);
            }
            return 0;
        }
        else if (answer == 2){
            cout << "arguments: seed, algo, instance set, instance, NrBreaksPerTeam*, non-constant capacity*, setting**" << endl;
            cout << "* in case instance set = 0 (Miao)" << endl;
            cout << "** in case instance set = 0 (Miao) AND non-constant capacity = 1" << endl;
            return 0;
        }
    }

    if (argc < 2){
        cout << "Enter seed" << endl;
        if (cin >> seed){}
        else{
            cout << "Seed needs to be integer" << endl;
            std::exit(0);
        }
    }
    else{
        seed = std::stoi(argv[1]);
    }

    if (argc < 2){
        cout << "Which algorithm?" << endl;
        cout << "---------------------------" << endl;
        cout << "0: full IP Karel (7200s time limit)" << endl;
        cout << "1: full IP Miao (7200s time limit)" << endl;
        cout << "2: Relax and Fix Miao (3600s+3600s time limit)" << endl;
        cout << "3: Metaheuristic Miao (60s time limit)" << endl;
        cout << "4: Fix-and-Optimize Karel (60s time limit)" << endl;
        cout << "5: Metaheuristic Karel (60s time limit)" << endl;
        cout << "6: run them all" << endl;
        cout << "---------------------------" << endl;
        do{
            cin >> A;
            if (A != 0 && A != 1 && A != 2 && A != 3 && A != 4 && A != 5 && A != 6){
                count++;
                if (count > 3){
                    cout << "Abort, please specify an available algorithm next time" << endl;
                }
                cout << "Please enter an available algorithm (0-5)" << endl;
            }
            else{
                if (A < 6){
                    SetAlgo(AlgoSelected, A);
                    cout << "Run also another algorithm? (No: 0, Yes: >= 1)" << endl;
                    cin >> yn;
                    if (!yn){
                        stop = true;
                    }
                    else{
                        cout << "New algorithm: " << endl;
                    }
                }
                else{
                    for (int a = 0; a < 6; a++){
                        SetAlgo(AlgoSelected, a);
                    }
                    stop = true;
                }
            }
        }
        while (!stop);
    }
    else{
        A = std::stoi(argv[2]);
        SetAlgo(AlgoSelected, A);
    }

    unordered_map<Algorithm, vector<int>>Objectives = {{Algorithm::IP_Karel, {}}, {Algorithm::IP_Miao, {}}, {Algorithm::RF_Miao, {}}, {Algorithm::FO_Karel, {}}, {Algorithm::Meta_Miao, {}}, {Algorithm::Meta_Karel, {}}};
    unordered_map<Algorithm, vector<int>>Durations = {{Algorithm::IP_Karel, {}}, {Algorithm::IP_Miao, {}}, {Algorithm::RF_Miao, {}}, {Algorithm::FO_Karel, {}}, {Algorithm::Meta_Miao, {}}, {Algorithm::Meta_Karel, {}}};
    unordered_map<Algorithm, vector<int>>LowerBounds = {{Algorithm::IP_Karel, {}}, {Algorithm::IP_Miao, {}}, {Algorithm::RF_Miao, {}}, {Algorithm::FO_Karel, {}}, {Algorithm::Meta_Miao, {}}, {Algorithm::Meta_Karel, {}}};
    unordered_map<Algorithm, vector<Solution>>Solutions = {{Algorithm::IP_Karel, {}}, {Algorithm::IP_Miao, {}}, {Algorithm::RF_Miao, {}}, {Algorithm::FO_Karel, {}}, {Algorithm::Meta_Miao, {}}, {Algorithm::Meta_Karel, {}}};

    int Inst;
    if (argc < 2){
        cout << "Instances Miao (0), Hockey (1) or Test (2)?" << endl;
        cin >> Inst;
    }
    else{
        Inst = std::stoi(argv[3]);
    }
    try{
        if (Inst != 0 && Inst != 1 && Inst != 2){
            throw (Inst);
        }
        else{
            string InstPath;
            if (Inst == 0){
                InstPath = std::string(PATHSEP) + "Miao";
            }
            else if (Inst == 1){
                InstPath = std::string(PATHSEP) + "Hockey";
            }
            else{
                InstPath = std::string(PATHSEP) + "Test" + std::string(PATHSEP) + "Single";
            }
            file_path_base += InstPath;
            file_path_initial_solution_base += InstPath;
            file_path_results_base += InstPath;
        }
    }
    catch (char num){
        cout << "Input should be 0 (Miao), 1 (Hockey) or 2 (Test)!" << endl;
    }

    vector<int>Instances;
    int ex;
    int NrInstances;
    count = 0;
    stop = false;
    bool InstancesMiaoChosen = false;
    if (Inst == 0){
        NrInstances = 7;
        InstancesMiaoChosen = true;
        if (argc < 2){
            cout << "Which instance (for all, press 0)?" << endl;
            cout << "1: S" << endl;
            cout << "2: U13" << endl;
            cout << "3: U15" << endl;
            cout << "4: U17" << endl;
            cout << "5: U21" << endl;
            cout << "6: M" << endl; // Does not work yet
            cout << "7: Tiny" << endl; // Does not work for miao algo
            do{
                if (!(cin >> ex)) {
                    cin.clear(); // clear error flag
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
                    cout << "Please enter a number between 1 and 7" << endl;
                    count++;
                }
                else if (++count > 3){
                    cout << "Please specify available instance next time" << endl;
                    return 0;
                }
                else if (ex != 1 && ex != 2 && ex != 3 && ex != 4 && ex != 5 && ex != 6 && ex != 7 && ex != 0){
                    cout << "Please enter an available instance (1-7)" << endl;
                }
                else{
                    stop = true;
                }
            }
            while(!stop);
        }
        else{
            ex = std::stoi(argv[4]);
            /*
            if (ex == 7){
                cout << "This instance does not work yet, NrClubs and nr of columns in distance matrix does not match!!" << endl;
                return 0;
            }
            */
        }
    }
    else if (Inst == 1){
        NrInstances = 6;
        if (argc < 2){
            cout << "Which instance?" << endl;
            cout << "1: U8_indoor_23_24" << endl;
            cout << "2: U8_indoor_24_25" << endl;
            cout << "3: U7_U12_outdoor_22_23" << endl;
            cout << "4: U6_U8_outdoor_23_24a" << endl;
            cout << "5: U7_U9_outdoor_23_24b" << endl;
            cout << "6: U7_U8_outdoor_24_25a" << endl;
            do{
                if (!(cin >> ex)) {
                    cin.clear(); // clear error flag
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
                    cout << "Please enter a number between 1 and 6" << endl;
                    count++;
                }
                else if (++count > 3){
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
            ex = std::stoi(argv[4]);
        }
    }
    else{
        NrInstances = 26;
        if (argc < 2){
            cout << "Which instance?" << endl;
            cout << "Choose 1-26 (for all, press 0)" << endl;
            do{
                if (!(cin >> ex)) {
                    cin.clear(); // clear error flag
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
                    cout << "Please enter a number between 1 and 26" << endl;
                    count++;
                }
                else if (++count > 3){
                    cout << "Please specify available instance next time" << endl;
                    return 0;
                }
                else if (ex < 0 || ex > 26){
                    cout << "Please enter an available instance (1-26) or all (0)" << endl;
                }
                else{
                    stop = true;
                }
            }
            while(!stop);
        }
        else{
            ex = std::stoi(argv[4]);
        }
    }
    count = 0;
    stop = false;
    if (ex != 0){
        Instances.push_back(ex);
    }
    else{
        Instances.resize(NrInstances);
        for (int i = 1; i < Instances.size()+1; i++){ // TODO: not all instance for MIAO!!!
            Instances[i-1] = i;
        }
    }

    int NrBreaksPerTeam = 100; // unlimited for hockey and test
    int Cap;
    int Setting;

    if (Inst == 0){
        if (argc < 2){
            cout << "Max nr of Breaks per team? (0, 1, 2 or 3)" << endl;
            do{
                if (!(cin >> NrBreaksPerTeam)) {
                    cin.clear(); // clear error flag
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
                    cout << "Please enter a number between 0 and 3" << endl;
                    count++;
                }
                else if (++count > 3){
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
            NrBreaksPerTeam = std::stoi(argv[5]);
        }
        count = 0;
        stop = false;

        if (argc < 2){
            cout << "Constant (0) or Variable (1) capacity?" << endl;
            do{
                if (!(cin >> Cap)) {
                    cin.clear(); // clear error flag
                    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
                    cout << "Please enter a number between 0 and 1" << endl;
                    count++;
                }
                else if (++count > 3){
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
            Cap = std::stoi(argv[6]);
        }
        count = 0;
        stop = false;
        
        string CapPath;
        if (Cap == 0){
            CapPath = std::string(PATHSEP) + "ConstantCapacity";
        }
        else{
            CapPath = std::string(PATHSEP) + "VariableCapacity";
        }
        file_path_base += CapPath;
        file_path_initial_solution_base += CapPath;
        file_path_results_base += CapPath;
        
        if (Cap == 1){
            if (argc < 2){
                do{
                    cout << "Setting? (1 or 2) " << endl;
                    if (!(cin >> Setting)) {
                        cin.clear(); // clear error flag
                        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
                        cout << "Please enter a number between 0 and 1" << endl;
                        count++;
                    }
                    else if (++count > 3){
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
                Setting = std::stoi(argv[7]);
            }
            
            string SettingPath;
            if (Setting == 1){
                SettingPath = std::string(PATHSEP) + "Setting1";
            }
            else{
                SettingPath = std::string(PATHSEP) + "Setting2";
            }
            file_path_base += SettingPath;
            file_path_initial_solution_base += SettingPath;
            file_path_results_base += SettingPath;
        }
    }

    if (Inst == 0){ // If Miao is chosen
        file_path_initial_solution_base += (std::string(PATHSEP) + "b" + to_string(NrBreaksPerTeam));
        file_path_results_base += (std::string(PATHSEP) + "b" + to_string(NrBreaksPerTeam)); 
    }
            

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
        // BalancedCycle -> ERROR
        // Bipartite matching -> done
        // Normal matching -> ERROR
        // TS -> done, also stays the same
        // PTS -> done
    // TODO: calculate minimum capacity violations for each instance and store them in a file
    // Check whether these values are the same as Miao!!!
    // TODO: initial solution for hockey?
    // Do greedy matchings, then assign HAPs, try to fix capacities?

    // std::cout << "Start" << std::endl;

    // Just provide complete path --> needed for comp David
    std::string file_path;
    std::string file_path_initial_solution;

    std::string file_path_results_performance = file_path_results_base + std::string(PATHSEP) + "Performance" + std::string(PATHSEP) + "Results_TS_test.txt";
    std::ofstream results_file_performance(file_path_results_performance);
    results_file_performance << "Move, NrChosen, NrAccepted, % Improvement found, Average improvement found, % contribution to best objective, Instance\n";

    std::string file_path_results_failures = file_path_results_base + std::string(PATHSEP) + "Failures" + std::string(PATHSEP) + "Results_TS_test.txt";
    std::ofstream results_file_failures(file_path_results_failures);
    results_file_failures << "Instance,TS-InfOpp,TS-HAP,PTS-InfOpp,PTS-HAP,PTS-DRR,PTS-NoPath,PRS-HAP,M-HAP,M-NoPath,C-DRR\n";

    for (int inst: Instances){
        string instance_path;
        if (inst < 10){
            instance_path = std::string(PATHSEP) + "i0";
        }
        else{
            instance_path = std::string(PATHSEP) + "i";
        }
        instance_path += to_string(inst) + ".txt";
        file_path = file_path_base + instance_path;
        file_path_initial_solution = file_path_initial_solution_base + instance_path;
        // cout << file_path << endl;
        
        // std::string file_path = (string) argv[1];
        Input in;
        // cout << "start reading file" << endl;
        if (!in.read(file_path, InstancesMiaoChosen)){
            cout << "Could not read " << file_path << endl;
            return 0;
        }
        // cout << "done reading file" << endl;
        bool NoThreeConsecutive = true;
        bool BreakLimit;
        bool NoBreakBeginningEnd;
        bool QuarterBalanced;
        if (Inst == 0){
            BreakLimit = true;
            NoBreakBeginningEnd = true;
            QuarterBalanced = true;
        }
        else{
            BreakLimit = false;
            NoBreakBeginningEnd = false;
            QuarterBalanced = false;
        }
        if (Inst == 2){
            in.setMaxSameClub(in.getNrTeams()); // No restrictions
        }
        in.setHAP_requirements(NoThreeConsecutive, NoBreakBeginningEnd, QuarterBalanced, BreakLimit, NrBreaksPerTeam);

        if (InstancesMiaoChosen){
            // cout << "set HAP settings Miao" << endl;
            if (Cap == 1){
                in.setMiaoHAPSetting(Setting);
            }
            in.setAllowedNrCapacityViolations();
        }

        if (!in.read_HAPs()){ // do not use HAPs for hockey for now
            cout << "Problem with reading HAP files" << endl;
            return 0;
        }
        if (Inst == 2){
            // I have some assert statements that do not work with SRR!
            in.SRR = false;
        }
        else{
            in.SRR = false;
        }

        // Next steps: comparing different algorithms on instances
        // Is FO working well??
        // Finding feasible solutions for harder instances?
        // IP, Method Miao, or neighborhoods?
        // Instance 8: IP finds feasible solution in 32s, Miao in 3s
        // Instance 10: IP finds solution within 13s, Miao in 3s
        // Instance 11: IP finds solution in 11s, Miao in 2s
        // Instance 16: IP finds solution in 33s, Miao in 3s
        // Instance 17: IP finds solution in 18s, Miao in 1s

        int N = in.getNrTeams();
        const int TimeLimitIP = 20;

        if (AlgoSelected.at(Algorithm::IP_Miao)){
            GurSolver gur_miao(in);
            Solution sol(in);
            bool relax_x = false;
            gur_miao.setTimeLimit(TimeLimitIP);
            gur_miao.BuildMiaoFormulation(relax_x);
            gur_miao.AddObj(true, false); 
            gur_miao.TrackTimeBestSolution = true;
            // cout << "Solve IP Miao" << endl;
            ReadSolution(file_path_initial_solution, sol); 
            gur_miao.WarmStart(sol); 
            gur_miao.solve();
            gur_miao.SaveSolution(sol);
            sol.validate();
            Objectives.at(Algorithm::IP_Miao).push_back(sol.ComputeTotalCost());
            Durations.at(Algorithm::IP_Miao).push_back(gur_miao.getTimeTillBestSolution());
            LowerBounds.at(Algorithm::IP_Miao).push_back(gur_miao.getBestBound());
            Solutions.at(Algorithm::IP_Miao).push_back(sol);
        }

        const bool FindInitialSolutionWithMiao = false; // try to find initial solutions for hockey

        if (AlgoSelected.at(Algorithm::Meta_Miao)){
            for (auto& MetaSeed: MetaSeeds){
                Solution sol(in);
                std::unordered_map<HAP_operator, double>weights;
                if (NrBreaksPerTeam == 0){
                    weights = Miao_weights_zero_breaks;
                }
                else if (InstancesMiaoChosen && in.getNrTeams() == 608){
                    weights = Miao_weights_Merged;
                }
                else{
                    weights = Miao_weights;
                }
                MiaoAlgo miao_algo(Miao_operators, weights, in, MetaSeed);
                if (!FindInitialSolutionWithMiao){
                    ReadSolution(file_path_initial_solution, sol);
                    miao_algo.InitialSolutionGiven = true;
                }
                if (FindInitialSolutionWithMiao){
                    miao_algo.InitialOnly = true; 
                }
                miao_algo.solve(in, sol);
                // cout << "Solve Meta Miao" << endl;
                sol.validate();
                if (FindInitialSolutionWithMiao){
                    SaveSolution(file_path_initial_solution, sol); // INITIAL
                    return 0;
                }
                miao_algo.SaveResultsMoves(results_file_performance, inst);
                Objectives.at(Algorithm::Meta_Miao).push_back(sol.ComputeTotalCost());
                Durations.at(Algorithm::Meta_Miao).push_back(miao_algo.TimeTillBestSolution);
                LowerBounds.at(Algorithm::Meta_Miao).push_back(sol.ComputeTotalCost());
                Solutions.at(Algorithm::Meta_Miao).push_back(sol);
            }
        }

        if (AlgoSelected.at(Algorithm::RF_Miao)){
            Solution sol(in);
            // ReadSolution(file_path_initial_solution, sol); // TURN BACK
            // cout << "Solve RF Miao" << endl;
            int LowerBound_RF = 0;
            int dur = RF_Miao(in, sol, LowerBound_RF);
            sol.validate();
            if (dur >= 0){
                LowerBounds.at(Algorithm::RF_Miao).push_back(LowerBound_RF);
                Objectives.at(Algorithm::RF_Miao).push_back(sol.ComputeTotalCost());
                Durations.at(Algorithm::RF_Miao).push_back(dur);
                Solutions.at(Algorithm::RF_Miao).push_back(sol);
            }
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
            // cout << "solve IP Karel" << endl;
            gur.setTimeLimit(TimeLimitIP);
            gur.TrackTimeBestSolution = true;
            ReadSolution(file_path_initial_solution, sol);
            FindSolutionIP(gur, sol, HA, min_travel, min_cap);
            // cout << "save solution" << endl;
            gur.SaveSolution(sol);
            sol.validate();
            Objectives.at(Algorithm::IP_Karel).push_back(sol.ComputeTotalCost());
            Durations.at(Algorithm::IP_Karel).push_back(gur.getTimeTillBestSolution());
            LowerBounds.at(Algorithm::IP_Karel).push_back(gur.getBestBound());
            Solutions.at(Algorithm::IP_Karel).push_back(sol);
            // cout << "cost = " << sol.ComputeTotalCost() << endl;
        }

        // Save initial solutions in file in map?

        if (AlgoSelected.at(Algorithm::FO_Karel)){
            Solution sol(in);
            // const bool min_travel = false, min_cap = false;
            // FindSolutionIP(gur, sol, HA, min_travel, min_cap);
            // FindInitialSolution(in, sol);
            ReadSolution(file_path_initial_solution, sol);
            FO fo(in, FO_operators, FO_weights, seed);
            // cout << "Solve FO" << endl;
            fo.solve(in,sol);
            sol.validate();
            fo.SaveResultsMoves(results_file_performance, inst);
            Objectives.at(Algorithm::FO_Karel).push_back(sol.ComputeTotalCost());
            Durations.at(Algorithm::FO_Karel).push_back(fo.TimeTillBestSolution);
            LowerBounds.at(Algorithm::FO_Karel).push_back(sol.ComputeTotalCost());
            Solutions.at(Algorithm::FO_Karel).push_back(sol);
            // cout << "Objective = " << sol.ComputeTotalCost() << endl;
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

            for (auto& MetaSeed: MetaSeeds){
                Solution sol(in);
                // FindInitialSolution(in, sol);
                ReadSolution(file_path_initial_solution, sol);
                // RF_Miao(in, sol); -> does not find improvements..
        
                ILS ils(Karel_operators, Karel_weights, MetaSeed);
                std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
                ils.setStartTime(start_time);
                ils.solve(in, sol);
                // cout << "validate" << endl;
                sol.validate();
                // cout << "done" << endl;
                results_file_failures << inst << ",";
                ils.SaveResultsFailures(results_file_failures);
                ils.SaveResultsMoves(results_file_performance, inst);
                Objectives.at(Algorithm::Meta_Karel).push_back(sol.ComputeTotalCost());
                Durations.at(Algorithm::Meta_Karel).push_back(ils.TimeTillBestSolution);
                LowerBounds.at(Algorithm::Meta_Karel).push_back(sol.ComputeTotalCost());
                Solutions.at(Algorithm::Meta_Karel).push_back(sol);
            }
        }

#ifdef PRINT
#if PRINT == 1
        cout << "******************************" << endl;
        cout << "Solutions found by algorithms: " << endl;
        cout << "------------------------------" << endl;
#endif
#endif
        for (auto&[algo, selected]: AlgoSelected){
            if (selected){
                std::string file_path_results_objectives = file_path_results_base + std::string(PATHSEP) + "Objectives" + std::string(PATHSEP) + AlgoString.at(algo) + "_" + to_string(inst) + "_TS_test.txt";
#ifdef PRINT
#if PRINT == 1
                cout << "save file in " << file_path_results_objectives << endl;
#endif
#endif
                std::ofstream results_file_objectives(file_path_results_objectives);
                // Algo, Instance, Best, Time, Avg, Time, Bound
                results_file_objectives << AlgoString.at(algo) << "," << to_string(inst);
                if (Solutions.at(algo).size() == 0){
                    results_file_objectives << "," << -1 << "," << -1 << "," << -1 << "," << -1 << "\n";
                    continue;
                }
                assert(Objectives.at(algo).size() > 0);
                assert(Durations.at(algo).size() > 0);
                assert(LowerBounds.at(algo).size() > 0);
                assert(Objectives.at(algo).size() == Durations.at(algo).size());
                // get best and average value
                int best_value_index = 0;
                int best_value = Objectives.at(algo)[0];
                int average_value = Objectives.at(algo)[0];
                int average_duration = Durations.at(algo)[0];
                for (int i = 1; i < Objectives.at(algo).size(); ++i){
                    if (Objectives.at(algo)[i] < best_value){
                        best_value_index = i;
                        best_value = Objectives.at(algo)[i];
                    }
                    average_value += Objectives.at(algo)[i];
                    average_duration += Durations.at(algo)[i];
                }
                int best_lb = LowerBounds.at(algo)[0];
                for (int i = 1; i < LowerBounds.at(algo).size(); ++i){
                    if (LowerBounds.at(algo)[i] > best_lb){
                        best_lb = LowerBounds.at(algo)[i];
                    }
                }
                average_value /= Objectives.at(algo).size();
                average_duration /= Durations.at(algo).size();
                int best_duration = Durations.at(algo)[best_value_index];
                results_file_objectives << "," << best_value << "," << best_duration << "," << average_value << "," << average_duration << "\n";
                Solution BestSol = Solutions.at(algo)[best_value_index];
                for (int i = 0; i < BestSol.getNrTeams(); ++i){
                    for (int j = 0; j < BestSol.getNrTeams(); ++j){
                        for (int r = 0; r < BestSol.getNrRounds(); ++r){
                            if (BestSol.MatchColor[i][j] == r){
                                results_file_objectives << i << "," << j << "," << r << "\n";
                            }
                        }
                    }
                }
                results_file_objectives.close();
#ifdef PRINT
#if PRINT == 1
                cout << AlgoString.at(algo) << ": " << best_value << " in " << best_duration << "s" << endl;
#endif
#endif
            }
        }
#ifdef PRINT
#if PRINT == 1
        cout << "******************************" << endl;
#endif
#endif
    }

    results_file_performance.close();
    results_file_failures.close();

    return 1;
}
