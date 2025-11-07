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

const unordered_map<string,int>InstanceHL = {
    {"I_BRA24_6",100000},
    {"I_BRA24_12",10000},
    {"I_BRA24_18",10000},
    {"I_CIRC40_10",500},
    {"I_CIRC40_20",500},
    {"I_CIRC40_30",500},
    {"I_CON40_10",1},
    {"I_CON40_20",100},
    {"I_CON40_30",1},
    {"I_GAL40_10",1000},
    {"I_GAL40_20",500},
    {"I_GAL40_30",500},
    {"I_INCR40_10",1000},
    {"I_INCR40_20",500},
    {"I_INCR40_30",500},
    {"I_LINE40_10",1000},
    {"I_LINE40_20",1000},
    {"I_LINE40_30",500},
    {"I_NL16_4",1000},
    {"I_NL16_8",10000},
    {"I_NL16_12",10000},
    {"I_NFL32_8",1000},
    {"I_NFL32_16",1000},
    {"I_NFL32_24",1000}
};

const unordered_map<string,int>InstanceBound = {
    {"I_BRA24_6",38745},
    {"I_BRA24_12",98815},
    {"I_BRA24_18",167657},
    {"I_CIRC40_10",560},
    {"I_CIRC40_20",1760},
    {"I_CIRC40_30",3600},
    {"I_CON16_4",48}, 		// Via Benders
    {"I_CON16_8",96}, 		// Via Benders
    {"I_CON16_12",132}, 	// Via Benders
    {"I_CON24_6",96}, 		// Via Benders
    {"I_CON24_12",192}, 	// Via Benders
    {"I_CON24_18",291}, 	// Via Benders, running on HPC
    {"I_CON32_8",192}, 		// Via Benders
    {"I_CON32_16",352}, 	// Via Benders
    {"I_CON32_24",516}, 	// Via Benders, running on HPC
    {"I_CON40_10",280}, 	// Via Benders
    {"I_CON40_20",555}, 	// Via Benders, running on HPC
    {"I_CON40_30",803}, 	// Via Benders, running on HPC
    {"I_GAL40_10",23617},
    {"I_GAL40_20",50962},
    {"I_GAL40_30",81661},
    {"I_INCR40_10",13284},
    {"I_INCR40_20",46402},
    {"I_INCR40_30",102306},
    {"I_LINE40_10",656},
    {"I_LINE40_20",2322},
    {"I_LINE40_30",5118},
    {"I_NL16_4",23625},
    {"I_NL16_8",57263},
    {"I_NL16_12",92580},
    {"I_NFL32_8",70127},
    {"I_NFL32_16",173493},
    {"I_NFL32_24",297068}
};

void ReadSolution(const string path, Solution& sol){

    cout << "read the file " << path << endl;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file " << path;
        return;
    }

    std::string line;
    bool StartReading = false;

    // Read the rest
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        // cout << line << endl;

        if (!StartReading && line.find("Round") != std::string::npos) {
            StartReading = true;
            continue;  // Skip the header line
        }
        if (!StartReading) {
            // You can parse the first block here if you need it
            continue;
        }

        int h, a, r;
        char comma; // to consume commas

        if (ss >> r >> comma >> h >> comma >> a) {
            SetValueCircleMethod(h,a,r,sol);
            sol.Orientation[h][r] = HA::H;
            sol.Orientation[a][r] = HA::A;
            // cout << h << " vs " << a << " in " << r << endl;
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

int RF_Miao(Input& in, const int TimeLimitRF){

	// This is not part of GurSolver since we cannot change the variable type once this is defined in Gurobi
	// Instead, we should create a new model, so it's best to define this function outside GurSolver and create seperate gur objects

    int dur = 0;
    Solution sol(in);
    GurSolver gur_relax(in);

	// relax x_ijr variables:
	bool relax_x = true;
	gur_relax.BuildMiaoFormulation(relax_x);

	const bool min_travel = true;
	const bool min_capacity_violations = false;
	gur_relax.AddObj(min_travel, min_capacity_violations);
    gur_relax.setTimeLimit(TimeLimitRF);
    gur_relax.TrackTimeBestSolution = true;

    // cout << "solve model" << endl;
	int lb = gur_relax.solve();
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

void SolveMiaoHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data){
    // Find initial solution with Vizing
    Solution sol(in);
    string path = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
    path += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
    if (!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
        ReadSolution(path, sol);
    }
    std::mt19937 gen(data.seed);
    MiaoAlgo miao_algo(MiaoMoves, MiaoWeights, in.getNrRounds(), gen);
    miao_algo.InitialSolutionGiven = true;
    miao_algo.setTimeLimit_meta(data.TimeLimit);
    miao_algo.SetTimeStamps(TimeStamps);
    miao_algo.solve(in, sol);
    sol.validate();

    const string FilePath = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
    const string config = to_string(data.seed) + ",MiaoAlgo," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);

    cout << "Save file as " << FilePath << endl;
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    miao_algo.SaveSolutionsTimeStamps(output_file);
    SaveSolution(output_file, sol);
    output_file.close();
    cout << "Close file" << endl;
}

void SolveHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data){
    // Find initial solution with Vizing
    cout << "Solve heuristic" << endl;
    Solution sol(in);
    sol.SetOneCostAllViolations(data.ConstrViolationCost);

    cout << "Solve Vizing" << endl;
    VizingConstruction(sol, data.seed);
    cout << "Found initial solution" << endl;

    assert(sol.validate());
    int obj = sol.ComputeTotalCost();
    cout << "Cost initial solution = " << obj << endl;
    cin.get();

    std::mt19937 gen(data.seed);
    int HL = data.HistoryLength;
    int TL = data.TimeLimit;
    if (data.HillClimbingFirst){
        HL = 1;
    }
    Heuristic_CM algo(data.Moves, data.InputWeights, gen, HL, obj);
    algo.setTimeLimit_meta(TL);
    algo.SetMaxIt(data.MaxIt);
    algo.SetTimeStamps(TimeStamps);

    if (data.HillClimbingFirst){
        algo.AddLowerBound(InstanceBound.at(in.getInstanceName()));
        algo.AddLowerBoundStoppingCriterion(data.LowerBoundGap);
        algo.solve(in, sol);

        algo.SaveBestSolution(sol);
        sol.validate();

        obj = sol.ComputeTotalCost();
        cout << "Total cost after hill climbing = " << sol.ComputeTotalCost() << endl;
        algo.SetHistoryLength(data.HistoryLength);
        algo.InitializeHistoricValues(obj);
        algo.AddLowerBoundStoppingCriterion(1.0);
    }
    algo.solve(in, sol);

    int NrViolations = 0;
    if (data.TTP){
        sol.SetOneCostAllViolations(1);
        NrViolations = sol.ComputeTotalCostTTPViolations();
        if (NrViolations > 0){
            cout << "Solution not feasible, nr of violations = " << NrViolations << endl;
            cout << "Start hill climbing" << endl;
            // Hill climbing
            sol.SetOneCostAllViolations(100000);
            obj = sol.ComputeTotalCost();
            algo.SetHistoryLength(1);
            algo.setTimeLimit_meta(2*data.TimeLimit);
            algo.solve(in, sol);
            sol.SetOneCostAllViolations(1);
            NrViolations = sol.ComputeTotalCostTTPViolations();
            cout << "Final nr of violations = " << NrViolations << endl;
            sol.SetOneCostAllViolations(100000);
        }
    }

    algo.SaveBestSolution(sol);
    sol.validate();
    cout << "Final cost = " << sol.ComputeTotalCost() << endl;
    if (data.TTP){
        cout << "Travel cost = " << sol.ComputeTravelCostTTP() << ", Cost violations = " << 10000 << " x " << NrViolations << endl;
    }

    string FilePath; 
    if (data.OutputFolder.empty()){
        FilePath = FolderPath + "Results" + std::string(PATHSEP) + "Heuristic";
    }
    else{
        FilePath = data.OutputFolder;
    }
    FilePath += std::string(PATHSEP) + sol.getInstanceName();
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
    const bool min_travel = true, min_cap = false;
    if (in.getSetting() == Setting::CM){
        gur.build_base(HA, relax_x);
        gur.AddObjGeneralCosts();
    }
    else if (in.getSetting() == Setting::TTP){
        // gur.iTTP();
        gur.iTTP_TripModel();
        gur.Fix_x(sol);
    }
    else{
        // IP without patterns:
        string path = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        path += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        if (!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
            ReadSolution(path, sol);
        }
        const bool relax_x = false;
        gur.build_all(HA, relax_x);
        gur.setBoundCapacityViolations();
        gur.AddObj(min_travel, min_cap);
        if (!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
            gur.WarmStart(sol);
        }
        // gur.Fix_x(sol);
    }
    gur.setTimeLimit(data.TimeLimit);
    gur.SetTimeStamps(TimeStamps);
    gur.solve();
    gur.SaveSolution(sol);
    // Save solution in file:
    if (in.getSetting() == Setting::Miao && min_cap){
        string FilePathVcr = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        FilePathVcr += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        std::ofstream output_file_Vcr(FilePathVcr);
        output_file_Vcr << "Instance,CapacitySetting,MaxNrBreaks,Vcr\n";
        output_file_Vcr << data.Instance << "," << data.CapacitySetting << "," << data.MaxNrBreaks << "," << gur.getBestObjValue() << "\n";
        SaveSolution(output_file_Vcr, sol);
        output_file_Vcr.close();
        cout << "Save " << gur.getBestObjValue() << " in file " << FilePathVcr << endl;
        return;
    } 
    sol.validate();

    string FilePath;
    string config;
    
    if (in.getSetting() == Setting::TTP){
        FilePath = FolderPath + "Results" + std::string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + "_" + to_string(in.getNrRounds()) + ".txt";
        config = to_string(data.seed) + ",IP," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
    }
    else if (in.getSetting() == Setting::Miao){
        FilePath = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        config = to_string(data.seed) + ",IP," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);
    }
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

