#ifndef CM_H  
#define CM_H

#include "Input.h"
#include "GurSolver.h"
#include "Heuristic_CM.h"
#include "Algo.h"

#include <filesystem>

const vector<string>InstancesTTP = {"BRA24", "CIRC40", "CON40", "GAL40", "INCR40", "LINE40", "N16", "NFL32"};

inline std::string FolderPathTTP() {
    return "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP);
}

inline std::string FolderPathCM() {
    return "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel" + std::string(PATHSEP) + "0_100" + std::string(PATHSEP); 
}

namespace fs = std::filesystem;

unordered_map<move_name_CM, double> WeightsMap(const unordered_map<string, double>& InputWeights){
    // Weights: see Heuristic_CM.h
    unordered_map<move_name_CM, double>Weights;
    for (const auto& [move, weight]: InputWeights){
        if (move == "TS" && weight > 0.0){
            Weights[move_name_CM::TS] = weight;
        }
        else if (move == "PTS" && weight > 0.0){
            Weights[move_name_CM::PTS] = weight;
        }
        else if (move == "RS" && weight > 0.0){
            Weights[move_name_CM::RS] = weight;
        }
        else if (move == "PRS" && weight > 0.0){
            Weights[move_name_CM::PRS] = weight;
        }
        else if (move == "M" && weight > 0.0){
            Weights[move_name_CM::M] = weight;
        }
        else if (move == "BM" && weight > 0.0){
            Weights[move_name_CM::BM] = weight;
        }
        else if (move == "C" && weight > 0.0){
            Weights[move_name_CM::C] = weight;
        }
    }
    return Weights;
}

void SaveSolutionXML(std::ofstream& output_file, Solution& sol){
	// Ugly printing to get the RobinX solution file format

	output_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	output_file << "<Solution>\n";

	// --- MetaData section (you can make these parameters if needed) ---
	output_file << "    <MetaData>\n";
	output_file << "        <InstanceName>LINE6</InstanceName>\n";
	output_file << "        <Contributor>Devriesere, Karel</Contributor>\n";
	output_file << "        <Date year=\"2025\" month=\"\" day=\"\"/>\n";
	output_file << "        <ObjectiveValue infeasibility=\"" << sol.ComputeTotalCostTTPViolations() << "\" objective=\"" << sol.ComputeTravelCostTTP() << "\"/>\n";
	output_file << "        <Remarks/>\n";
	output_file << "    </MetaData>\n";

	// --- Games section ---
	output_file << "    <Games>\n";

	int nrRounds = sol.getNrRounds();
	int nrTeams  = sol.getNrTeams();

	for (int r = 0; r < nrRounds; ++r) {
		std::vector<bool> NodeSeen(nrTeams, false);
		for (int i = 0; i < nrTeams; ++i) {
			if (!NodeSeen[i]) {
				int j = sol.TeamColorOpp[i][r];
				if (sol.Orientation[i][r] == HA::H) {
					output_file << "        <ScheduledMatch home=\"" << i
						<< "\" away=\"" << j
						<< "\" slot=\"" << r << "\"/>\n";
				} else {
					output_file << "        <ScheduledMatch home=\"" << j
						<< "\" away=\"" << i
						<< "\" slot=\"" << r << "\"/>\n";
				}
				NodeSeen[j] = true;
			}
		}
	}

    output_file << "    </Games>\n";
    output_file << "</Solution>\n";
}


void SaveSolution(std::ofstream& output_file, Solution& sol){
    int i,j,r;
    output_file << "Round,H_team,A_team \n";
    for (r = 0; r < sol.getNrRounds(); ++r){
        vector<bool>NodeSeen(sol.getNrTeams(), false);
        for (i = 0; i < sol.getNrTeams(); ++i){
            if (!NodeSeen[i]){
                j = sol.TeamColorOpp[i][r];
                if (sol.Orientation[i][r] == HA::H){
                    output_file << r << "," << i << "," << j << "\n";
                }
                else{
                    output_file << r << "," << j << "," << i << "\n";
                }
                NodeSeen[j] = true;
            }
        }
    }
}

void SolveHeuristic(Input& in, const int seed, const bool MinCostNB, const int HistoryLength, const int TimeLimit, const int MaxIt, vector<int>& TimeStamps, const string Instance, const unordered_map<string, double>& InputWeights, const string FolderPath){
    // Find initial solution with Vizing
    Solution sol(in);
    cout << "Solve Vizing" << endl;
    VizingConstruction(sol, seed);
    assert(sol.validate());
    const int obj = sol.ComputeTotalCost();
    cout << "Cost initial solution = " << obj << endl;

    std::mt19937 gen(seed);
    unordered_map<move_name_CM, double>Weights = WeightsMap(InputWeights);
    Heuristic_CM algo(Moves, Weights, gen, 10, obj, MinCostNB);
    algo.setTimeLimit_meta(TimeLimit);
    algo.SetMaxIt(MaxIt);
    algo.SetTimeStamps(TimeStamps);
    algo.solve(in, sol);
    algo.SaveBestSolution(sol);
    sol.validate();
    cout << "Final solution = " << sol.ComputeTotalCost() << endl;

    string FilePath = FolderPath + "Results" + std::string(PATHSEP) + "Heuristic" + std::string(PATHSEP);
    if (MinCostNB){
        FilePath += "MinCost";
    }
    else{
        FilePath += "NoMinCost";
    }
    FilePath += std::string(PATHSEP) + Instance;
    if (in.getSetting() == Setting::TTP){
        FilePath += "_" + to_string(in.getNrRounds());
    }
    FilePath += "_s" + to_string(seed) + "_HL" + to_string(HistoryLength) + ".txt";
    const string config = to_string(seed) + ",Heuristic," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds()) + "," + to_string(MinCostNB) + "," + to_string(HistoryLength);

    cout << "Save file as " << FilePath << endl;
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    algo.SaveSolutionsTimeStamps(output_file);
    SaveSolution(output_file, sol);

    // Replace txt extension with XML
    FilePath.replace(FilePath.size() - 4, 4, ".xml");
    cout << "Save XML file as " << FilePath << endl;
    std::ofstream output_fileXML(FilePath);
    SaveSolutionXML(output_fileXML, sol);

    output_file.close();
    return;
}

