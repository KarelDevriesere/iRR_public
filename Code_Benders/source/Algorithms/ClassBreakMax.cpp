/************************** Include class header *******************************/
#include "ClassBreakMax.h"
/*******************************************************************************/

// Some parameters of the algorithm
const int maxDepthFrac = -1;
const int numThreadsMaster = 5;
// const int TreeMemory = 16000;
const int WorkMem = 16000;
bool complementary = true;
bool lexicographic = true;
bool patternVars = false;

// Backtracking information
std::string cutNames1[4]={"Benders feas", "Benders optim", "No-good feas", "No-good optim"};
// Total number of times a cut is added
int cutsAdded1[4] = {0,0,0,0};
// Total time taken to {solve all LP's, solve all IPs}
float timeTaken1[2] = {0,0};

// Protect from multiple accesses
IloFastMutex lck1;


// constructor
BreakMax::BreakMax(Instance* i, const int timeLimit, const std::string solName, const int method) : Algorithm(i, timeLimit, solName) {
	std::cout << "--------------------------------------------------------------------" << std::endl;
	std::cout << "--- Starting 2-phase HAP approach for break minimization and maximization ---" << std::endl;
	std::cout << "timeLimit: " << timeLimit << std::endl;
	std::cout << "solName: " << solName << std::endl;

	if(method == 0){
		complementary = true;
		lexicographic = true;
	} else if(method == 1){
		complementary = false;
		lexicographic = true;
	} else if(method == 2){
		complementary = true;
		lexicographic = false;
	} else if(method == 3){
		complementary = false;
		lexicographic = false;
	} else if(method == 4){
		patternVars = true;
		complementary = false;
		lexicographic = false;
	} else if(method == 5){
		patternVars = true;
		complementary = true;
		lexicographic = false;
	} else {
		std::cout << "Method for break maximization not known..." << std::endl;
		std::abort();
	}

	std::cout << "Complementary: " << complementary << std::endl;
	std::cout << "Lexicographic: " << lexicographic << std::endl;
	std::cout << "Pattern vars: " << patternVars << std::endl;
	std::cout << "--------------------------------------------------------------------" << std::endl;

	

	// Symmetry structure
	(IN->getLeague(0)->getMode() == M) ? mirrored = 1 : mirrored = 0;

	// Read constraints to determine availability matrix
	for(auto c : IN->getCnstrs()){
		std::string cName = c->getName();
		if (cName.compare("CA3") == 0) {
			const CA3 *ca3 = dynamic_cast<CA3*>(c);
			if(IN->collectTeams(ca3->teams1, ca3->teamGroups1).size() != nrTeams || IN->collectTeams(ca3->teams1, ca3->teamGroups1).size() != nrTeams){
				std::cout << "CA3 should involve all teams!" << std::endl;
				std::abort();
			}
			if (ca3->min != 0) {
				std::cout << "Expect CA3 min to be 0" << std::endl;
				std::abort();
			}
			if(ca3->mode2 !=  GAMES){
				std::cout << "Expect CA3 mode2 to be GAMES" << std::endl;
				std::abort();
			}
			if(ca3->mode1 == H){
				maxHomeGames = ca3->max;
			} else if (ca3->mode1 == A){
				maxAwayGames = ca3->max;
			} else {
				std::cout << "Mode of CA3 not yet implemented" << std::endl;
				std::abort();
			}
		} else if (cName.compare("SE1") == 0) {
			const SE1 *se1 = dynamic_cast<SE1*>(c);
			if(IN->collectTeams(se1->teams, se1->teamGroups).size() != nrTeams){
				std::cout << "SE1 should involve all teams!" << std::endl;
				std::abort();
			}
			separation = se1->min;
		} else if (cName.compare("CA1") == 0) {
			std::cout << "WARNING IGNORING CA1" << std::endl;
		} else if (cName.compare("BA1") == 0) {
			std::cout << "WARNING IGNORING BA1" << std::endl;
		} else {
			std::cout << "Constraint " << cName << " not implemented" << std::endl;
			std::abort();
		}
	}

	// Objective of the problem
	if(IN->getObjective() == TR){
	} else {
		std::cout << "Objective mode not yet implemented." << std::endl;
		std::abort();
	}

	// Measure time after initializing data structures
       	begin = std::chrono::steady_clock::now();
}

// destructor
BreakMax::~BreakMax(){
	std::cout << "---------------------------------------" << std::endl;
	std::cout << "--- Two-step approach done ---" << std::endl;
	std::cout << "---------------------------------------" << std::endl;
}

void BreakMax::enumeratePatterns(const int requiredBreaks){
	// We use CP to generate all patterns
	// Complete enumeration with next_permutation, and then filtering all patterns that satisfy some
	// requirements is too slow when teams > 16 (too many possibilities to enumerate)
	
	std::cout << "Construct patterns with " << requiredBreaks << " breaks" << std::endl;

	// Cplex object to store the model with all settings
	IloEnv env = IloEnv();

	// Cplex object to store settings of the model
	IloModel model = IloModel(env);
	try{
		// h[s] = 1 if the pattern has a home game in slot s, 0 else
		IloBoolVarArray h(env, nrSlots);

		// sequence(nbMin, nbMax, width, vars, values, card)
		// values and cards must have the same index set I and in the given application the two arrays are of size 1.
		// vars is an array of variables and the constraint requires that for all i\in I the number of variables in vars, 
		// which is equal to values[i], must be card[i].
		// Furthermore, any subsequence of size width must contain at least nbMin and at most nbMax values from values.
		// The constraint requires that each team has nrSlots/2 home games, and plays at most 3 home and 3 away games in a row
		model.add(IloSequence(env, 1, 3, 4, h, IloIntArray(env,1,0), IloIntVarArray(env,1,nrSlots/2,nrSlots/2)));

		// First time slot is a home game: we add all other patterns later on
		model.add(h[0] == 0);

		// Count the total number of breaks
		IloExpr breaks(env);
		for (int s = 0; s < nrSlots-1; ++s) {
			breaks += (h[s] == h[s+1]);	
		}
		model.add(breaks == requiredBreaks);

		if(mirrored){
			for (int s = 0; s < nrSlots/2; ++s) {
				model.add(h[s] + h[s + nrSlots/2] == 1);
			}
		}

		// Depth first with one worker to find each pattern only once
		IloCP cp = IloCP(model);
		cp.setParameter(IloCP::SearchType, IloCP::DepthFirst);
		cp.setParameter(IloCP::Workers, 1);
		cp.setOut(env.getNullStream());

		cp.startNewSearch();
		while(cp.next()){
			// Store the pattern and its complement
			std::vector<int> pattern(nrSlots,0);
			std::vector<int> patternC(nrSlots,0);
			for (IloInt s = 0; s < nrSlots; ++s) {
				pattern.at(s) = cp.getValue(h[s]);
				patternC.at(s) = 1 - pattern.at(s);
				std::cout << cp.getValue(h[s]) << "\t";
			}
			std::cout << std::endl;

			patterns.push_back(pattern);
			patterns.push_back(patternC);
		}

	} catch (IloException &ex){
		env.out() << "Unexpected error. Caught " << ex << std::endl;
	}
	env.end();
}


