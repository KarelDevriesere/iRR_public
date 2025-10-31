#include "CM.h"

string PathInitialSolutionMiao(InputData data){
    string path = "InitialSolutions"  + string(PATHSEP)  +"Miao" + string(PATHSEP);
    if (data.ConstantCapacity){
        path += "ConstantCapacity" + string(PATHSEP);
    }
    else{
        path += "VariableCapacity" + string(PATHSEP) + "Setting" + to_string(data.CapacitySetting) + string(PATHSEP);
    }
    path += "b" + to_string(data.MaxNrBreaks) + string(PATHSEP) + data.Instance + ".txt";
    return path;
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

std::string FolderPath(const InputData& data) {
    string folder_path;
    if (data.TTP){
        folder_path =  "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP);
    }
    else if (data.CM){
        folder_path = "Instances" + std::string(PATHSEP) + "CostMinimization" + std::string(PATHSEP) + "Karel" + std::string(PATHSEP) + "0_100" + std::string(PATHSEP);
    }
    else {
        folder_path = "Instances" + std::string(PATHSEP) + "Miao"  + std::string(PATHSEP);
        if (data.ConstantCapacity){
            folder_path += ("ConstantCapacity" + std::string(PATHSEP));
        }
        else{
            folder_path += ("VariableCapacity" + std::string(PATHSEP));
            if (data.CapacitySetting == 1){
                folder_path += ("Setting1" + std::string(PATHSEP));
            }
            else{
                folder_path += ("Setting2" + std::string(PATHSEP));
            }
        }
    }
    return folder_path;

}

void SaveSolutionXML(std::ofstream& output_file, Solution& sol){
	// Ugly printing to get the RobinX solution file format

	output_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	output_file << "<Solution>\n";

	// --- MetaData section (you can make these parameters if needed) ---
	output_file << "    <MetaData>\n";
	output_file << "        <InstanceName>INSTANCE_NAME_TO_BE_FILLED_IN</InstanceName>\n";
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

void SolveHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data){
    // Find initial solution with Vizing
    Solution sol(in);
    sol.SetOneCostAllViolations(data.ConstrViolationCost);
    if (sol.getSetting() != Setting::Miao){
        cout << "Solve Vizing" << endl;
        VizingConstruction(sol, data.seed);
        cout << "Found initial solution" << endl;
    }
    else{
        string path = PathInitialSolutionMiao(data);
        ReadSolution(path, sol);
    }
    assert(sol.validate());
    int obj = sol.ComputeTotalCost();
    cout << "Cost initial solution = " << obj << endl;

    std::mt19937 gen(data.seed);
    Heuristic_CM algo(data.Moves, data.InputWeights, gen, data.HistoryLength, obj);
    algo.setTimeLimit_meta(data.TimeLimit);
    algo.SetMaxIt(data.MaxIt);
    algo.SetTimeStamps(TimeStamps);
    algo.solve(in, sol);

    int NrViolations = 0;
    bool HillClimbing = false;
    if (data.TTP){
        sol.SetOneCostAllViolations(1);
        NrViolations = sol.ComputeTotalCostTTPViolations();
        if (NrViolations > 0){
            HillClimbing = true;
            cout << "Solution not feasible, nr of violations = " << NrViolations << endl;
            cout << "Start hill climbing" << endl;
            // Hill climbing
            sol.SetOneCostAllViolations(100000);
            obj = sol.ComputeTotalCost();
            algo.SetHistoryLength(5);
            algo.setTimeLimit_meta(2*data.TimeLimit);
            algo.solve(in, sol);
            sol.SetOneCostAllViolations(1);
            NrViolations = sol.ComputeTotalCostTTPViolations();
            cout << "Final nr of violations = " << NrViolations << endl;
            sol.SetOneCostAllViolations(0);
        }
    }

    algo.SaveBestSolution(sol);
    sol.validate();
    cout << "Final solution (travel cost) = " << sol.ComputeTotalCost() << endl;

    string FilePath;
    if (data.OutputFolder.empty()){
        FilePath = FolderPath + "Results" + std::string(PATHSEP);
        if (data.Base){
            FilePath += "Base";
        }
        else{
            FilePath += "Heuristic";
        }
    }
    else{
        FilePath = data.OutputFolder;
    }
    FilePath += std::string(PATHSEP) + data.Instance;
    if (in.getSetting() == Setting::TTP){
        FilePath += "_" + to_string(in.getNrRounds());
    }
    FilePath += "_s" + to_string(data.seed) + "_HL" + to_string(data.HistoryLength) + ".txt";

    string config = to_string(data.seed);
    if (data.Base){
        config += ",BaseAlgo,";
    }
    else{
        config += ",Heuristic,";
    }
    config += to_string(data.MaxIt) + "," + to_string(data.TimeLimit) + "," + to_string(data.HistoryLength) + "," + to_string(data.ConstrViolationCost) + "," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());

    cout << "Save file as " << FilePath << endl;
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    algo.SaveSolutionsTimeStamps(output_file);
    if (data.TTP){
        output_file << "NrViolations," << NrViolations << "\n";
    }
    SaveSolution(output_file, sol);

    // Replace txt extension with XML
    FilePath.replace(FilePath.size() - 4, 4, ".xml");
    cout << "Save XML file as " << FilePath << endl;
    std::ofstream output_fileXML(FilePath);
    SaveSolutionXML(output_fileXML, sol);

    output_file.close();
    return;
}

