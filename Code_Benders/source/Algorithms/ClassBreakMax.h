/******************************************
*  CODE FOR PAPER JOURNAL OF SCHEDULING  *
******************************************/

#ifndef CLASSMASTER_H
#define CLASSMASTER_H

// Include Cplex
#include <ilcplex/ilocplex.h>
#include <ilcplex/cplex.h>
#include <ilconcert/ilothread.h>
#include <ilcp/cp.h>

// Algorithm template
#include "ClassAlgorithm.h"

// Include standard library
#include <algorithm>
#include <chrono>
#include <string>

// Macro to define a shortcut for the instance object pointer
#define IN Instance::get()

// Cplex structures for multi-index variables
typedef IloArray<IloIntVarArray> IloIntVarArray2;
typedef IloArray<IloBoolVarArray> IloBoolVarArray2;
typedef IloArray<IloBoolVarArray2> IloBoolVarArray3;
typedef IloArray<IloFloatVarArray> IloFloatVarArray2;
typedef IloArray<IloFloatVarArray2> IloFloatVarArray3;


// Master object which generates the gop set and handles backtracking
class BreakMax : public Algorithm {
private:
	
	// Timing duration of algorithm
	std::chrono::steady_clock::time_point begin;
	std::chrono::steady_clock::time_point end;

	// Master variables
	IloBoolVarArray2 h; 	// h[i][s] = 1 if team i has a home game on slot s, 0 else
	IloBoolVarArray2 bh; 	// bh[i][s] = 1 if team i has a home break on slot s, 0 else
	IloBoolVarArray2 ba; 	// ba[i][s] = 1 if team i has an away break on slot s, 0 else

	bool mirrored;

	// Master env, model, and cplex
	IloEnv masterEnv;
	IloModel masterModel;	
	IloCplex masterCplex;

public:
	// Default constructor
	BreakMax(Instance* i, const int timeLimit, const std::string solName, const int method);

	// Default destructor
	virtual ~BreakMax();

	// Construct the master model
	void createMasterILP();
	void createMasterILPPatterns(const int firstNewPattern, const int lastNewPattern);

	// Start two-step approach
	void solve();

	// Start two-step approach with pattern variables
	void solvePattern();

	// Break symmetry based on the observation that in a group of n teams, at
	// least ceil(n/2) teams must have the same home-away status
	void breakSymmetry(std::vector<std::vector<std::string>>&, int, int, int);

	// Dimensions of the problem
	const int nrTeams = IN->getNrTeams();
	const int nrSlots = IN->getNrSlots();

	// Availability matrix of teams: 0 can play home or away, 
	// 1 cannot play home but can play away
	// 2 cannot play away but can play home
	std::vector<std::vector<int>> availMatrix;

	// Max number of home games
	int maxHomeGames = -1;

	// Max number of away games
	int maxAwayGames = -1;

	// Minimal separation between mutual games
	int separation = -1;

	// True if the objective is break minimization, false if the objective is break maximization
	bool breakMin = false;

	// True if the teamIds are symmetric, false else
	bool symmetric = true;

	// Enumerate all possible patterns with CP
	void enumeratePatterns(const int requiredBreaks);
	// Possible patterns when we use pattern variables
	std::vector<std::vector<int>> patterns;
};

// Subproblem object which assigns games to time slots and tries to separate cuts
class Worker {
	public:
		// Default constructor
   		Worker(const IloBoolVarArray2& h,  const IloEnv masterEnv, const int threadNo, const bool mirrored, const int separation);
		// Default destructor
		~Worker();

		// Separate a Benders cut
		std::vector<IloRange> separate(const IloBoolVarArray2& h, IloNumArray2& hSol, const IloEnv& masterEnv, bool fractionalMaster, std::vector<std::vector<int>> &timetable);



	private:
		// Worker cplex and environment
		IloEnv env;
		IloCplex cplex;

		// Access of constraints to update right hand sides
                std::unordered_map<std::string, IloRange> mirroredCons;
                std::unordered_map<std::string, IloRange> sepCons;
                std::unordered_map<std::string, IloRange> relCons;
                std::unordered_map<std::string, IloRange> hapCons;
                std::unordered_map<std::string, IloRange> assignmentCons;

		// For each constraint: store right hand sides
		// Key is the same as in the maps above
                std::unordered_map<std::string, IloNumExpr> rhsMap;

		// x_ijs variables in a 3-dimensional structure to create model
		// and in an array-like structure as needed for certain cplex operations
		IloFloatVarArray3 x;
		IloNumVarArray xVars;


		// Features of the problem to be solved
		const int nrTeams;
		const int nrSlots;
		const int threadNo;
};

// IloCplex callback object to implement generic callbacks
class Callback : public IloCplex::Callback::Function {
	public:
		// Default constructor
		Callback(const IloBoolVarArray2 h, const IloInt numWorkers, const bool mirrored, const int separation);

		// Default destructor
		~Callback();

		// Overwrite the invoke function
		void invoke(const IloCplex::Callback::Context& context);
		double bestIntObj = INT_MAX;
	private:
		// Some features of the master problem and the problem instance
		const IloBoolVarArray2 h;
		const IloBoolVarArray2 bh;
		const IloBoolVarArray2 ba;
		const IloFloatVarArray3 x;
		const bool mirrored;
		const int separation;


		// Vector of workers that we can call to separate cuts
		std::vector<Worker*> workers;

		// Keep track of the total number of cuts added at each node
		std::unordered_map<int, int> nrCutsAddedNode;

};

std::vector<std::vector<std::vector<int>>> checkBlossom(std::vector<std::vector<std::vector<float>>> vars);

#endif /* CLASSMASTER_H */