void BreakMax::breakSymmetry(std::vector<std::vector<std::string>> &temp, int start, int end, int round){
	int length = std::ceil(((float) end-start)/2);	
	if(length < 2) {
		if(start+1 < end){
			temp[start][round] = "<";
			temp[start+1][round] = "<";
			masterModel.add(h[start][round] <= h[start+1][round]);

			if(!complementary){
				temp[start+nrTeams/2][round] = "<";
				temp[start+1+nrTeams/2][round] = "<";
				masterModel.add(h[start+nrTeams/2][round] <= h[start+1+nrTeams/2][round]);
			}
		}

		return;
	}
	static int symbol = -1;
	symbol++;

	for (int i = start; i < start + length; ++i) {
		if(i < start + length - 1){
			temp[i][round] = std::to_string(symbol);
			temp[i+1][round] = std::to_string(symbol);
			masterModel.add(h[i][round] == h[i+1][round]);

			if(!complementary){
				temp[i+nrTeams/2][round] = std::to_string(symbol);
				temp[i+nrTeams/2+1][round] = std::to_string(symbol);
				masterModel.add(h[i+nrTeams/2][round] == h[i+nrTeams/2+1][round]);
			}
		}
	}
	breakSymmetry(temp, start, start+length, round+1);
	breakSymmetry(temp, start+length, end, round);
}
	

void BreakMax::createMasterILP(){
	std::cout << "*******************\n"
		  << "** Create Master **\n"
		  << "*******************\n" << std::endl;
	
	// Construct model and cplex
	masterModel = IloModel(masterEnv, "HAPSetGenerationModel");
	masterCplex = IloCplex(masterModel);

	// Allow cplex to store the branch-and-bound tree in a file, 
	// to avoid running out of memory
	// 16 GB of working memory = 16000 MB
	// To avoid a failure due to running out of memory, set the working memory parameter, WorkMem, to a value 
	// *** SIGNIGICANTLY LOWER ***
	// than the available memory on your computer (in megabytes), to instruct CPLEX to begin compressing the storage of nodes before it consumes all of available memory. See the related topic Use node files for storage, for other choices of what should happen when WorkMem is exceeded. That topic explains how to tell CPLEX that it should use disk for working storage.
	// See: https://www.ibm.com/support/knowledgecenter/ko/SSSA5P_12.8.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/troubleshoot/61_mem_gone.html
	masterCplex.setParam(IloCplex::Param::WorkMem, WorkMem);

	// TODO TODO 
	// masterCplex.setParam(IloCplex::Param::Emphasis::MIP,CPX_MIPEMPHASIS_HIDDENFEAS);


	// Instead of limiting the size of the tree, we opt to write the tree out to node files
	// from as soon as it exceeds the work memory
	//masterCplex.setParam(IloCplex::Param::MIP::Limits::TreeMemory, TreeMemory);
	// Best to use a single node to run Cplex on, as SCRATCH NODE is 
	// only accessible by the workers from the same node
	// TODO TODO TODO
	//masterCplex.setParam(IloCplex::Param::WorkDir, std::getenv("VSC_SCRATCH_NODE"));
	//masterCplex.setParam(IloCplex::Param::MIP::Strategy::File, 3);

	// Avoid cplex from running out of time
	masterCplex.setParam(IloCplex::Param::TimeLimit, timeLimit);

	// h[i][s] = 1 if team i has a home game on time slot s, 0 if it has an away game
	h = IloBoolVarArray2(masterEnv, nrTeams);
	for (int i = 0; i < nrTeams; ++i) {
		h[i] = IloBoolVarArray(masterEnv, nrSlots);		
		// Reduce the total number of variables in the system
		for (int s = 0; s < nrSlots; ++s) {
			std::string varName = "h_" + std::to_string(i) + "_" + std::to_string(s);
			h[i][s].setName(varName.c_str());
			masterModel.add(h[i][s]);
		}
	}

	if (mirrored) {
		for (int i = 0; i < nrTeams; ++i) {
			for (int s = 0; s < nrTeams-1; ++s) {
				masterModel.add(h[i][s] == 1-h[i][s+nrTeams-1]);
			}
		}
	}

	if(complementary){
		for (int i = 0; i < nrTeams/2; ++i) {
			for (int s = 0; s < nrSlots; ++s) {
				masterModel.add(h[i][s] == 1-h[i+nrTeams/2][s]);
			}
		}		
	}

	// Symmetry breaking: lexicographical ordering
	if (symmetric) {

		// First half of the teams play home
		for (int i = 0; i < nrTeams/2; ++i) {
			masterModel.add(h[i][0] == 1);
		}

		// TODO TODO TODO
		// Break symmetry based on the observation that in any group of n teams, at least 
		// n/2 teams will have the same pattern
		// E.g. in time slot 0, first n/2 teams play home --> out of these teams, at least half (i.e. n/4) teams 
		// must have the same pattern in the second time slot
		std::vector<std::vector<std::string>> temp(nrTeams, std::vector<std::string>(nrSlots, ""));
		for (int i = 0; i < nrTeams; ++i) {
			if(i < nrTeams/2){
				temp[i][0] = "H";
			} else {
				temp[i][0] = "A";
			}
		}
		breakSymmetry(temp, 0, nrTeams/2, 1);

		std::cout << "Symmetry breaking structure:" << std::endl;
		for(auto r : temp){
			for(auto c : r){
				std::cout << c << " ";
			}
			std::cout << std::endl;
		}
	}

	{ // Pattern of each team contains exactly nrSlots/2 h's
		int i,s;
		for (i = 0; i < nrTeams; ++i) {
			IloNumExpr homes(masterEnv);
			for (s = 0; s < nrSlots; ++s) {
				homes += h[i][s];	
			}
			if(nrSlots%2 != 0){
				std::cout << "I expected the number of slots to be even!" << std::endl;
				std::abort();
			}
			masterModel.add(homes == nrSlots/2);
		}
	}

	 // During each time slot, half of the teams play home
	for (int s = 0; s < nrSlots; ++s) {
		IloNumExpr homes(masterEnv);
		for (int i = 0; i < nrTeams; ++i) {
			homes += h[i][s];	
		}
		masterModel.add(homes == nrTeams/2);
	}

	// Max series of home or away games
	if (maxHomeGames != -1) {
		for (int i = 0; i < nrTeams; ++i) {
			for (int s = maxHomeGames; s < nrSlots; ++s) {
				// At least one away game for every maxHomeGames+1 slots
				IloExpr hVars(masterEnv);	
				for (int k = 0; k <= maxHomeGames; ++k) {
					hVars += h[i][s-k];	
				}
				masterModel.add(hVars <= maxHomeGames);
			}	
		}
	}
	if (maxAwayGames != -1) {
		for (int i = 0; i < nrTeams; ++i) {
			for (int s = maxAwayGames; s < nrSlots; ++s) {
				// At least one home game for every maxHomeGames+1 slots
				IloExpr hVars(masterEnv);	
				for (int k = 0; k <= maxAwayGames; ++k) {
					hVars += h[i][s-k];	
				}
				masterModel.add(hVars >= 1);
			}	
		}
	}

	// bh[i][s] = 1 if team i has a home break on slot s, 0 else
	bh = IloBoolVarArray2(masterEnv, nrTeams);
	for (int i = 0; i < nrTeams; ++i) {
		bh[i] = IloBoolVarArray(masterEnv, nrSlots);		
		// Reduce the total number of variables in the system
		for (int s = 0; s < nrSlots; ++s) {
			std::string varName = "bh_" + std::to_string(i) + "_" + std::to_string(s);
			bh[i][s].setName(varName.c_str());
			masterModel.add(bh[i][s]);
		}
		bh[i][0].setBounds(0,0);
	}
	
	// ba[i][s] = 1 if team i has an away break on slot s, 0 else
	ba = IloBoolVarArray2(masterEnv, nrTeams);
	for (int i = 0; i < nrTeams; ++i) {
		ba[i] = IloBoolVarArray(masterEnv, nrSlots);		
		// Reduce the total number of variables in the system
		for (int s = 0; s < nrSlots; ++s) {
			std::string varName = "ba_" + std::to_string(i) + "_" + std::to_string(s);
			ba[i][s].setName(varName.c_str());
			masterModel.add(ba[i][s]);
		}
		ba[i][0].setBounds(0,0);
	}

	// Regulate value of break variables
	for (int i = 0; i < nrTeams; ++i) {
		for (int s = 1; s < nrSlots; ++s) {
			masterModel.add(h[i][s]+h[i][s-1]-bh[i][s]+ba[i][s] == 1);
		}
	}
	

	// Regulate break variables from above (otherwise, bh and ba can cancel out each other)
	// 0.7 + 0 + 0.65 - 0.35 == 1
	for (int i = 0; i < nrTeams; ++i) {
		for (int s = 1; s < nrSlots; ++s) {
			masterModel.add(bh[i][s]-h[i][s]<=0);
			masterModel.add(bh[i][s]-h[i][s-1]<=0);
			masterModel.add(ba[i][s]+h[i][s]<=1);
			masterModel.add(ba[i][s]+h[i][s-1]<=1);
		}
	}
	

	 // Objective is to minimize or maximize the sum of breaks or to minimize the total travel distance
	IloNumExpr breaks(masterEnv);
	int i, s;

	// Max number of breaks per team
	for (i = 0; i < nrTeams; ++i) {
		IloNumExpr breaksBis(masterEnv);
		for (s = 1; s < nrSlots; ++s) {
			breaks += bh[i][s] + ba[i][s];
			breaksBis += bh[i][s] + ba[i][s];
		}
	}

	// Tell Cplex that the number of breaks must be even
	IloIntVar dummy(masterEnv, 0, IloInfinity);
	masterModel.add(dummy == 2*breaks);

	// Transform breaks into trips
	IloIntVar travelDist(masterEnv, 0, IloInfinity);
	//masterModel.add(travelDist == 2*nrTeams*(nrTeams-1) - breaks/2);
	masterModel.add(travelDist == nrTeams*nrSlots - breaks/2);

	// Minimize the total number of trips
	masterModel.add(IloMinimize(masterEnv, travelDist));

	std::cout << "Master constructed" << std::endl;
}

