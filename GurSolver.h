#ifndef GURSOLVER_H  
#define GURSOLVER_H

#include "Input.h"
#include "Solution.h"
#include "gurobi_c++.h"
#include <cmath>
#include <chrono>
#include <random>
#include <array>
#include <memory>
#include <fstream>
#include <filesystem>

class GurSolver : public Input
{
    protected:
        int TimeTillBestSolutionGurSolverOuter;
        std::chrono::high_resolution_clock::time_point StartTimeGurSolver;

        // const GRBEnv env;
	    // GRBModel model = GRBModel(env);
        GRBEnv env;
        GRBModel model;
        GRBLinExpr Objective;
        vector<vector<vector<GRBVar>>> x;
        vector<vector<GRBVar>>v;
        vector<vector<GRBVar>>b;
        vector<vector<GRBVar>>y;
        vector<vector<HA>>Orientation;

        vector<vector<vector<GRBConstr>>>Constraints_fixed_variables; // per team per round

        GRBModel createModel(GRBEnv& env); // this way, we can set GRB_IntParam_OutputFlag to 0 before building the model

        // Callback to retrieve time till best solution:
        // *********** CALLBACK ***************** //
        class MyCallback : public GRBCallback { // nested class
            public:
                MyCallback(int& timeVar, std::chrono::high_resolution_clock::time_point& start, int& T, vector<int>& TS, unordered_map<int,int>& TSS)
                    : TimeTillBestSolutionGurSolver(timeVar), StartTimeGurSolver(start), CurrentTimeStampIndex(T), TimeStamps(TS), TimeStampSolution(TSS) {}

            protected:
                void callback() override {
                    try {
                        if (where == GRB_CB_MIPSOL) { // check whether a new MIP incumbent is found
                            auto time_diff = std::chrono::high_resolution_clock::now() - StartTimeGurSolver;
                            TimeTillBestSolutionGurSolver = std::chrono::duration_cast<std::chrono::seconds>(time_diff).count();

                            if (TimeTillBestSolutionGurSolver > TimeStamps.at(CurrentTimeStampIndex)){
                                TimeStampSolution[TimeStamps.at(CurrentTimeStampIndex)] = getDoubleInfo(GRB_CB_MIPSOL_OBJ); // retrieve objective value
                                std::cout << "Callback triggered at t = " << TimeTillBestSolutionGurSolver << " (Current TS = " << TimeStamps.at(CurrentTimeStampIndex) << ")\n";
                                if (CurrentTimeStampIndex+1 < TimeStamps.size()){
                                    CurrentTimeStampIndex++;
                                }
                            }
                        }
                    } catch (GRBException& e) {
                        std::cerr << "Gurobi callback error: " << e.getMessage() << std::endl;
                    }
                }

            private:
                int& TimeTillBestSolutionGurSolver;
                int& CurrentTimeStampIndex;
                vector<int>& TimeStamps;
                unordered_map<int,int>& TimeStampSolution;
                std::chrono::high_resolution_clock::time_point& StartTimeGurSolver;
            };
        std::unique_ptr<MyCallback> cb; // keep callback alive until GurSolver dies
        // *********** CALLBACK ***************** //

    public:

        int CurrentTimeStampIndexOuter = 0; // duplicate in SA.h
        vector<int>TimeStampsOuter; // duplicate in SA.h
        unordered_map<int,int>TimeStampSolutionOuter; // Best solution after certain time (in seconds) // duplicate in SA.h

        GurSolver(const Input& in);
        ~GurSolver();

        vector<bool>HapFixed; // whether the hap of team i is fixed
        
        void build_base(const bool HA, const bool relax_x);
        void build_league(const bool HA, const bool relax_x);
        int build_all(const bool HA, const bool relax_x);
        void build_HAP_constraints();
        void AddObj(const bool min_travel, const bool min_capacity_violations);
        void AddObjGeneralCosts(); // for cost minimization
        void AddObjMinBreaks(); // break minimization
        void setTimeLimit(const int time_limit);
        void setBoundCapacityViolations();
        int solve();
        double getMipGap();
        int getBestBound();
        void SaveSolution(Solution& sol);
        void WarmStart(Solution& sol);

        void FixHAP(Solution& sol);
        void BuildMiaoFormulation(const bool relax_x);
        void BuildPatternFormulation();
        void Fix_y_Patterns(const Solution& sol);
        void AssignHAPsToTeams(Solution& sol); // For Miao's algorithm
        void StoreHAPs(Solution& sol);
        void AddMiaoSymmetryConstraint();
        
        bool TrackTimeBestSolution = true;
        void addCallbackToTrackTime();
        int getTimeTillBestSolution()const{return TimeTillBestSolutionGurSolverOuter;};


        vector<vector<pair<int,int>>> EdgeColoring(int& C, vector<vector<bool>>& ForbiddenEdge, int& NrColorsUsed);

        int PerfectMatching(vector<vector<bool>>& GameForbidden);

        void SetTimeStamps(vector<int>& TS){ // duplicate in SA.h
            TimeStampsOuter = TS;
        }

        void SaveSolutionsTimeStamps(const string FilePath, const string config){ // duplicate in SA.h
            cout << "Save file as " << FilePath << endl;
            std::ofstream output_file(FilePath);
            output_file << config << "\n";
            for (auto&[TimeStamp, Solution]: TimeStampSolutionOuter){
                output_file << TimeStamp << "," << Solution << "\n";
            }
            output_file.close();
        }
};

#endif
