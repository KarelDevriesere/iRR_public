#include "AlgoSelection.h"

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

        sol.SetColorMatch(h,a,r);
        sol.Orientation[h][r] = HA::H;
        sol.Orientation[a][r] = HA::A;
    }

    file.close();
    sol.validate();
}

void ReadSolution(const string path, Solution& sol){

#if PRINT
    cout << "read the file " << path << endl;
#endif
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
            sol.SetColorMatch(h,a,r);
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
        folder_path = "Instances" + std::string(PATHSEP) + "Football"  + std::string(PATHSEP);
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

	for (auto& [l, league_size]: LeagueSize){
        GurSolver gursol(in);
        // cout << "build base " << l << endl;
		gursol.build_base_league(HA, relax_x, l);
        if (!ComputeTravelBound){
            gursol.build_capacity_constraint_league(sol,l);
            gursol.AddObj(min_travel, min_cap);
        }
        else{
            gursol.AddObjMinTravelLeague(l);
        }
        // cout << "Solve league " << l << endl;

        gursol.solve();
        gursol.SaveSolutionLeague(sol, l);
	}
    sol.validate();
    sol.CostCapacityViol = 1;
#ifdef PRINT
#if PRINT == 1
    if (ComputeTravelBound){
        cout << "Total cost = " << sol.ComputeTravelCost() << endl;
    }
    else{
        cout << "Total cost = " << sol.ComputeTotalHACost() << endl;
    }
#endif
#endif
    if (ComputeTravelBound){
        string FilePathBound = "Instances" + string(PATHSEP);
        FilePathBound += "Hockey" + string(PATHSEP) + "Bounds" + string(PATHSEP);
        FilePathBound += data.Instance + ".txt";
        std::ofstream output_file_bound(FilePathBound);
        output_file_bound << "Instance,Bound\n";
        output_file_bound << data.Instance << "," << sol.ComputeTravelCost() << "\n";
        output_file_bound.close();
#ifdef PRINT
#if PRINT == 1
        cout << "Save " << sol.ComputeTravelCost() << " in file " << FilePathBound << endl;
#endif
#endif
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
#ifdef PRINT
#if PRINT == 1
        cout << "Save " << sol.ComputeTotalHACost() << " in file " << FilePathVcr << endl;
#endif
#endif
    }
    return;
}