void BreakMax::createMasterILPPatterns(const int firstNewPattern, const int lastNewPattern){
	std::cout << "********************************\n"
		  << "** Create Master Pattern vars **\n"
		  << "********************************\n" << std::endl;
	
	// Construct model and cplex
	masterModel = IloModel(masterEnv, "HAPSetGenerationModel");
	masterCplex = IloCplex(masterModel);

	// Allow cplex to store the branch-and-bound tree in a file, 
	// to avoid running out of memory
	// 16 GB of working memory = 16000 MB
	// To avoid a failure due to running out of memory, set the working memory parameter, WorkMem10, to a value 
	// *** SIGNIGICANTLY LOWER ***
	// than the available memory on your computer (in megabytes), to instruct CPLEX to begin compressing the storage of nodes before it consumes all of available memory. See the related topic Use node files for storage, for other choices of what should happen when WorkMem10 is exceeded. That topic explains how to tell CPLEX that it should use disk for working storage.
	// See: https://www.ibm.com/support/knowledgecenter/ko/SSSA5P_12.8.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/troubleshoot/61_mem_gone.html
	masterCplex.setParam(IloCplex::Param::WorkMem, WorkMem);

	// TODO TODO 
	// masterCplex.setParam(IloCplex::Param::Emphasis::MIP,CPX_MIPEMPHASIS_HIDDENFEAS);


	// Instead of limiting the size of the tree, we opt to write the tree out to node files
	// from as soon as it exceeds the work memory
	//masterCplex.setParam(IloCplex::Param::MIP::Limits::TreeMemory, TreeMemory);
	// Best to use a single node to run Cplex on, as SCRATCH NODE is 
	// only accessible by the workers from the same node
	// TODO TODO TODO
	//masterCplex.setParam(IloCplex::Param::WorkDir, std::getenv("VSC_SCRATCH_NODE"));
	//masterCplex.setParam(IloCplex::Param::MIP::Strategy::File, 3);

	// Avoid cplex from running out of time
	masterCplex.setParam(IloCplex::Param::TimeLimit, timeLimit);

	IloBoolVarArray2 z(masterEnv, nrTeams);
	for (int i = 0; i < nrTeams; ++i) {
		z[i] = IloBoolVarArray(masterEnv, patterns.size());		
		for(int p=0; p < patterns.size(); ++p){
			std::string varName = "p_" + std::to_string(i) + "_" + std::to_string(p);
			z[i][p].setName(varName.c_str());
			masterModel.add(z[i][p]);
		}
	}

	if (complementary) {
		for (int i = 0; i < nrTeams/2; i++) {
			for (int p = 0; p < patterns.size()-1; p+=2) {
				masterModel.add(z[i][p] == z[i+nrTeams/2][p+1]);
			}
		}
	}

	// Each team is assigned to exactly one pattern
	for (int i = 0; i < nrTeams; ++i) {
		IloExpr patternSum(masterEnv);
		for(int p=0; p < patterns.size(); ++p){
			patternSum += z[i][p];
		}
		masterModel.add(patternSum == 1);
	}	

	if (complementary) {
		for (int i = 0; i < nrTeams/2 - 1; i++) {
			IloExpr patternSum1(masterEnv);
			IloExpr patternSum2(masterEnv);
			for(int p=0; p < patterns.size(); ++p){
				patternSum1 += p*z[i][p];
				patternSum2 += p*z[i+1][p];
			}
			masterModel.add(patternSum1 <= patternSum2);	
		}		
	} else {
		// Patterns are assigned lexicographically
		for (int i = 0; i < nrTeams-1; ++i) {
			IloExpr patternSum1(masterEnv);
			IloExpr patternSum2(masterEnv);
			for(int p=0; p < patterns.size(); ++p){
				patternSum1 += p*z[i][p];
				patternSum2 += p*z[i+1][p];
			}
			masterModel.add(patternSum1 <= patternSum2);
		}
	}

	// At least one of the new patterns is chosen
	{
		IloExpr patternSum(masterEnv);
		for (int i = 0; i < nrTeams-1; ++i) {
			for(int p=firstNewPattern; p < lastNewPattern; ++p){
				patternSum += z[i][p];
			}
		}
		masterModel.add(patternSum >= 1);
	}
	


	// h[i][s] = 1 if team i has a home game on time slot s, 0 if it has an away game
	h = IloBoolVarArray2(masterEnv, nrTeams);
	for (int i = 0; i < nrTeams; ++i) {
		h[i] = IloBoolVarArray(masterEnv, nrSlots);		
		// Reduce the total number of variables in the system
		for (int s = 0; s < nrSlots; ++s) {
			std::string varName = "h_" + std::to_string(i) + "_" + std::to_string(s);
			h[i][s].setName(varName.c_str());
			masterModel.add(h[i][s]);
		}
	}

	// Link the h and z vars
	for (int i = 0; i < nrTeams; ++i) {
		for (int s = 0; s < nrSlots; ++s) {
			IloExpr homePatterns(masterEnv);
			for(int p=0; p < patterns.size(); ++p){
				if(patterns[p][s]){
					homePatterns += z[i][p];
				}
			}
			masterModel.add(h[i][s] == homePatterns);
		}
	}
	
	// During each time slot, half of the teams play home
	for (int s = 0; s < nrSlots; ++s) {
		IloNumExpr homes(masterEnv);
		for (int i = 0; i < nrTeams; ++i) {
			homes += h[i][s];	
		}
		masterModel.add(homes == nrTeams/2);
	}

	
	// // Objective is to minimize or maximize the sum of breaks or to minimize the total travel distance
	IloNumExpr breaks(masterEnv);
	for(int p=0; p < patterns.size(); ++p){
		// Count breaks in pattern p
		int noBreaks = 0;
		for (int s = 0; s < nrSlots - 1; ++s) {
			noBreaks += (patterns[p][s] == patterns[p][s+1]);	
		}

		for (int i = 0; i < nrTeams; ++i) {
			breaks += z[i][p]*noBreaks;
		}
	}


	//// Tell Cplex that the number of breaks must be even
	IloIntVar dummy(masterEnv, 0, IloInfinity);
	masterModel.add(dummy == 2*breaks);

	// Transform breaks into trips
	IloIntVar travelDist(masterEnv, 0, IloInfinity);
	//masterModel.add(travelDist == 2*nrTeams*(nrTeams-1) - breaks/2);
	masterModel.add(travelDist == nrTeams*nrSlots - breaks/2);

	// Minimize the total number of trips
	masterModel.add(IloMinimize(masterEnv, travelDist));

	std::cout << "Master constructed" << std::endl;
}

