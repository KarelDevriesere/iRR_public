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

const unordered_map<string,int>ConSolutions = {
    {"I_CON16_4",48}, 		// Via Benders -> optimal values, see map Code_Benders/Best
    {"I_CON16_8",96}, 		// Via Benders
    {"I_CON16_12",132}, 	// Via Benders
    {"I_CON24_6",96}, 		// Via Benders
    {"I_CON24_12",192}, 	// Via Benders
    {"I_CON24_18",294}, 	// Via Benders, running on HPC
    {"I_CON32_8",192}, 		// Via Benders
    {"I_CON32_16",352}, 	// Via Benders
    {"I_CON32_24",520}, 	// Via Benders, running on HPC
    {"I_CON40_10",280}, 	// Via Benders
    {"I_CON40_20",560},     // Via Benders
    {"I_CON40_30",810} 	// Via Benders, running on HPC
};

void ReadSolutionXML(const string path, Solution& sol){

    cout << "read the file " << path << endl;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file " << path;
        return;
    }


    std::string line;

    auto getAttr = [](const std::string& s, const std::string& key) -> int {
        size_t pos = s.find(key + "=\"");
        if (pos == std::string::npos) return -1;
        pos += key.size() + 2;
        size_t end = s.find('"', pos);
        return std::stoi(s.substr(pos, end - pos));
    };

    while (std::getline(file, line)) {

        if (line.find("ScheduledMatch") == std::string::npos)
            continue;

        int h = getAttr(line, "home");
        int a = getAttr(line, "away");
        int r = getAttr(line, "slot");

        SetValueCircleMethod(h,a,r,sol);
        sol.Orientation[h][r] = HA::H;
        sol.Orientation[a][r] = HA::A;
    }

    file.close();
    sol.validate();
}

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
    else if (data.Hockey){
        folder_path = "Instances" + std::string(PATHSEP) + "Hockey"  + std::string(PATHSEP);
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

void SolveLeagueByLeague(Input& in, const InputData& data, const bool ComputeTravelBound){
	const bool HA = true;
	const bool relax_x = false;
	bool min_travel = false;
	bool min_cap = true;
    if (ComputeTravelBound){
        bool min_travel = true;
	    bool min_cap = false;
    }
    Solution sol(in);
    // sort the leagues by size
    vector<pair<int,int>>LeagueSize;
    for (int l = 0; l < sol.getNrLeagues(); ++l){
        LeagueSize.emplace_back(l, (int)sol.getNrTeamsLeague(l));
    }

    // Sorting leagues does not seem to be better!!!
    /*
    std::sort(LeagueSize.begin(), LeagueSize.end(),
              [](const auto &a, const auto &b) {
                  return a.second > b.second;  // descending by value
              });
    */

    // TODO: Vcr of hockey i03 and i06!!!
    // Next: test algo
    // Analyze results Miao TTP (effect os eeletcing only subset of patterns)
	for (auto& [l, league_size]: LeagueSize){
        GurSolver gursol(in);
        cout << "build base " << l << endl;
		gursol.build_base_league(HA, relax_x, l);
        if (!ComputeTravelBound){
            gursol.build_capacity_constraint_league(sol,l);
            gursol.AddObj(min_travel, min_cap);
        }
        else{
            gursol.AddObjMinTravelLeague(l);
        }
        cout << "Solve league " << l << endl;

        gursol.solve();
        gursol.SaveSolutionLeague(sol, l);
	}
    sol.validate();
    sol.CostCapacityViol = 1;
    if (ComputeTravelBound){
        cout << "Total cost = " << sol.ComputeTravelCost() << endl;
    }
    else{
        cout << "Total cost = " << sol.ComputeTotalHACost() << endl;
    }
    if (ComputeTravelBound){
        string FilePathBound = "Instances" + string(PATHSEP);
        FilePathBound += "Hockey" + string(PATHSEP) + "Bounds" + string(PATHSEP);
        FilePathBound += data.Instance + ".txt";
        std::ofstream output_file_bound(FilePathBound);
        output_file_bound << "Instance,Bound\n";
        output_file_bound << data.Instance << "," << sol.ComputeTravelCost() << "\n";
        output_file_bound.close();
        cout << "Save " << sol.ComputeTravelCost() << " in file " << FilePathBound << endl;
    }
    else{
        string FilePathVcr = "Instances" + string(PATHSEP);
        FilePathVcr += "Hockey" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        FilePathVcr += data.Instance + ".txt";
        std::ofstream output_file_Vcr(FilePathVcr);
        output_file_Vcr << "Instance,Vcr\n";
        output_file_Vcr << data.Instance << "," << sol.ComputeTotalHACost() << "\n";
        SaveSolution(output_file_Vcr, sol);
        output_file_Vcr.close();
        cout << "Save " << sol.ComputeTotalHACost() << " in file " << FilePathVcr << endl;
    }
    return;
}


void SolveMiaoHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data){
    // Find initial solution with Vizing
    Solution sol(in);
    std::mt19937 gen(data.seed);

    std::unordered_map<Move, string>moves;
    std::unordered_map<Move, double>weights;
    if (in.getSetting() == Setting::Miao || in.getSetting() == Setting::Hockey){
        moves = MiaoMoves;
        weights = MiaoWeights;
    }
    else{
        if (in.getInstanceName().find("I_CON") != std::string::npos){
            // random swap has no effect
            moves = MiaoMovesTTP_CON;
            weights = MiaoWeightsTTP_CON;
        }
        else{   
            moves = MiaoMovesTTP;
            weights = MiaoWeightsTTP;
        }  
    }

    MiaoAlgo miao_algo(moves, weights, in.getNrRounds(), gen);

    if (in.getSetting() == Setting::Miao){
        string path = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        path += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        if (/*!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)*/ true){
            ReadSolution(path, sol);
        }
        miao_algo.InitialSolutionGiven = true;
    }
    else if (in.getSetting() == Setting::Hockey){
        string path = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Vcr" + string(PATHSEP) + data.Instance + ".txt";
        ReadSolution(path, sol);
        miao_algo.InitialSolutionGiven = true;
    }
    else{
        if (in.getNrRounds() > in.getNrTeams() / 2){ // If this is the case, our strategy of using only 2 HAPs does not work!!
            VizingConstruction(sol, data.seed);
            miao_algo.InitialSolutionGiven = true;
        }
        else{
            // If not, assign only 2 HAPs to the teams: we know that this always results in a feasible solution!!
            miao_algo.InitialSolutionGiven = false;
        }
    }

    miao_algo.setTimeLimit_meta(data.TimeLimit);
    miao_algo.SetTimeStamps(TimeStamps);
    miao_algo.solve(in, sol);
    if (miao_algo.NrSuccesfullMatchings >= 1){
        sol.validate();
    }

    string FilePath;
    string config;

    if (in.getSetting() == Setting::Miao){
        FilePath = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + "_s" + to_string(data.seed) + ".txt";
        
        config = to_string(data.seed) + ",MiaoAlgo," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.seed) + ".txt";
        config = to_string(data.seed) + ",MiaoAlgo," + data.Instance;
    }
    else{
        FilePath = "Instances" + string(PATHSEP) + "TTP" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + std::string(PATHSEP) + sol.getInstanceName();
        if (data.PercentageHAPs < 99.99){
        FilePath += "_PercHAPs" + to_string(data.PercentageHAPs);
        } 
        FilePath += "_s" + to_string(data.seed) + ".txt";
        
        config = to_string(data.seed) + ",MiaoAlgo," + sol.getInstanceName() + "," + to_string(data.HistoryLength) + "," + to_string(data.PercentageHAPs);
    }
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    output_file << "NrSuccesfullMatchings," << miao_algo.NrSuccesfullMatchings << ",NrInfeasibleMatchings," << miao_algo.NrInfeasibleMatchings << "\n";
    if (miao_algo.NrSuccesfullMatchings >= 1 || miao_algo.InitialSolutionGiven){
        miao_algo.SaveSolutionsTimeStamps(output_file);
        SaveSolution(output_file, sol);

	    // Replace txt extension with XML
	    FilePath.replace(FilePath.size() - 4, 4, ".xml");
	    cout << "Save XML file as " << FilePath << endl;
	    std::ofstream output_fileXML(FilePath);
	    SaveSolutionXML(output_fileXML, sol);
    }
    output_file.close();
    cout << "Close file" << endl;
}