void SolveGreedyMatching(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data, const ParameterValues& param){
    // Find initial solution with Vizing
    Solution sol(in);
    std::mt19937 gen(data.seed);

    std::unordered_map<Move, string>moves;
    std::unordered_map<Move, double>weights;
    if (in.getSetting() == Setting::Football || in.getSetting() == Setting::Hockey){
        moves = GreedyMatchingMoves;
        weights = GreedyMatchingWeights;
    }
    else{
        moves = GreedyMatchingMovesiTTP;
        weights = GreedyMatchingWeightsiTTP;
    }

    int obj = sol.ComputeTotalCost();
    auto MetaStrategy = MetaFactory<Move>::create(MetaHeuristic::HC, obj, param, moves, weights, weights, gen);
    GreedyMatching GM(moves, weights, in.getNrRounds(), gen, sol, std::move(MetaStrategy));

    if (in.getSetting() == Setting::Football){
        string path = "Instances" + string(PATHSEP) + "Football" + string(PATHSEP) + "Vcr" + string(PATHSEP);
        path += data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        if (/*!(data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)*/ true){
            ReadSolution(path, sol);
        }
        GM.InitialSolutionGiven = true;
    }
    else if (in.getSetting() == Setting::Hockey){
        string path = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Vcr" + string(PATHSEP) + data.Instance + ".txt";
        ReadSolution(path, sol);
        GM.InitialSolutionGiven = true;
    }
    else{
        if (in.getNrRounds() > in.getNrTeams() / 2){ // If this is the case, our strategy of using only 2 HAPs does not work!!
            VizingConstruction(sol, data.seed);
            GM.InitialSolutionGiven = true;
        }
        else{
            // If not, assign only 2 HAPs to the teams: we know that this always results in a feasible solution!!
            GM.InitialSolutionGiven = false;
        }
        if (data.GM_Constructive){
            GM.InitialOnly = true;
        }
    }

    GM.solve(in, sol);
    if (GM.NrSuccesfullMatchings >= 1){
        sol.validate();
    }

    string FilePath;
    string config;

    if (in.getSetting() == Setting::Football){
        FilePath = "Instances" + string(PATHSEP) + "Football" + string(PATHSEP) + "Results" + string(PATHSEP) + "GM" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + "_s" + to_string(data.seed) + ".txt";
        
        config = to_string(data.seed) + ",MiaoAlgo," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "GM" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.seed) + ".txt";
        config = to_string(data.seed) + ",MiaoAlgo," + data.Instance;
    }
    else{
        FilePath = "Instances" + string(PATHSEP) + "TTP" + string(PATHSEP) + "Results" + string(PATHSEP) + "GM";
        if (GM.InitialOnly){
            FilePath += "Constructive";
        }
        FilePath += std::string(PATHSEP) + sol.getInstanceName();
        FilePath += "_s" + to_string(data.seed) + ".txt";
        
        config = to_string(data.seed) + ",MiaoAlgo," + sol.getInstanceName();
    }
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    output_file << "NrSuccesfullMatchings," << GM.NrSuccesfullMatchings << ",NrInfeasibleMatchings," << GM.NrInfeasibleMatchings << "\n";
    if (GM.NrSuccesfullMatchings >= 1 || GM.InitialSolutionGiven){
        cout << "save time stamps" << endl;
        GM.saveTimeStamps(output_file);
        cout << "time stamps saved" << endl;
        SaveSolution(output_file, sol);
        cout << "solutions saved" << endl;

	    // Replace txt extension with XML
        FilePath.replace(FilePath.size() - 4, 4, ".xml");
#ifdef PRINT
#if PRINT == 1
	    cout << "Save XML file as " << FilePath << endl;
#endif
#endif
	    std::ofstream output_fileXML(FilePath);
	    SaveSolutionXML(output_fileXML, sol);
    }
    output_file.close();
    // cout << "Close file" << endl;
}