void BreakMax::solve(){
	IloEnv masterEnv;
	try {
		// Create master ILP
		createMasterILP();

		// Limit the total number of cores for the master
		masterCplex.setParam(IloCplex::Param::Threads, numThreadsMaster);

		// Better exploit parallel structure
		// See Fischetti, M.; Ljubi I. & Sinnl, M. Benders decomposition without separability: A computational study for capacitated facility location problems European Journal of Operational Research, 2016, 253, 557 - 569
		if (numThreadsMaster > 1) {
			masterCplex.setParam(IloCplex::Param::Parallel, CPX_PARALLEL_OPPORTUNISTIC);
		}


		// Set up the callback to be used for separating cuts
		// Subproblem uses the same number of threads as the master
		Callback cb(h, numThreadsMaster, mirrored, separation);
		CPXLONG contextmask = IloCplex::Callback::Context::Id::Candidate
			| IloCplex::Callback::Context::Id::ThreadUp
			| IloCplex::Callback::Context::Id::ThreadDown;

		// False if we only run the benders for integer subproblems, true if we also separate fracational master solutions
		const bool separateFracSols = true;
		if (separateFracSols){
			contextmask |= IloCplex::Callback::Context::Id::Relaxation;
		}
		masterCplex.use(&cb, contextmask);

		// Solve the model and write out the solution
		if (masterCplex.solve()) {

			IloAlgorithm::Status solStatus= masterCplex.getStatus();
			std::cout << "Solution status master: " << solStatus << std::endl;
			std::cout << "Objective value master: " << masterCplex.getObjValue() << std::endl;
			std::cout << "Objective bound master: " << masterCplex.getBestObjValue() << std::endl;

			std::cout << "Optimal HAP set:" << std::endl;
			for (int i = 0; i < nrTeams; ++i) {
				for (int s = 0; s < nrSlots; ++s) {
					std::cout << masterCplex.getIntValue(h[i][s]) << "\t";
				}	
				std::cout << std::endl;
			}

		} else {

			IloAlgorithm::Status solStatus= masterCplex.getStatus();
			std::cout << "Status master: " << solStatus << std::endl;
			std::cout << "No solution found." << std::endl;
		}
	} catch (const IloException& e) {
		std::cout << "IloException caught: " << e << std::endl;
		masterEnv.end();
		throw;
	} catch (...) {
		std::cout << "Unknown exception caught!" << std::endl;
		masterEnv.end();
		throw;
	}

	// Close the environments
	masterEnv.end();

	end = std::chrono::steady_clock::now();
	std::cout << std::fixed;
	std::cout << "Total elapsed time " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << " seconds\n" << std::endl;
	
	std::cout << "Total number of Benders feasibility cuts added: " << cutsAdded1[0] << std::endl;
	std::cout << "Total number of Benders optimality cuts added: " << cutsAdded1[1] << std::endl;
	std::cout << "Total number of no-good feasiblity cuts added: " << cutsAdded1[2] << std::endl;	
	std::cout << "Total number of feasible solutions found: " << cutsAdded1[3] << std::endl;
	
	std::cout << "Total time spend to solve LP subproblems: " << timeTaken1[0] << std::endl;
	std::cout << "Total time spend to solve IP subproblems: " << timeTaken1[1] << std::endl;

	return;
}