void SolveIP(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data){
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
    gur.setTimeLimit(data.TimeLimit);
    gur.SetTimeStamps(TimeStamps);
    gur.solve();
    gur.SaveSolution(sol);
    sol.validate();
    cout << "Final solution = " << sol.ComputeTotalCost() << endl;

    const string FilePath = FolderPath + "Results" + std::string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + "_" + to_string(in.getNrRounds()) + ".txt";
    const string config = to_string(data.seed) + ",IP," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
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

void TestCostMinimization(const InputData& data){

    vector<int>TimeStamps;
    int TimeStamp = 0;
    int Incrementor = 30;
    while (TimeStamp <= data.TimeLimit){
        TimeStamps.push_back(TimeStamp);
        TimeStamp += Incrementor;
    }

    string folder_path = FolderPath(data);
    cout << "FolderPath: " << folder_path << endl;
    string file_path;
    if (data.TTP){
        file_path = data.Instance;
    }
    else{
        file_path = folder_path + data.Instance + ".txt";
    }

    Input in;
    if (data.CM && !in.read_CostMinimization(file_path, InstanceSetCM::Karel)){
        cout << "Could not read " << file_path << endl;
        return;
    }
    else if (data.TTP && !in.read_TTP(file_path)){
        cout << "could not read " << file_path << endl;
        return;
    }
    else if (data.Miao && !in.read_Miao_Hockey(file_path, data.Miao)){
        cout << "could not read " << file_path << endl;
        return;
    }
    if (data.Base){
        if (!data.Heuristic){
            cout << "Base algo but IP: do not change NrRounds!!!" << endl;
        }
        else{
            in.setBaseAlgo();
        }
    }
    if (!data.Miao){
        in.setHAP_requirements(false, false, false, false, in.getNrRounds());
        in.SRR = true;
    }
    else{
        in.setHAP_requirements(true, true, true, true, data.MaxNrBreaks);
        in.setMaxSameClub(2);
        in.SRR = false;
        if (!data.ConstantCapacity){
            in.setMiaoHAPSetting(data.CapacitySetting);
        }
        in.setAllowedNrCapacityViolations();
    }
    
    if (data.Heuristic){
        SolveHeuristic(in,TimeStamps,folder_path,data);
    }
    else{
        SolveIP(in,TimeStamps,folder_path,data);
    }
}

void BoundTTP(const int TimeLimit, const string Instance, const int NrRoundsTTP, std::ofstream& output_file){

    Input in;
    InputData data;
    data.TTP = true;

    if (!in.read_TTP(Instance)){
        cout << "could not read " << Instance << endl;
        return;
    }

    GurSolver gur(in);
    Solution sol(in);
    int sum = 0;
    gur.setTimeLimit(TimeLimit); 
    gur.BoundTTP_AllTeams();
    sum += gur.solve();

    /*
    for (int t = 0; t < in.getNrTeams(); ++t){
        gur.BoundTTP(t);
        sum += gur.solve();
    }
    */

    cout << "sum for instance " << Instance << " with " << NrRoundsTTP << " = " << sum << endl;

    output_file << Instance << "," << sum << "," << NrRoundsTTP << "\n";
}

void BoundsTTP_OneInstance(InputData& data){
    string OutputFilePath = "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP) + "Bound_" + data.Instance + "_" + to_string(data.NrRounds) + ".txt";
    cout << "Save file as " << OutputFilePath << endl;
    std::ofstream output_file(OutputFilePath);
    BoundTTP(data.TimeLimit, data.Instance, data.NrRounds, output_file);
    output_file.close();
}

void BoundsTTP_All(const InputData& data){
    bool Bounds2RR = false;
    string OutputFilePath = "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP) + "Bounds.txt";
    if (Bounds2RR){
        OutputFilePath = "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP) + "Bounds_2RR.txt";
    }
    cout << "Save file as " << OutputFilePath << endl;
    std::ofstream output_file(OutputFilePath);

    for (string Instance: InstancesTTP){

        vector<int>Rounds;
        Rounds = {10,20,30};
        if (Bounds2RR){
            Rounds = {2*39};
        }
        if (Instance == "N16"){
            Rounds = {4,8,12};
            if (Bounds2RR){
                Rounds = {2*15};
            } 
        }
        else if (Instance == "BRA24"){
            Rounds = {6,12,18};
            if (Bounds2RR){
                Rounds = {2*23};
            }
        }
        else if (Instance == "NFL32"){
            Rounds = {8,16,24};
            if (Bounds2RR){
                Rounds = {2*31};
            }
        }
        else if (Instance == "SUP12"){
            Rounds = {4,6,8};
            if (Bounds2RR){
                Rounds = {2*11};
            }
        }

        for (int NrRoundsTTP: Rounds){
            BoundTTP(data.TimeLimit, Instance, NrRoundsTTP, output_file);
        }
    }

    output_file.close();
}