int ReturnBestSeed(string& path){
    // array<int,10>seeds = {0,11,42,154,396,588,1217,2486,5003,10000};
    array<int,1>seeds = {0};

    int BestValue = INT_MAX;
    int BestSeed = -1;
    string path_seed = "";

    for (int& seed: seeds){
        path_seed = path + "_s" + to_string(seed) + ".txt";
        // cout << "string path = " << path_seed << endl;

        std::ifstream file(path_seed);
        if (!file.is_open()) {
            std::cerr << "Could not open file " << path_seed << " \n";
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
    // cout << "Best seed = " << BestSeed << " with value = " << BestValue << endl;
    assert(BestSeed > -1);
    return BestSeed;
}


void InitializeSol(Solution& sol, const InputData& data){

    string path = "Instances" + string(PATHSEP);

    if (sol.getSetting() == Setting::Football){
        path += "Football" + string(PATHSEP) + "Results" + string(PATHSEP) + "GM" + string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks);
        // cout << "path = " << path << endl;
        int BestSeed = ReturnBestSeed(path);
        string path_seed = path + "_s" + to_string(BestSeed) + ".txt";
        ReadSolution(path_seed, sol);
    }
    else if (sol.getSetting() == Setting::Hockey){
        path += "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "GM" + string(PATHSEP) + data.Instance;
        int BestSeed = ReturnBestSeed(path);
        string path_seed = path + "_s" + to_string(BestSeed) + ".txt";
        ReadSolution(path_seed, sol);
    }
    else{
        // cout << "Solve Vizing" << endl;
        VizingConstruction(sol, data.seed);

        /*
        // Start from best found solution by Greedy Matching (with 100% of the HAPs)
        path += "TTP" + string(PATHSEP) + "Results" + string(PATHSEP) + "GM" + string(PATHSEP) + sol.getInstanceName();
        */
    }
   
    sol.validate();

    // cout << "Found initial solution" << endl;

    assert(sol.validate());
    return;
}

string FolderHeuristic(const Input& in, const InputData& data, const ParameterValues& param){
    string FolderPath = "Instances" + string(PATHSEP);
    if (in.getSetting() == Setting::TTP){
        FolderPath += "TTP";
    }
    else if (in.getSetting() == Setting::Football){
        FolderPath += "Football";
    }
    else{
        FolderPath += "Hockey";
    }
    FolderPath += string(PATHSEP) + "Results" + string(PATHSEP) + "Heuristic" + string(PATHSEP);;
    // contains requires C++ 20
    if (data.InputWeights.contains(Move::TS) && data.InputWeights.contains(Move::PRS) && data.InputWeights.contains(Move::C)){
        if ((data.InputWeights.contains(Move::iPTS_Random_PR) || data.InputWeights.contains(Move::iPTS_Random_PR_CR)) && data.InputWeights.contains(Move::Random_M_Random_PR)){
            FolderPath += "All";
        }
        else {
            FolderPath += "Base";
        }
    }
    else if (data.InputWeights.contains(Move::iPTS_Random_PR)){
        FolderPath += "iPTS_Random_PR";
    }
    else if (data.InputWeights.contains(Move::iPTS_Random_PR_CR)){
        FolderPath += "iPTS_Random_PR_CR";
    }
    else if (data.InputWeights.contains(Move::Random_M_Random_PR)){
        FolderPath += "Random_M_Random_PR";
    }
    else if (data.InputWeights.contains(Move::Random_BM) && data.InputWeights.contains(Move::C)){
        FolderPath += "Random_BM_C";
    }
    else if (data.InputWeights.contains(Move::MinCost_BM) && data.InputWeights.contains(Move::C)){
        FolderPath += "MinCost_BM_C";
    }
    else{
        FolderPath += "Other";
    }
    if (param.MAB){
        FolderPath += "_MAB";
    }
    // Check if FolderPath exists
    if (!std::filesystem::exists(FolderPath)) {
        // if not, construct it
        bool success = std::filesystem::create_directories(FolderPath); 
        
        /*
        if (success) {
            std::cout << "Successfully created folder: " << FolderPath << "\n";
        } else {
            std::cerr << "Failed to create folder: " << FolderPath << "\n";
        }
            */
    }
    return FolderPath;
}

void SolveFixAndOptimize(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data, const ParameterValues& param){
    Solution sol(in);
    std::mt19937 gen(data.seed);

    InitializeSol(sol, data);

    int obj = sol.ComputeTotalCost();
    // cout << "Cost initial solution = " << obj << endl;

    auto MetaStrategy = MetaFactory<FO_move>::create(MetaHeuristic::HC, obj, param, FixAndOptimizeMoves, FixAndOptimizeWeights, FixAndOptimizeWeights, gen);
    FO FO(in, std::move(MetaStrategy));
    FO.InitializeModel(sol, data);
    FO.solve(in, sol, data);

    // cout << "Final objective = " << sol.ComputeTotalCost() << endl;

    string FilePath;
    string config;
    
    if (in.getSetting() == Setting::TTP){
        FilePath = FolderPath + "Results" + std::string(PATHSEP);
        if (param.MAB){
            FilePath += "FO_MAB" ;
        }
        else{
            FilePath += "FO";
        }
        FilePath += std::string(PATHSEP) + sol.getInstanceName() + ".txt";
        config = to_string(data.seed) + ",FO," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
    }
    else if (in.getSetting() == Setting::Football){
        FilePath = "Instances" + string(PATHSEP) + "Football" + string(PATHSEP) + "Results" + string(PATHSEP);
        if (param.MAB){
            FilePath += "FO_MAB";
        }
        else{
            FilePath += "FO";
        } 
        FilePath += std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        config = to_string(data.seed) + ",FO," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP);
        if (param.MAB){
            FilePath += "FO_MAB";
        }
        else{
            FilePath += "FO";
        }
        FilePath += std::string(PATHSEP) + data.Instance + ".txt";
        config = to_string(data.seed) + ",IP," + data.Instance;
    }
#ifdef PRINT
#if PRINT == 1  
    cout << "Save file as " << FilePath << endl;
#endif 
#endif
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    FO.SaveSolutionsTimeStamps(output_file);
    SaveSolution(output_file, sol);
    if (in.getSetting() == Setting::TTP){
#ifdef PRINT
#if PRINT == 1
        output_file << "Travel cost = " << sol.ComputeTravelCostTTP() << "\n";
        output_file << "TTP violations cost = " << sol.ComputeTotalCostTTPViolations() << "\n";
#endif 
#endif
    }
    output_file.close();

    // Replace txt extension with XML
    FilePath.replace(FilePath.size() - 4, 4, ".xml");
#ifdef PRINT
#if PRINT == 1
    cout << "Save XML file as " << FilePath << endl;
#endif 
#endif
    std::ofstream output_fileXML(FilePath);
    SaveSolutionXML(output_fileXML, sol);

    output_file.close();
    cout << sol.ComputeTotalCost() << endl; // for irace: print final solution

    return;
}