void BreakMax::solvePattern(){

	IloEnv masterEnv;
	try {
		bool solFound  = false;
		int noBreaks = 4*((int) (nrSlots/2)/3) + ((nrSlots/2)%3 > 1 ? 2*((nrSlots/2)%3-1) : 0);

		while(!solFound){
			if (noBreaks < 0) {
				std::cout << "No feasible solution exists: checked all possible patterns..." << std::endl;
				std::abort();
			}

			// Initialize patterns
			int oldSize = (int) patterns.size();
			// At least n patterns, and at least one newly generated
			while(patterns.size() < nrTeams || patterns.size() == oldSize){
				enumeratePatterns(noBreaks);
				noBreaks--;
				if(noBreaks < 0){
					break;
				}
			}
			std::cout << "Size pattern set: " << patterns.size() << std::endl;

			// Create master ILP
			createMasterILPPatterns(oldSize, (int) patterns.size()-1);

			// Limit the total number of cores for the master
			masterCplex.setParam(IloCplex::Param::Threads, numThreadsMaster);

			// Better exploit parallel structure
			// See Fischetti, M.; Ljubi I. & Sinnl, M. Benders decomposition without separability: A computational study for capacitated facility location problems European Journal of Operational Research, 2016, 253, 557 - 569
			if (numThreadsMaster > 1) {
				masterCplex.setParam(IloCplex::Param::Parallel, CPX_PARALLEL_OPPORTUNISTIC);
			}

			// Set up the callback to be used for separating cuts
			// Subproblem uses the same number of threads as the master
			Callback cb(h, numThreadsMaster, mirrored, separation);
			CPXLONG contextmask = IloCplex::Callback::Context::Id::Candidate
				| IloCplex::Callback::Context::Id::ThreadUp
				| IloCplex::Callback::Context::Id::ThreadDown;

			// False if we only run the benders for integer subproblems, true if we also separate fracational master solutions
			const bool separateFracSols = true;
			if (separateFracSols){
				contextmask |= IloCplex::Callback::Context::Id::Relaxation;
			}
			masterCplex.use(&cb, contextmask);

			// Solve the model and write out the solution
			if (masterCplex.solve()) {
				IloAlgorithm::Status solStatus= masterCplex.getStatus();
				std::cout << "Solution status master: " << solStatus << std::endl;
				std::cout << "Objective value master: " << masterCplex.getObjValue() << std::endl;
				std::cout << "Objective bound master: " << masterCplex.getBestObjValue() << std::endl;

				std::cout << "Optimal GOP set:" << std::endl;
				for (int i = 0; i < nrTeams; ++i) {
					for (int s = 0; s < nrSlots; ++s) {
						std::cout << masterCplex.getValue(h[i][s]) << "\t";
					}	
					std::cout << std::endl;
				}

				solFound = true;

			} else {

				IloAlgorithm::Status solStatus= masterCplex.getStatus();
				std::cout << "Status master: " << solStatus << std::endl;
				std::cout << "No solution found." << std::endl;
			}
		}
	} catch (const IloException& e) {
		std::cout << "IloException caught: " << e << std::endl;
		masterEnv.end();
		throw;
	} catch (...) {
		std::cout << "Unknown exception caught!" << std::endl;
		masterEnv.end();
		throw;
	}

	// Close the environments
	masterEnv.end();

	end = std::chrono::steady_clock::now();
	std::cout << std::fixed;
	std::cout << "Total elapsed time " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << " seconds\n" << std::endl;
	
	std::cout << "Total number of Benders feasibility cuts added: " << cutsAdded1[0] << std::endl;
	std::cout << "Total number of Benders optimality cuts added: " << cutsAdded1[1] << std::endl;
	std::cout << "Total number of no-good feasiblity cuts added: " << cutsAdded1[2] << std::endl;	
	std::cout << "Total number of feasible solutions found: " << cutsAdded1[3] << std::endl;
	
	std::cout << "Total time spend to solve LP subproblems: " << timeTaken1[0] << std::endl;
	std::cout << "Total time spend to solve IP subproblems: " << timeTaken1[1] << std::endl;

	return;
}

Callback::Callback(const IloBoolVarArray2 h, const IloInt numWorkers, const bool mirrored, const int separation): h(h), bh(bh), ba(ba), workers(numWorkers, 0), mirrored(mirrored), separation(separation){
}

Callback::~Callback(){
	std::cout << "Clearing" << std::endl; 
	IloInt numWorkers = workers.size();
	for (IloInt w = 0; w < numWorkers; w++) {
		delete workers.at(w);
	}
	workers.clear();
}