int ReturnBestSeed(string& path){
    array<int,10>seeds = {0,11,42,154,396,588,1217,2486,5003,10000};

    int BestValue = INT_MAX;
    int BestSeed = -1;
    string path_seed = "";

    for (int& seed: seeds){
        path_seed = path + "_s" + to_string(seed) + ".txt";
        // cout << "string path = " << path_seed << endl;

        std::ifstream file(path_seed);
        if (!file.is_open()) {
            std::cerr << "Could not open file\n";
            std::abort();
        }

        std::string line;
        int Value = -1;

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;

            // Read first token
            if (!std::getline(ss, token, ',')) continue;

            if (token == "Final") {
                // Read the value right next to "Final"
                if (std::getline(ss, token, ',')) {
                    Value = std::stoi(token);
                }
                break;
            }
        }
        if (Value == -1){
            cout << "Line with Final not found, abort" << endl;
            std::abort();
        }
        else{
            if (Value < BestValue){
                BestValue = Value;
                BestSeed = seed;
            }
        }
    }
    cout << "Best seed = " << BestSeed << " with value = " << BestValue << endl;
    assert(BestSeed > -1);
    return BestSeed;
}

void SolveHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data){
    // Find initial solution with Vizing
    cout << "Solve heuristic" << endl;
    Solution sol(in);
    sol.SetOneCostAllViolations(data.ConstrViolationCost);

    string path = "Instances" + string(PATHSEP);

    if (in.getSetting() == Setting::Miao){
        // string path = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        // path += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        /*
        string path = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + string(PATHSEP);
        string instance_full = data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks);
        path += instance_full + "_seed" + to_string(BestSeedsMiaoAlgo.at(instance_full)) + ".txt";
        if (!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
            ReadSolution(path, sol);
        }
        else{
            VizingConstruction(sol, data.seed); 
        }
        */
        path += "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks);
        // cout << "path = " << path << endl;
    }
    else if (in.getSetting() == Setting::Hockey){
        path += "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "MiaoAlgo" + string(PATHSEP) + data.Instance;
    }
    else{
        // cout << "Solve Vizing" << endl;
        VizingConstruction(sol, data.seed);

        /*
        // Start from best found solution by Miao's algorithm (with 100% of the HAPs)
        path += "TTP" + string(PATHSEP) + "Results" + string(PATHSEP);
        if (sol.getNrRounds() <= sol.getNrTeams()/2){
            path += "MiaoAlgo08_01_2026";
        }
        else{
            path += "MiaoAlgo";
        }
        path += string(PATHSEP) + sol.getInstanceName();
        */
    }

    /*
    cout << "path = " << path << endl;
    int BestSeed = ReturnBestSeed(path);

    string path_seed = path + "_s" + to_string(BestSeed) + ".txt";
    ReadSolution(path_seed, sol);
    */

    sol.validate();

    cout << "Found initial solution" << endl;

    assert(sol.validate());
    int obj = sol.ComputeTotalCost();
    cout << "Cost initial solution = " << obj << endl;

    std::mt19937 gen(data.seed);
    int HL = 1;
    if (data.HistoryLengthProvided){
        HL = data.HistoryLength;
    }
    int TL = data.TimeLimit;
    Heuristic_CM algo(data.Moves, data.InputWeights, gen, HL, obj);
    if (!data.HistoryLengthProvided){
        algo.MakeHistoryLengthDynamic();
    }
    algo.SetPerturbeIncrease(data.PerturbeIncrease);
    algo.setTimeLimit_meta(TL);
    algo.SetMaxIt(data.MaxIt);
    algo.SetTimeStamps(TimeStamps);
    algo.solve(in, sol);

    int NrViolations = 0;
    sol.SetOneCostAllViolations(1);
    if (data.TTP){
        NrViolations = sol.ComputeTotalCostTTPViolations();
    }
    else{
        NrViolations = sol.ComputeTotalCostMiaoHockey() - sol.ComputeTravelCost();
        cout << "TravelCost = " << sol.ComputeTravelCost() << endl;
        cout << "HA Cost = " << sol.ComputeTotalHACost() << endl;
        cout << "Eligible opponents cost = " << sol.ComputeCostNonEligibleOpponents() << endl;
    }
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
        if (data.TTP){
            NrViolations = sol.ComputeTotalCostTTPViolations();
        }
        else{
            NrViolations = sol.ComputeTotalCostMiaoHockey() - sol.ComputeTravelCost();
        }
        cout << "Final nr of violations = " << NrViolations << endl;
        sol.SetOneCostAllViolations(100000);
    }

    algo.SaveBestSolution(sol);
    sol.validate();
    cout << "Final cost = " << sol.ComputeTotalCost() << endl;
    if (data.TTP){
        cout << "Travel cost = " << sol.ComputeTravelCostTTP() << ", Cost violations = " << 10000 << " x " << NrViolations << endl;
    }

    string FilePath; 
    string config;
    if (in.getSetting() == Setting::TTP){
        if (data.OutputFolder.empty()){
            FilePath = FolderPath + "Results" + std::string(PATHSEP) + "Heuristic";
        }
        else{
            FilePath = data.OutputFolder;
        }
        FilePath += std::string(PATHSEP) + sol.getInstanceName();
        FilePath += "_s" + to_string(data.seed);
        if (data.HistoryLengthProvided){
            FilePath += "_HL" + to_string(data.HistoryLength);
        }
        FilePath += ".txt";
        config = to_string(data.seed);
        if (data.Base){
            config += ",BaseAlgo,";
        }
        else{
            config += ",Heuristic,";
        }
        config += to_string(data.MaxIt) + "," + to_string(data.TimeLimit) + "," + to_string(data.HistoryLength) + "," + to_string(data.ConstrViolationCost) + "," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
    }
    else if (in.getSetting() == Setting::Miao){
        cout << "Started from Miao solution but save in Heuristic!!!" << endl;
        FilePath = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "Heuristic" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + "_seed" + to_string(data.seed) + ".txt";
        config = to_string(data.seed) + ",Heuristic," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks) + "," + to_string(data.HistoryLength);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "Heuristic" + std::string(PATHSEP) + data.Instance + "_seed" + to_string(data.seed) + ".txt";
        config = to_string(data.seed) + ",Heuristic," + data.Instance + "," + to_string(data.HistoryLength);
    }

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
    cout << "gur" << endl;
    GurSolver gur(in);
    cout << "gur success" << endl;
    Solution sol(in);
    bool HA = true;
    bool relax_x = false;
    const bool min_travel = true, min_cap = false;
    if (in.getSetting() == Setting::TTP){
        bool TripModelHAP_Fixed = false;
        if (TripModelHAP_Fixed){
            // First, read best found solution so far by greedy matching
            // string path = "Code_Benders" + string(PATHSEP) + "BestNoLex" + string(PATHSEP) + "I_CON" + to_string(sol.getNrTeams()) + "_" + to_string(sol.getNrRounds()) + ".xml";
            // ReadSolutionXML(path, sol);
            // Start from best found solution by Miao's algorithm (with 100% of the HAPs)
            string path = "Instances" + string(PATHSEP) + "TTP" + string(PATHSEP) + "Results" + string(PATHSEP);
            path += "MiaoAlgo08_01_2026";
            path += string(PATHSEP) + sol.getInstanceName();

            cout << "path = " << path << endl;
            int BestSeed = ReturnBestSeed(path);

            string path_seed = path + "_s" + to_string(BestSeed) + ".txt";
            ReadSolution(path_seed, sol);

            cout << "Total travel time = " << sol.ComputeTravelCostTTP() << endl;
            cout << "Cost violations = " << sol.ComputeTotalCostTTPViolations() << endl;
            cout << "Total cost = " << sol.ComputeTotalCost() << endl;

            gur.iTTP_TripModel_HAP_fixed(sol);
            /*
            gur.iTTP();
            for (int t = 0; t < sol.getNrTeams(); ++t){
                gur.HapFixed[t] = true;
            }
            gur.FixHAP(sol);
            */
        }
        else{
            gur.iTTP();
            gur.AddMinTripLowerBoundiTTP();
            // gur.Min2Factor();
        }
        // gur.iTTP_TripModel();
        // gur.Fix_x(sol);
    }
    else{
        // IP without patterns:
        if (min_travel){

            string path = "Instances" + string(PATHSEP);
            if (in.getSetting() == Setting::Miao){
                path += "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
                path += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
                if (!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
                    ReadSolution(path, sol);
                }
            }
            else {
                path += "Hockey" + string(PATHSEP) + "Vcr" + string(PATHSEP) + data.Instance  +".txt";
                ReadSolution(path, sol);
            }
        }
        const bool relax_x = false;
        if (min_travel || (min_cap && in.getSetting() == Setting::Miao)){
            gur.build_all(HA, relax_x);
        }
        else{
            const bool TravelBound = false;
            SolveLeagueByLeague(in, data, TravelBound);
            return;
        }
        if (min_travel){
            gur.setBoundCapacityViolations();
        }
        if (min_travel || (min_cap && in.getSetting() == Setting::Miao)){
            gur.AddObj(min_travel, min_cap);
        }
        if (min_travel && !(in.getSetting() == Setting::Miao && data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
            gur.WarmStart(sol);
        }
        // gur.Fix_x(sol);
    }
    gur.setTimeLimit(data.TimeLimit);
    gur.SetTimeStamps(TimeStamps);
    gur.solve();
    cin.get();
    gur.PrintVariables();
    cin.get();
    cout << "save solution" << endl;
    gur.SaveSolution(sol);
    cout << "test whether solution is feasible" << endl;
    sol.validate();
    cout << "Travel cost = " << sol.ComputeTotalCostTTP() << endl;
    GurSolver gur_validate(in);
    gur_validate.iTTP();
    gur_validate.Fix_x(sol);
    gur_validate.solve();
    cout << "feasible!!" << endl;

    // Save solution in file:
    if (in.getSetting() == Setting::Miao || (in.getSetting() == Setting::Hockey && min_cap)){
        string FilePathVcr = "Instances" + string(PATHSEP);
        if (in.getSetting() == Setting::Miao){
            FilePathVcr += "Miao" + string(PATHSEP) + "Vcr" + string(PATHSEP);
            FilePathVcr += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        }
        else{
            FilePathVcr += "Hockey" + string(PATHSEP) + "Vcr" + string(PATHSEP);
            FilePathVcr += data.Instance + ".txt";
        }
        std::ofstream output_file_Vcr(FilePathVcr);
        output_file_Vcr << "Instance,CapacitySetting,MaxNrBreaks,Vcr\n";
        output_file_Vcr << data.Instance << "," << data.CapacitySetting << "," << data.MaxNrBreaks << "," << gur.getBestObjValue() << "\n";
        SaveSolution(output_file_Vcr, sol);
        output_file_Vcr.close();
        cout << "Save " << gur.getBestObjValue() << " in file " << FilePathVcr << endl;
        return;
    } 
    cout << "validate" << endl;
    sol.validate();

    string FilePath;
    string config;
    
    if (in.getSetting() == Setting::TTP){
        FilePath = FolderPath + "Results" + std::string(PATHSEP) + "IP_TripModel" + std::string(PATHSEP) + sol.getInstanceName() + ".txt";
        config = to_string(data.seed) + ",IP_TripModel," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
    }
    else if (in.getSetting() == Setting::Miao){
        FilePath = "Instances" + string(PATHSEP) + "Miao" + string(PATHSEP) + "Results" + string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        config = to_string(data.seed) + ",IP," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + ".txt";
        config = to_string(data.seed) + ",IP," + data.Instance;
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
    if (data.TTP && !in.read_TTP(file_path)){
        cout << "could not read TTP path " << file_path << endl;
        return;
    }
    else if ((data.Miao || data.Hockey) && !in.read_Miao_Hockey(file_path, data.Miao)){
        cout << "could not read Miao or Hockey path " << file_path << endl;
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
    in.SRR = true;
    if (!data.Miao && !data.Hockey){
        in.setHAP_requirements(false, false, false, false, in.getNrRounds());
    }
    else if(data.Hockey){
        in.setHAP_requirements(true, false, false, false, in.getNrRounds());
        in.setAllowedNrCapacityViolations1RR(data);
        in.read_HAPs();
    }
    else{
        bool SetMaxNrBreaks = true;
        if (data.MaxNrBreaks > 3){
            SetMaxNrBreaks = false;
        }
        in.setHAP_requirements(true, false, false, SetMaxNrBreaks, data.MaxNrBreaks);
        if (!data.ConstantCapacity){
            in.setMiaoHAPSetting(data.CapacitySetting);
        }
        in.setAllowedNrCapacityViolations1RR(data);
        in.read_HAPs();
    }

    if (data.TTP && in.getNrRounds() >= 4 /*&& data.RunMiaoAlgo*/){ // Do this also for IP!!!
        in.read_HAPs();
        if (data.PercentageHAPs < 100){
            double NrPromisingHAPs = in.getNrHAPs()*((double)data.PercentageHAPs/100.0);
            if (NrPromisingHAPs < 2.0){
                NrPromisingHAPs = 2;
            }
            in.DeleteNonPromisingHAPsTTP((int)NrPromisingHAPs);
        }
    }

    /*
    bool TravelBound = true;
    SolveLeagueByLeague(in, data, TravelBound);
    return;
    */
    
    if (data.Heuristic && !data.RunMiaoAlgo && !data.RunMiaoRF){
        SolveHeuristic(in,TimeStamps,folder_path,data);
    }
    else if (data.RunMiaoAlgo){
        SolveMiaoHeuristic(in,TimeStamps,folder_path,data);
    }
    else{
        cout << "Solve IP" << endl;
        SolveIP(in,TimeStamps,folder_path,data);
    }
}

void BoundTTP(const int TimeLimit, const string Instance, const int NrRoundsTTP, std::ofstream& output_file, const bool addMinTripConstraint){

    Input in;
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

    GurSolver gur_relax(in);
    gur_relax.BoundTTP_AllTeams(addMinTripConstraint, ConSolutions.at("I_CON" + std::to_string(in.getNrTeams()) + "_" + std::to_string(in.getNrRounds())));
    gur_relax.solve();
    cin.get();

    /*
    // Generate initial solution with Vizing:
    VizingConstruction(sol, 0);
    cout << "travel distance = " << sol.ComputeTravelCostTTP() << endl;
    gur.InitializeMasterProblem(sol);
    int it = 0;
    while (true){
        gur.solve();
        if (it++ % 1 == 0){
            cout << "Travel time = " << gur.getBestObjValue() << endl;
        }
        if (!gur.getDualVariablesiTTPBounds()){
            break;
        }
    }
    cout << "CONVERGED" << endl;
    // Solve problem with all variables as binary
    gur.ConvertVariablesToBinary();
    gur.solve();
    cin.get();
    */

    gur.BoundTTP_AllTeams(addMinTripConstraint, ConSolutions.at("I_CON" + std::to_string(in.getNrTeams()) + "_" + std::to_string(in.getNrRounds())));
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
    cin.get();

    output_file << Instance << "," << LB << "," << UB << "," << gap << "," << NrRoundsTTP << "\n";
}

void BoundsTTP_OneInstance(InputData& data){
    Input in;
    if (!in.read_TTP(data.Instance)){
        cout << "could not read " << data.Instance << endl;
        return;
    }
    string OutputFilePath = "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP) + "Bounds" + std::string(PATHSEP);
    if (data.addMinTripConstraint){
        OutputFilePath += "DLB_MinTrip_";
    }
    else{
        OutputFilePath += "DLB_";
    }
    OutputFilePath += in.getInstanceName() + ".txt";
    cout << "Save file as " << OutputFilePath << endl;
    std::ofstream output_file(OutputFilePath);
    BoundTTP(data.TimeLimit, data.Instance, data.NrRounds, output_file, data.addMinTripConstraint);
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
            BoundTTP(data.TimeLimit, Instance, NrRoundsTTP, output_file, data.addMinTripConstraint);
        }
    }

    output_file.close();
}