void SolveIP(Input& in, const int seed, const int TimeLimit, vector<int>& TimeStamps, const string Instance, const string FolderPath){
    GurSolver gur(in);
    Solution sol(in);
    bool HA = true;
    bool relax_x = false;
    if (in.getSetting() == Setting::CM){
        gur.build_base(HA, relax_x);
        gur.AddObjGeneralCosts();
    }
    else{
        assert(in.getSetting() == Setting::TTP);
        gur.iTTP();
    }
    gur.setTimeLimit(TimeLimit);
    gur.SetTimeStamps(TimeStamps);
    gur.solve();
    gur.SaveSolution(sol);
    sol.validate();
    cout << "Final solution = " << sol.ComputeTotalCost() << endl;

    const string FilePath = FolderPath + "Results" + std::string(PATHSEP) + "IP" + std::string(PATHSEP) + Instance + "_" + to_string(in.getNrRounds()) + ".txt";
    const string config = to_string(seed) + ",IP," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
    cout << "Save file as " << FilePath << endl;
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    gur.SaveSolutionsTimeStamps(output_file);
    SaveSolution(output_file, sol);
    if (in.getSetting() == Setting::TTP){
        // Time limit is doing really weird stuff, giving infeasible solutions
        // Is gurobi also giving a high cost to infeasible solutions???
        output_file << "Travel cost = " << sol.ComputeTravelCostTTP() << "\n";
        output_file << "TTP violations cost = " << sol.ComputeTotalCostTTPViolations() << "\n";
    }
    output_file.close();
    cout << "Close file" << endl;

    // cin.get();
    return;
}

void TestCostMinimization(const int seed, const string Instance, const bool CM, const bool TTP, const bool Heuristic, const bool MinCostNB, const int HistoryLength, const int TimeLimit, const int MaxIt, const unordered_map<string, double>& InputWeights, const int NrRoundsTTP, const bool BaseAlgo){

    vector<int>TimeStamps;
    int TimeStamp = 0;
    int Incrementor = 30;
    while (TimeStamp <= TimeLimit){
        TimeStamps.push_back(TimeStamp);
        TimeStamp += Incrementor;
    }

    string FilePath;
    string FolderPath;
    if (CM){
        FolderPath = FolderPathCM();
        FilePath = FolderPath + Instance + ".txt";
    }
    else{
        FolderPath = FolderPathTTP();
        FilePath = FolderPath + Instance + ".xml";
    }
    cout << "FilePath: " << FilePath << endl;

    Input in;
    if (CM && !in.read_CostMinimization(FilePath, InstanceSetCM::Karel)){
        cout << "Could not read " << FilePath << endl;
        return;
    }
    else if (TTP && !in.read_TTP(FilePath, NrRoundsTTP)){
        cout << "could not read " << FilePath << endl;
        return;
    }
    if (BaseAlgo){
        if (!Heuristic){
            cout << "Base algo but IP: do not change NrRounds!!!" << endl;
        }
        else{
            in.setBaseAlgo();
        }
    }
    in.setHAP_requirements(false, false, false, true, in.getNrRounds());
    in.SRR = true;
    
    if (Heuristic){
        SolveHeuristic(in, seed, MinCostNB, HistoryLength, TimeLimit, MaxIt, TimeStamps, Instance, InputWeights, FolderPath);
    }
    else{
        SolveIP(in,seed,TimeLimit,TimeStamps, Instance, FolderPath);
    }
}

void BoundsTTP(){
    const string OutputFilePath = "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP) + "Bounds.txt";
    cout << "Save file as " << OutputFilePath << endl;
    std::ofstream output_file(OutputFilePath);

    for (string Instance: InstancesTTP){
        Input in;
        string FilePath = FolderPathTTP() + Instance + ".xml";

        vector<int>Rounds = {10,20,30};
        if (Instance == "N16"){
            Rounds = {4,8,12};
        }
        else if (Instance == "BRA24"){
            Rounds = {6,12,18};
        }
        else if (Instance == "NFL32"){
            Rounds = {8,16,24};
        }

        for (int NrRoundsTTP: Rounds){

            if (!in.read_TTP(FilePath, NrRoundsTTP)){
                cout << "could not read " << FilePath << endl;
                return;
            }

            GurSolver gur(in);
            Solution sol(in);
            int sum = 0;
            for (int t = 0; t < in.getNrTeams(); ++t){
                gur.BoundTTP(t);
                sum += gur.solve();
            }
            output_file << Instance << "," << sum << "," << NrRoundsTTP << "\n";
            cout << "Bound of " << Instance << " with " << NrRoundsTTP << " = " << sum << endl;
        }
    }

    output_file.close();
}

#endif