void Callback::invoke(const IloCplex::Callback::Context& context){

	int const threadNo = context.getIntInfo(IloCplex::Callback::Context::Info::ThreadId);

	// setup
	if (context.inThreadUp()) {
		delete workers[threadNo];
		workers[threadNo] = new Worker(h, context.getEnv(), threadNo, mirrored, separation);
		return;
	}

	// teardown
	if (context.inThreadDown()) {
		delete workers[threadNo];
		workers[threadNo] = 0;
		return;
	}

	// We need to separate a cut

	// Get info of problem
	IloEnv env = context.getEnv();
	IloInt nrTeams = h.getSize();
	IloInt nrSlots = h[0].getSize();

	// Retrieve the GOP set generated by the master
	IloNumArray2 hSol(env, nrTeams);
	bool separate = false;
	switch (context.getId()) {
		case IloCplex::Callback::Context::Id::Candidate:
			separate = true;
			if (!context.isCandidatePoint()){ // The model is always bounded
				throw IloCplex::Exception(-1, "Unbounded master problem");
			}
			for (int i = 0; i < nrTeams; ++i) {
				hSol[i] = IloNumArray(env, nrSlots);
				context.getCandidatePoint(h[i], hSol[i]);
			}

			break;
		case IloCplex::Callback::Context::Id::Relaxation:
			if (context.getIntInfo(IloCplex::Callback::Context::Info::NodeDepth) <= maxDepthFrac) { // Only generate fractional cuts at the root
				separate = true;
				for (int i = 0; i < nrTeams; ++i) {
					hSol[i] = IloNumArray(env);
					context.getRelaxationPoint(h[i], hSol[i]);
				}
			}
			break;

		default:
			// Free memory
			for (IloInt i = 0; i < nrTeams; ++i){
				hSol[i].end();
			}
			hSol.end();
			throw IloCplex::Exception(-1, "Unexpected contextID");
	}

	// Get the right worker
	Worker* worker = workers[threadNo];


	if (separate) {
		const bool fractionalMaster = (context.getId() == IloCplex::Callback::Context::Id::Relaxation);
		const int bestInt = (context.getId() == IloCplex::Callback::Context::Id::Relaxation) ? -1 : bestIntObj;


		// timetable[i][j] gives the time slot for game (i,j)
		std::vector<std::vector<int>> timetable(nrTeams, std::vector<int>(nrTeams,-1));

		const float objValue = ((fractionalMaster ? context.getRelaxationObjective() : context.getCandidateObjective()));
		std::vector<IloRange> cuts = worker->separate(h, hSol, context.getEnv(), fractionalMaster, timetable);

		if (cuts.size() > 0) {
			// Add the cuts
			switch (context.getId()) {
				case IloCplex::Callback::Context::Id::Candidate:
					for(auto r : cuts){
						// GOP is infeasible: reject
						context.rejectCandidate(r);
						r.end();
					}
					break;
				case IloCplex::Callback::Context::Id::Relaxation:
					for(auto r : cuts){
						context.addUserCut(r,IloCplex::UseCutPurge,IloFalse);
						r.end();
					}
					{
						const int nodeId = context.getIntInfo(IloCplex::Callback::Context::Info::NodeUID);
						if(nrCutsAddedNode.count(nodeId)){
							nrCutsAddedNode[nodeId]++;
						} else {
							nrCutsAddedNode[nodeId] = 0;
						}
						// Cplex user manual: Because of numerics, always impose a limit on the number of times you return cuts on a given node.
						const int cutThreshold = 100;
						if (nrCutsAddedNode[nodeId] > cutThreshold) {
							std::cout << "Exit cut loop at " << nodeId << std::endl;
							context.exitCutLoop();
						}
					}
					break;

				default:
					throw IloCplex::Exception(-1, "Unexpected contextID");
			}
		} else if(!fractionalMaster && objValue < bestIntObj) {
			std::cout << "Found a new better solution: " << objValue << std::endl;	
			bestIntObj = objValue;


			// Store the new solution
			//std::vector<std::array<int, 3>> meetings;
			Instance::get()->clearSchedule();
			for (int i = 0; i < nrTeams; ++i) {
				for (int j = i+1; j < nrTeams; ++j) {
					if(timetable[i][j] != -1){
						//meetings.push_back({i,j,timetable[i][j]});
						Interface::get()->scheduleMeeting(i,j,timetable[i][j]);
					}
					if(timetable[j][i] != -1){
						//meetings.push_back({j,i,timetable[j][i]});
						Interface::get()->scheduleMeeting(j,i,timetable[j][i]);
					}
				}
			}

			// Verify solution with RobinX
			Interface::get()->addObjectiveValue(0,objValue);
			ObjCost obj = Interface::get()->checkConstr();//meetings, ObjCost(0,objValue));



			std::cout << "Cost RobinX: " << std::setw(20) << "(" + std::to_string(obj.first) + "," + std::to_string(obj.second) + ")" << std::right << std::endl;
			if (obj.first > 0) {
				std::cout << "Solution not valid according to RobinX" << std::endl;
				std::abort();
			}
			if (obj.second != objValue) {
				std::cout << "Objective value does not correspond with RobinX: " << obj.second << " vs. " << objValue << std::endl;
				std::abort();
			}

			Interface::get()->writeSolutionXml(Instance::get()->getInstanceName() + (mirrored ? "Mirrored" : "") + (complementary ? "_Comp_" : "_") + (lexicographic ? "" : "NoLex_") + std::to_string(objValue) + ".xml");

		}
	}

	// Free memory
	for (int i = 0; i < nrTeams; ++i){
		hSol[i].end();
	}
	hSol.end();
}