void SolveHeuristic(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data, const ParameterValues& param){
    // Find initial solution with Vizing
    // cout << "Solve heuristic" << endl;
    Solution sol(in);
    sol.SetOneCostAllViolations(data.ConstrViolationCost); // this makes it impossible to select moves that violate any of the constraints
    if (data.ConstraintViolationAllowed){
        sol.ConstraintViolationAllowed = true;
    }

    InitializeSol(sol, data);

    int obj = sol.ComputeTotalCost();
    // cout << "Cost initial solution = " << obj << endl;

    std::mt19937 gen(data.seed);
    MetaHeuristic M;
    if (param.SA){
        M = MetaHeuristic::SA;
    }
    else if (param.HC){
        M = MetaHeuristic::HC;
    }
    else if (param.ILS){
        M = MetaHeuristic::ILS;
    }
    else if (param.VNS){
        M = MetaHeuristic::VNS;
    }
    else{
        M = MetaHeuristic::LAHC; // default is (adaptive) LAHC
    }
    auto MetaStrategy = MetaFactory<Move>::create(M, obj, param, data.Moves, data.InputWeights, data.InputWeightsPerturb, gen);
    Heuristic myHeuristic(sol, std::move(MetaStrategy));
    myHeuristic.solve(in, sol);

    int NrViolations = 0;
    sol.SetOneCostAllViolations(1);
    if (data.TTP){
        NrViolations = sol.ComputeTotalCostTTPViolations();
    }
    else{
        NrViolations = sol.ComputeTotalCostYSTP() - sol.ComputeTravelCost();
#ifdef PRINT
#if PRINT == 1
        cout << "TravelCost = " << sol.ComputeTravelCost() << endl;
        cout << "HA Cost = " << sol.ComputeTotalHACost() << endl;
        cout << "Eligible opponents cost = " << sol.ComputeCostNonEligibleOpponents() << endl;
#endif 
#endif
    }
#ifdef PRINT
#if PRINT == 1
    cout << "Final cost = " << sol.ComputeTotalCost() << endl;
#endif 
#endif
    if (data.TTP){
#ifdef PRINT
#if PRINT == 1
        cout << "Travel cost = " << sol.ComputeTravelCostTTP() << ", Cost violations = " << sol.CostTTPViolation << " x " << NrViolations << endl;
#endif
#endif
    }

    string FilePath;
    if (data.OutputFolder.empty()){
        FilePath = FolderHeuristic(in, data, param);
    }
    else{
        FilePath = data.OutputFolder;
    } 
    // cout << "FilePath so far = " << FilePath << endl;
    string config;
    if (in.getSetting() == Setting::TTP){
        FilePath += std::string(PATHSEP) + sol.getInstanceName();
        FilePath += "_s" + to_string(data.seed);
        if (param.HistoryLengthProvided){
            FilePath += "_HL" + to_string(param.HistoryLength);
        }
        FilePath += ".txt";
        config = to_string(data.seed);
        config += ",Heuristic,";
        config += to_string(param.MaxIt) + "," + to_string(param.TIME_LIMIT) + "," + to_string(param.HistoryLength) + "," + to_string(param.PerturbeIncrease) + "," + to_string(data.ConstrViolationCost) + "," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
    }
    else if (in.getSetting() == Setting::Football){
        FilePath += std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + "_seed" + to_string(data.seed) + ".txt";
        config = to_string(data.seed) + ",Heuristic," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks) + "," + to_string(param.HistoryLength);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath += std::string(PATHSEP)  + data.Instance + "_seed" + to_string(data.seed) + ".txt";
        config = to_string(data.seed) + ",Heuristic," + data.Instance + "," + to_string(param.HistoryLength);
    }

    // cout << "Save file as " << FilePath << endl;
    std::ofstream output_file(FilePath);
    output_file << config << "\n";
    myHeuristic.saveTimeStamps(output_file);
    if (data.TTP){
        output_file << "NrViolations," << NrViolations << "\n";
    }
    SaveSolution(output_file, sol);

    // Replace txt extension with XML
    FilePath.replace(FilePath.size() - 4, 4, ".xml");