void TestCostMinimization(InputData& data){

    vector<int>TimeStamps;
    int TimeStamp = 0;
    int Incrementor = 30;
    while (TimeStamp <= 2*data.TimeLimit){ // At most we do 2x the TimeLimit (because if infeasible we also do Hill Climbing)
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
        bool SetMaxNrBreaks = true;
        if (data.MaxNrBreaks > 3){
            SetMaxNrBreaks = false;
        }
        in.setHAP_requirements(true, false, false, SetMaxNrBreaks, data.MaxNrBreaks);
        in.SRR = true;
        if (!data.ConstantCapacity){
            in.setMiaoHAPSetting(data.CapacitySetting);
        }
        in.setAllowedNrCapacityViolations1RR(data);
    }

    if (data.TTP && !data.HistoryLengthProvided){
        data.HistoryLength = InstanceHL.at(in.getInstanceName()); 
    }
    
    if (data.Heuristic && !data.RunMiaoAlgo && !data.RunMiaoRF){
        SolveHeuristic(in,TimeStamps,folder_path,data);
    }
    else if (data.RunMiaoAlgo){
        SolveMiaoHeuristic(in,TimeStamps,folder_path,data);
    }
    else if (data.RunMiaoRF){
        RF_Miao(in, data.TimeLimit);
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
    int LB = 0;
    int UB = 0;
    double gap = 0;
    gur.setTimeLimit(TimeLimit); 
    // This assumes that a bound is known for the corresponding constant distance ttp instance
    gur.BoundTTP_AllTeams(InstanceBound.at("I_CON" + std::to_string(in.getNrTeams()) + "_" + std::to_string(in.getNrRounds())));
    gur.solve();
    LB = gur.getBestBound();
    UB = gur.getBestObjValue();
    gap = gur.getMipGap();


    /*
    for (int t = 0; t < in.getNrTeams(); ++t){
        gur.BoundTTP(t);
        sum += gur.solve();
    }
    */

    cout << "LB for instance " << Instance << " with " << NrRoundsTTP << " = " << LB << ", UB = " << UB << ", gap = " << gap << endl;

    output_file << Instance << "," << LB << "," << UB << "," << gap << "," << NrRoundsTTP << "\n";
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