Worker::Worker(const IloBoolVarArray2& h, const IloEnv masterEnv, const int threadNo, const bool mirrored, const int separation) : cplex(env), nrTeams(h.getSize()), nrSlots(h[0].getSize()), threadNo(threadNo) {
	std::cout << "Initialize Worker" << std::endl;

	// Construct a model
	IloModel model(env, "benders_worker");

	// Set up IloCplex algorithm to solve the worker LP
	cplex.extract(model);
	cplex.setOut(env.getNullStream());
	cplex.setWarning(env.getNullStream());

	// Subproblem cannot use more than one thread at the same time
	cplex.setParam(IloCplex::Param::Threads, 1);

	// Param::Advance, 2: retains the current incumbent (if there is one), re-applies presolve, and starts a new
	// search from a new root.
	// --> Repeat presolve when dealing with a new GOP
	// --> Repeat presolving when solving the corresponding MIP (further reductions based on binary vars)
	// NOTE. This parameter seems to considerably speed up the model for larger instances, since the 
	// presolve step essentially removes most of the redundant y_{i,s,t} constraints
	cplex.setParam(IloCplex::Param::Advance, 2);

	// Turn off the presolve reductions and set the CPLEX optimizer
	// to solve the worker LP with the dual simplex method.
	// We model the primal but solve the dual --> also needed for Farkas dual
	// Based on the example code of cplex, turning of reduce seems to be sufficient
	// IMPORTANT! The only reason to disable presolve is to get a Farkas certificate in 
	// the case of an infeasible subproblem (otherwise the solver might identify infeasibility 
	// of the problem without running simplex, and hence no dual infeasibility certificate can be returned). 
	// --> Disabling the presolve turns out to have a tremendous effect on computation time (many rows/columns are
	// redundant once the GOP set is fixed.
	// --> Whenever the model is infeasible, we instead resolve the model but with presolve disabled
	// cplex.setParam(IloCplex::Param::Preprocessing::Presolve, IloFalse);
	// cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 0);
	cplex.setParam(IloCplex::Param::RootAlgorithm, IloCplex::Dual);

	// Create variables x[i][j][s]
	// For simplicity, also dummy variables x[i][i][s] are created.
	// Those variables are fixed to 0 and do not partecipate to
	// the constraints.
	x = IloFloatVarArray3(env, nrTeams);
	xVars = IloNumVarArray(env);
	for (int i = 0; i < nrTeams; ++i) {
		x[i] = IloFloatVarArray2 (env, nrTeams);
		for (int j = 0; j < nrTeams; ++j) {
			// Add the binary bound via separate constraint for benders cuts
			// NOTE: x_ijs <= 1 con should be added as a separate con for the Benders cuts
			// However: is redundant since x_ijs is always <= 1 since otherwise we cannot schedule all games
			// Not necessarily true in a time-relaxed timetable where it can be beneficial to schedule a game twice
			x[i][j] = IloFloatVarArray (env, nrSlots, 0, IloInfinity);
			for (int s = 0; s < nrSlots; ++s) {
				if (i == j){
					x[i][j][s] = NULL;
				} else {
					std::string varName = "x_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(s);
					x[i][j][s].setName(varName.c_str());
					xVars.add(x[i][j][s]);
				}
			}	
		}	
	}

	// Pair separation contraint
	if(separation > 0){
		sepCons.clear();
		for (int i = 0; i < nrTeams; ++i) {
			for (int j = i+1; j < nrTeams; ++j) {
				for (int s = 0; s < nrSlots-separation; ++s) {
					// All non-constant terms
					IloNumExpr lhs(env);
					for (int k = 0; k <= separation; ++k) {
						lhs += x[i][j][s+k] + x[j][i][s+k];	
					}

					// Gen con
					std::string conName = "Sep_X_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(s);
					IloRange c(env, -IloInfinity, lhs, 1, conName.c_str());
					lhs.end();

					// Add con to the model
					sepCons[conName] = c;
					model.add(c);

					// All constant terms, including the vars from the master
					IloNumExpr rhs(masterEnv);
					rhs += 1;
					rhsMap[conName] = rhs;
				}
			}
		}
	}

	// Add constraints
	mirroredCons.clear();
	if(mirrored){
		for (int i = 0; i < nrTeams; ++i) {
			for (int j = 0; j < nrTeams; ++j) {
				if(i==j) { continue; }
				for (int s = 0; s < nrTeams-1; ++s) {
					// All non-constant terms
					IloNumExpr lhs(env);
					lhs += x[i][j][s];
					lhs -= x[j][i][s+nrTeams-1];

					// NOTE: we impose that x_ijs == x_jis --> since RHS of the constraints is 0, we know that this can have no impact
					// on the cut (rhs*dual --> constraint does not participate in the Benders cut)
					// Adding the lowerbound seems to considerably speed up the model: because presolve is disabled?
					std::string conName = "Symmetry_X_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(s);
					IloRange c(env, 0, lhs, 0, conName.c_str());
					lhs.end();

					// Add con to the model
					mirroredCons[conName] = c;
					model.add(c);

					// All constant terms, including the vars from the master
					IloNumExpr rhs(masterEnv);
					rhs += 0;
					rhsMap[conName] = rhs;
				}
			}
		}
	}

	// The HAP set is respected
	hapCons.clear();
	for(IloInt i=0; i < nrTeams; ++i){
		for(IloInt s=0; s < nrSlots; ++s){
			// All non-constant terms
			IloNumExpr lhs(env);
			for(int j=0; j < nrTeams; ++j){
				if (i!=j) {
					lhs -= x[i][j][s];
				}
			}

			// sum_{j\in T} x_{i,j,s} >= h_{i,s}
			std::string conName = "HAP_Home_" + std::to_string(i) + "_" + std::to_string(s);
			IloRange c(env, -IloInfinity, lhs, 0, conName.c_str());
			lhs.end();

			// Add con to the model
			hapCons[conName] = c;
			model.add(c);

			// All constant terms, including the vars from the master
			IloNumExpr rhs(masterEnv);
			rhs -= h[i][s];
			rhsMap[conName] = rhs;
		}	
	}

	for(IloInt i=0; i < nrTeams; ++i){
		for(IloInt s=0; s < nrSlots; ++s){
			// All non-constant terms
			IloNumExpr lhs(env);
			for(int j=0; j < nrTeams; ++j){
				if (i!=j) {
					lhs -= x[j][i][s];
				}
			}

			// sum_{j\in T} x_{j,i,s} >= 1-h_{i,s}
			std::string conName = "HAP_Away_" + std::to_string(i) + "_" + std::to_string(s);
			IloRange c(env, -IloInfinity, lhs, 0, conName.c_str());
			lhs.end();

			// Add con to the model
			hapCons[conName] = c;
			model.add(c);

			// All constant terms, including the vars from the master
			IloNumExpr rhs(masterEnv);
			rhs += h[i][s]-1;
			rhsMap[conName] = rhs;
		}	
	}

	/// Each team plays the required number of times against every other team
	assignmentCons.clear();
	for(IloInt i=0; i < nrTeams; ++i){
		for(IloInt j=0; j < nrTeams; ++j){
			if (i >= j) { continue; }

			// All non-constant terms
			IloNumExpr games(env);
			for(int s=0; s < nrSlots; ++s){
				games += x[i][j][s];
				games += x[j][i][s];
			}

			// sum_{s\in S} x_{i,j,s} <= 1
			std::string conName = "RequiredNoGames_" + std::to_string(i) + "_" + std::to_string(j);
			IloRange c(env, -IloInfinity, games, 1, conName.c_str());
			games.end();

			// Add con to the model
			assignmentCons[conName] = c;
			model.add(c);

			// All constant terms, including the vars from the master
			IloNumExpr rhs(masterEnv);
			rhs += 1;
			rhsMap[conName] = rhs;
		}
	}

	/// At most one game per round
	assignmentCons.clear();
	for(IloInt i=0; i < nrTeams; ++i){
		for(IloInt s=0; s < nrSlots; ++s){
			// All non-constant terms
			IloNumExpr games(env);
			for(IloInt j=0; j < nrTeams; ++j){
				if (i == j) { continue; }
				games += x[i][j][s] + x[j][i][s];
			}

			// sum_{j\in T} x_{i,j,s} <= 1 for all i,s
			std::string conName = "At_most_one_game_" + std::to_string(i) + "_" + std::to_string(s);
			IloRange c(env, -IloInfinity, games, 1, conName.c_str());
			games.end();

			// Add con to the model
			assignmentCons[conName] = c;
			model.add(c);

			// All constant terms, including the vars from the master
			IloNumExpr rhs(masterEnv);
			rhs += 1;
			rhsMap[conName] = rhs;
		}
	}

	std::cout << "Worker initialized" << std::endl;
}

Worker::~Worker(){
	// Free all memory in the rhsMap
	for(auto&  m : rhsMap) {
		// Remove the value which is an IloNumExpr
		m.second.end();
	}

	// Free all memory associated to env
	env.end();
}

