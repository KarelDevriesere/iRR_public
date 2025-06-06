#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <map>

// #include "Graph.h"
#include "Solution.h"

using namespace std;

enum class move_name{TS, PTS, RS, PRS, M, BM, C, NC}; 
const unordered_map<move_name, string>move_name_string = {{move_name::TS, "TS"}, {move_name::PTS, "PTS"}, {move_name::BM, "BM"},
         {move_name::RS, "RS"}, {move_name::PRS, "PRS"}, {move_name::M, "M"}, {move_name::C, "C"}, {move_name::NC, "NC"}};

class ILS
{
    private:
        bool include_travel;
        bool include_HAP;
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point current_time;
        bool stop = false; 
        bool pertube = false;
        bool optimal_obj_provided = false;
        bool Accept(int& cost); // acceptance criteria for a move

        int TIME_LIMIT;
        int MAX_NO_IMPROV = 10000;
        int it = 0; // current iteration
        int nr_no_improv = 0; // nr of times we have no improvement in the solution
        array<int, 4>TimesGaps = {-1, -1, -1, -1}; // [0] = 5%, [1] = 2%, [2] = 1%, [3] = 0.5%
        array<double, 4>Gaps = {0.05, 0.02, 0.01, 0.005}; // gaps

        // can be initialized in constructor:
        int optimal_obj; // If we know the optimal objective
        int previous_obj; // objective previous iteration
        int current_obj;
        int best_obj; // best objective
        double time_best_obj; // time it took to reach the best found objective
        
        array<move_name, 6>Moves = {move_name::TS, move_name::PTS, move_name::PRS, move_name::M, move_name::BM, move_name::C};
        // Cycle gives problems!!
        unordered_map<move_name, double>Weights = {{move_name::TS, 0.0}, {move_name::PTS, 0.0}, {move_name::PRS, 0.0}, {move_name::M, 1.0}, {move_name::BM, 0.0}, {move_name::C, 0.0}};
        unordered_map<move_name, double>WeightsCumul;
        unordered_map<move_name, int>NrChosen; // Nr of times a move was chosen in total
        unordered_map<move_name, int>NrChosenT; // Nr of times a move was chosen at current temperature
        unordered_map<move_name, double>RewardT; // total contribution of a move to improvement at current temperature
        unordered_map<move_name, int>NrImprov; // Nr of times a move found an improvement  
        unordered_map<move_name, vector<int>>Improv; // stores the improvements: to make boxplots

        void Move(Solution& sol);
        void repairHAPs(Solution& sol);
        void Perturbation(Solution& sol);
        // Moves:
        int SelectTS(Solution& sol);
        void SelectRS(Solution& sol);
        int SelectPTS(Solution& sol);
        int SelectPRS(Solution& sol);
        int SelectMatching(const int l, Solution& sol, const bool bipartite); 
        void SelectNegativeCycle(const int l, Solution& sol);
        int SelectBalancedCycle(const int l, Solution& sol);

        void Update();

        void print_solution();
        bool veto_haps(Solution& sol);

        // SA parameters
        double T;
        double T_0;
        double T_min;
        int MaxMoves; // max nr of moves per temperature
        int gamma_johnson;
        double alpha;
        int NrAcceptedMovesT;

        // RL parameters
        double MinWeight = 0.01;
        double lambda = 0.093;

    public:
        ILS(const int time_limit);
        ~ILS();
        void solve(Solution& sol);
        
        int getBestObj()const{return best_obj;}
        int getTimeBestObj()const{return time_best_obj;}
        array<double,4>getGaps()const{return Gaps;}
        int getTimeGap(const int g)const{return TimesGaps[g];}

        void setOptimalObj(const int obj){optimal_obj = obj; optimal_obj_provided = true;};

        void setParamatersSA(const double T_0, const double T_min, const int MaxMoves, const int gamma_johnson, const double alpha){
            T = T_0; this->T_0 = T_0; this->T_min = T_min; this->MaxMoves = MaxMoves; this->gamma_johnson = gamma_johnson; this->alpha = alpha; NrAcceptedMovesT = 0;
        }
        void setParametersRL(const double lambda){this->lambda = lambda;};
        void UpdateSelectionProbabilities();

        void OutputNB(ofstream& output);

        void MiaoAlgo(Input& in);
};