#ifdef PRINT
#if PRINT == 1
    cout << "Save XML file as " << FilePath << endl;
#endif 
#endif
    std::ofstream output_fileXML(FilePath);
    SaveSolutionXML(output_fileXML, sol);

    output_file.close();
    cout << sol.ComputeTotalCost() << endl; // for irace: print final solution
    return;
}

void SolveIP(Input& in, vector<int>& TimeStamps, const string FolderPath, const InputData& data, const ParameterValues& param){
    GurSolver gur(in);
    Solution sol(in);
    bool HA = true;
    bool relax_x = false;
    const bool min_travel = true, min_cap = false;
    if (in.getSetting() == Setting::TTP){
        if (data.TripModelHAP_Fixed){
            // First, read best found solution so far by greedy matching
            // string path = "Code_Benders" + string(PATHSEP) + "BestNoLex" + string(PATHSEP) + "I_CON" + to_string(sol.getNrTeams()) + "_" + to_string(sol.getNrRounds()) + ".xml";
            // ReadSolutionXML(path, sol);
            // Start from best found solution by Greedy Matching (with 100% of the HAPs)
            string path = "Instances" + string(PATHSEP) + "TTP" + string(PATHSEP) + "Results" + string(PATHSEP);
            path += "GM";
            path += string(PATHSEP) + sol.getInstanceName();

            // cout << "path = " << path << endl;
            int BestSeed = ReturnBestSeed(path);

            string path_seed = path + "_s" + to_string(BestSeed) + ".txt";
            ReadSolution(path_seed, sol);
#ifdef PRINT
#if PRINT == 1
            cout << "Total travel time = " << sol.ComputeTravelCostTTP() << endl;
            cout << "Cost violations = " << sol.ComputeTotalCostTTPViolations() << endl;
            cout << "Total cost = " << sol.ComputeTotalCost() << endl;
#endif 
#endif 
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
            // gur.iTTP();
            // gur.AddMinTripLowerBoundiTTP();
            // gur.Min2Factor();
            if (data.SolveTripModel){
                gur.iTTP_TripModel();
            }
            else{
                gur.iTTP();
                gur.AddMinTripLowerBoundiTTP();
            }
        }
        // gur.iTTP_TripModel();
        // gur.Fix_x(sol);
    }
    else{
        // IP without patterns:
        if (min_travel){

            string path = "Instances" + string(PATHSEP);
            if (in.getSetting() == Setting::Football){
                path += "Football" + string(PATHSEP) + "Vcr" + string(PATHSEP);
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
        if (min_travel || (min_cap && in.getSetting() == Setting::Football)){
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
        if (min_travel || (min_cap && in.getSetting() == Setting::Football)){
            gur.AddObj(min_travel, min_cap);
        }
        if (min_travel && !(in.getSetting() == Setting::Football && data.Instance == "i03" && data.CapacitySetting == 0 && data.MaxNrBreaks == 3)){
            gur.WarmStart(sol);
        }
        // gur.Fix_x(sol);
    }
    gur.setTimeLimit(param.TIME_LIMIT);
    gur.SetTimeStamps(TimeStamps);
    gur.solve();
    // cout << "save solution" << endl;
    gur.SaveSolution(sol);
    // cout << "test whether solution is feasible" << endl;
    sol.validate();
    /*
    cout << "Travel cost = " << sol.ComputeTotalCostTTP() << endl;
    GurSolver gur_validate(in);
    gur_validate.iTTP();
    gur_validate.Fix_x(sol);
    gur_validate.solve();
    cout << "feasible!!" << endl;
    */

    // Save solution in file:
    if ((in.getSetting() == Setting::Football || in.getSetting() == Setting::Hockey) && min_cap){
        string FilePathVcr = "Instances" + string(PATHSEP);
        if (in.getSetting() == Setting::Football){
            FilePathVcr += "Football" + string(PATHSEP) + "Vcr" + string(PATHSEP);
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
#ifdef PRINT
#if PRINT == 1
        cout << "Save " << gur.getBestObjValue() << " in file " << FilePathVcr << endl;
#endif 
#endif
        return;
    } 
    // cout << "validate" << endl;
    // sol.validate();

    string FilePath;
    string config;
    
    if (in.getSetting() == Setting::TTP){
        FilePath = FolderPath + "Results" + std::string(PATHSEP);
        if (data.TripModelHAP_Fixed){
            FilePath += "IP_TripModel_HAP_fixed";
        }
        else if (data.SolveTripModel){
            FilePath += "IP_TripModel";
        }
        else{
            FilePath += "IP";
        }
        FilePath += std::string(PATHSEP) + sol.getInstanceName() + ".txt";
        if (data.SolveTripModel){
            config = to_string(data.seed) + ",IP_TripModel," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
        }
        else{
            config = to_string(data.seed) + ",IP," + to_string(sol.getNrTeams()) + "," + to_string(sol.getNrRounds());
        }
    }
    else if (in.getSetting() == Setting::Football){
        FilePath = "Instances" + string(PATHSEP) + "Football" + string(PATHSEP) + "Results" + string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + "_s" + to_string(data.CapacitySetting) + "_b" + to_string(data.MaxNrBreaks) + ".txt";
        config = to_string(data.seed) + ",IP," + data.Instance + "," + to_string(data.CapacitySetting) + "," + to_string(data.MaxNrBreaks);
    }
    else if (in.getSetting() == Setting::Hockey){
        FilePath = "Instances" + string(PATHSEP) + "Hockey" + string(PATHSEP) + "Results" + string(PATHSEP) + "IP" + std::string(PATHSEP) + data.Instance + ".txt";
        config = to_string(data.seed) + ",IP," + data.Instance;
    }
#ifdef PRINT
#if PRINT == 1  
    cout << "Save file as " << FilePath << endl;
#endif 
#endif
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
    // cout << "Close file" << endl;

    // cin.get();
    return;
}

void SelectAlgo(InputData& data, ParameterValues& param){

    vector<int>TimeStamps;
    int TimeStamp = 0;
    int Incrementor = 30;
    while (TimeStamp <= param.TIME_LIMIT){ 
        TimeStamps.push_back(TimeStamp);
        TimeStamp += Incrementor;
    }

    string folder_path = FolderPath(data);
    // cout << "FolderPath: " << folder_path << endl;
    string file_path;
    if (data.TTP){
        file_path = data.Instance;
    }
    else{
        file_path = folder_path + data.Instance + ".txt";
    }

    // Code to construct vizing schedules:
    /*
    string FilePath = "Vizing_n36_r14.txt";
    std::ofstream output_file(FilePath);
    output_file << "s,r,h,a" << endl;
    for (int s = 0; s < 100000; ++s){
        if (s % 1000 == 0){
            cout << s << endl;
        }
        vector<vector<vector<int>>>schedule = VizingConstructionPython(data.seed+s);
        for (int r = 0; r < schedule.size(); ++r){
            for (int m = 0; m < schedule[r].size(); ++m){
                output_file << s << "," << r << "," << schedule[r][m][0] << "," << schedule[r][m][1] << endl;
            }
        }
    }
    return;
    */

    Input in;
    if (data.TTP && !in.read_TTP(file_path)){
        cout << "could not read TTP path " << file_path << endl;
        return;
    }
    else if ((data.Football || data.Hockey) && !in.read_YSTP(file_path, data.Football)){
        cout << "could not read Football or Hockey path " << file_path << endl;
        return;
    }
    in.SRR = true;
    if (!data.Football && !data.Hockey){
        in.setHAP_requirements(false, false, false, false, in.getNrRounds());
        in.read_HAPs(); 
    }
    else if (data.Hockey){
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
            in.setHAPSetting(data.CapacitySetting);
        }
        in.setAllowedNrCapacityViolations1RR(data);
        in.read_HAPs();
    }

    /*
    bool TravelBound = true;
    SolveLeagueByLeague(in, data, TravelBound);
    return;
    */
    
    if (data.Heuristic){
        SolveHeuristic(in,TimeStamps,folder_path,data,param);
    }
    else if (data.RunGM){
        SolveGreedyMatching(in,TimeStamps,folder_path,data,param);
    }
    else if (data.FO){
        SolveFixAndOptimize(in,TimeStamps,folder_path,data,param);
    }
    else{
        SolveIP(in,TimeStamps,folder_path,data,param);
    }
}

void BoundTTP(const int TimeLimit, const bool DLB, const string Instance, const int NrRoundsTTP, std::ofstream& output_file, const bool addMinTripConstraint, const bool addColoringConstraint){

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

    if (DLB){
        gur.BoundTTP_AllTeams(addMinTripConstraint, ConSolutions.at("I_CON" + std::to_string(in.getNrTeams()) + "_" + std::to_string(in.getNrRounds())), addColoringConstraint);
        // gur.solve();
        LB = gur.getBestBound();
        UB = gur.getBestObjValue();
        gap = gur.getMipGap();
#ifdef PRINT
#if PRINT == 1
        cout << "LB for instance " << Instance << " with " << NrRoundsTTP << " = " << LB << ", UB = " << UB << ", gap = " << gap << endl;
#endif 
#endif
        output_file << Instance << "," << LB << "," << UB << "," << gap << "," << NrRoundsTTP << "\n";
    }
    else{
        int sum = 0;
        for (int t = 0; t < in.getNrTeams(); ++t){
            gur.BoundTTP(t);
            sum += gur.solve();
        }
        output_file << Instance << "," << sum << "," << NrRoundsTTP << "\n";
    }
}

void BoundsTTP_OneInstance(InputData& data, ParameterValues& param){
    Input in;
    if (!in.read_TTP(data.Instance)){
        cout << "could not read " << data.Instance << endl;
        return;
    }
    string OutputFilePath = "Instances" + std::string(PATHSEP) + "TTP" + std::string(PATHSEP) + "Bounds" + std::string(PATHSEP);
    if (data.DLB){
        if (data.addMinTripConstraint){
            OutputFilePath += "DLB_MinTrip_";
        }
        else if (data.addColoringConstraint){
            OutputFilePath += "DLB_Coloring_";
        }
        else{
            OutputFilePath += "DLB_";
        }
    }
    else{
        OutputFilePath += "ILB_";
    }
    OutputFilePath += in.getInstanceName() + ".txt";
#ifdef PRINT
#if PRINT == 1
    cout << "Save file as " << OutputFilePath << endl;
#endif 
#endif
    std::ofstream output_file(OutputFilePath);
    BoundTTP(param.TIME_LIMIT, data.DLB, data.Instance, data.NrRounds, output_file, data.addMinTripConstraint, data.addColoringConstraint);
    output_file.close();
}