/*
 * h: the h_{i,s} variables
 * hSol: the given HAP set
 * masterEnv: cplex environment of master problem
 * fractionalMaster: 1 if we generate a Benders cut based on a fractional master
 * timetable: the actual integer solution in case of integer feasibility
*/
std::vector<IloRange> Worker::separate(const IloBoolVarArray2& h, IloNumArray2& hSol, const IloEnv& masterEnv, const bool fractionalMaster, std::vector<std::vector<int>> &timetable){

	// All cuts to be returned
	std::vector<IloRange> cuts;

	// Get model
	IloModel model = cplex.getModel();

	/*****  BENDERS' CUTS *****/

	// Step 1. 
	// Update the rhs of the constraints of the subproblem

	// Set up IloCplex algorithm to solve the worker LP
	// See also comment above w.r.t. preprocessing
	// See also https://community.ibm.com/community/user/datascience/communities/community-home/digestviewer/viewthread?MessageKey=e282eb60-e47a-4286-a191-b2b0532b4431&CommunityKey=ab7de0fd-6f43-47a9-8261-33578a231bb7&tab=digestviewer#bme282eb60-e47a-4286-a191-b2b0532b4431
	// where Daniel Junglas advises this strategy
	cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 3);
	// Update the rhs of the gopCons
	for(IloInt i=0; i < nrTeams; ++i){
		for(IloInt s=0; s < nrSlots; ++s){
			std::string conName = "HAP_Home_" + std::to_string(i) + "_" + std::to_string(s);
			hapCons.at(conName).setUB(-hSol[i][s]);

			conName = "HAP_Away_" + std::to_string(i) + "_" + std::to_string(s);
			hapCons.at(conName).setUB(hSol[i][s]-1);
		}	
	}

	// Step 2.
	// Solve the worker LP
	
	std::chrono::steady_clock::time_point beginLP;
	std::chrono::steady_clock::time_point endLP;

       	beginLP = std::chrono::steady_clock::now();
	cplex.solve();

	// Get status of sub problem
	IloCplex::CplexStatus status = cplex.getCplexStatus();

	if (status == IloCplex::Status::Infeasible) {
		/**********************************************************
		*  Sub problem infeasible --> Bender's infeasiblity cut  *
		**********************************************************/
		// Need to resolve the model with presolve disabled.
		// See also comment above.
		cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 0);
		cplex.solve();

		// Use Farkas dual to get an extreme ray
		IloRangeArray constraints(env);
		IloNumArray coefficients(env);
		cplex.dualFarkas(constraints, coefficients);

		IloNumExpr cutLhs(masterEnv);
		// process all elements of the Farkas certificate
		for(int c=0; c < coefficients.getSize(); ++c){
			if (coefficients[c] != 0) {
				cutLhs += coefficients[c]*rhsMap.at(constraints[c].getName());
			}
		}

		IloRange r(masterEnv, -IloInfinity, cutLhs, 0);
		cuts.push_back(r);

		constraints.end();
		coefficients.end();

		// Stop timer for the LP problems that turned out to be infeasible
       		endLP = std::chrono::steady_clock::now();
		lck1.lock();
		cutsAdded1[0]++;
		timeTaken1[0] += std::chrono::duration_cast<std::chrono::milliseconds>(endLP - beginLP).count();
	std::cout << "Total number of Benders feasibility cuts added: " << cutsAdded1[0] << std::endl;
	std::cout << "Total number of Benders optimality cuts added: " << cutsAdded1[1] << std::endl;
	std::cout << "Total number of no-good feasiblity cuts added: " << cutsAdded1[2] << std::endl;	
	std::cout << "Total number of feasible solutions found: " << cutsAdded1[3] << std::endl;
	
	std::cout << "Total time spend to solve LP subproblems: " << timeTaken1[0] << std::endl;
	std::cout << "Total time spend to solve IP subproblems: " << timeTaken1[1] << std::endl;
		lck1.unlock();        
	} else if (!fractionalMaster){
		// Stop timer for the LP problems that turned out to be infeasible
       		endLP = std::chrono::steady_clock::now();
		lck1.lock();
		timeTaken1[0] += std::chrono::duration_cast<std::chrono::milliseconds>(endLP - beginLP).count();
		lck1.unlock();        

		// Additionally, we need to perform integer checks
		// Convert all the x-variables to binaries
		IloConversion c = IloConversion(env, xVars, ILOBOOL);
		model.add(c);

		// Since we no longer need the dual, we can again enable the presolve
		cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 3);

		// Resolve the model
		std::chrono::steady_clock::time_point beginIP;
		std::chrono::steady_clock::time_point endIP;

		beginIP = std::chrono::steady_clock::now();

		if (cplex.solve()) {
			// Stop timer for the IP problems that turned out to be feasible
			endIP = std::chrono::steady_clock::now();
			lck1.lock();
			timeTaken1[1] += std::chrono::duration_cast<std::chrono::milliseconds>(endIP - beginIP).count();
			cutsAdded1[3]++;	
	std::cout << "Total number of Benders feasibility cuts added: " << cutsAdded1[0] << std::endl;
	std::cout << "Total number of Benders optimality cuts added: " << cutsAdded1[1] << std::endl;
	std::cout << "Total number of no-good feasiblity cuts added: " << cutsAdded1[2] << std::endl;	
	std::cout << "Total number of feasible solutions found: " << cutsAdded1[3] << std::endl;
	
	std::cout << "Total time spend to solve LP subproblems: " << timeTaken1[0] << std::endl;
	std::cout << "Total time spend to solve IP subproblems: " << timeTaken1[1] << std::endl;
			lck1.unlock();        

			// We found a feasible integer solution
			const int nrTeams = IN->getNrTeams();
			for (int i = 0; i < nrTeams; ++i) {
				for (int j = 0; j < nrTeams; ++j) {
					if (i == j) {
						continue;
					}	
					for (int s = 0; s < nrSlots; ++s) {
						if (cplex.getValue(x[i][j][s]) > 0.99) {
							timetable[i][j] = s;
						}	
					}
				}
			}
		} else {
			// Stop timer for the IP problems that turned out to be infeasible
			endIP = std::chrono::steady_clock::now();
			lck1.lock();
			timeTaken1[1] += std::chrono::duration_cast<std::chrono::milliseconds>(endIP - beginIP).count();
			cutsAdded1[2]++;	
	std::cout << "Total number of Benders feasibility cuts added: " << cutsAdded1[0] << std::endl;
	std::cout << "Total number of Benders optimality cuts added: " << cutsAdded1[1] << std::endl;
	std::cout << "Total number of no-good feasiblity cuts added: " << cutsAdded1[2] << std::endl;	
	std::cout << "Total number of feasible solutions found: " << cutsAdded1[3] << std::endl;
	
	std::cout << "Total time spend to solve LP subproblems: " << timeTaken1[0] << std::endl;
	std::cout << "Total time spend to solve IP subproblems: " << timeTaken1[1] << std::endl;
			lck1.unlock();

			std::cout << "Not integer feasible..." << std::endl;
			// No solution was found --> add a no-good cut
			IloNumExpr cutLhs(masterEnv);
			{
				int i,s;
				for (i = 0; i < nrTeams; ++i) {
					for (s = 0; s < nrSlots; ++s) {
						if (hSol[i][s] > 0.999) {
							cutLhs += h[i][s];
						}
					}
				}

				// Number of H's is even during each time slot: all feasible HAPS
				// differ in at least 2 H's
				IloRange r(masterEnv, -IloInfinity, cutLhs, nrTeams*(nrTeams-1)-2);
				cuts.push_back(r);
			}
		}

		// Remove the integrality constraints
		model.remove(c);
		c.end();
	}

	return cuts;
}
