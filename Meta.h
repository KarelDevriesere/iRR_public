#ifndef META_H  
#define META_H

#include <cmath> // for sqrt and log (log is ln)
#include <filesystem>
#include <iostream>
#include <fstream>
#include <map>
#include <array>
#include <chrono>
#include <random>
#include "assert.h"
#include "Solution.h"
#include "Input.h"
#include "Randomizer.h"

// TODO: Flat vectors, separate class for random numbers

// Every time we introduce a new parameter: in this struct as well as in the corresponding class!

struct ParameterValues{
    // Generic:
    double TIME_LIMIT = 7200; // TimeLimit
    long MaxIt = 10000000; // Maximum number of iterations (I only do time based)
    int MAX_IT = 100000; // Maximum number of iterations of local search before changing internal state of algorithm

    // HC:
    bool HC = false;

    // SA:
    bool SA = false;
    double T_begin = 83.0; // initial temperature
    double T_end = 1.0; // final temperature
    double cooling_rate = 0.99; 
    int I_temp = 20; // number of moves before cooling
    int I_accept = 4; // number of moves accepted before cooling
    bool IncludeReheating = false;

    // ILS:
    bool ILS = false;
    int IT_MAX_PERT = 1; // Maximum number of perturbations
    // TODO: weights of moves when doing perturbation?

    // VS: TimeBased, so no parameters (except for the order in which we do them)
    bool VNS = false;

    // LAHC:
    bool LAHC = false;
    int HistoryLength = 1; // HistoryLength
    bool HistoryLengthProvided = false;
    double PerturbeIncrease = 0.005; // Value with which we increase the percentage of the UB of how far the values in the list deviate from the best known UB
    double HistoryMultiplier = 1.5; // Value with which we increase the HL if we are stock in local optimum
    double PerturbeValue_INITIAL = 1.005; // Everytime we reset the history length, we fill the list with values in range [best_obj, best_obj*PerturbeValue_INITIAL]
};

enum class MetaHeuristic{HC, SA, ILS, LAHC, VNS};

class MoveExecutor { // Executor: here we do not need to know the logic behind the moves
public:
    virtual ~MoveExecutor() = default;
    virtual void DoMove() = 0; 
};

// By using template, it works with any type of Moves

// TODO TODO Whenever you use the template for a class, initialize at the bottom of Meta.cpp!
template<typename Move> 
class MetaBase{ // Everything that can be used for all metaheuristics
    protected:
        MoveExecutor* executor = nullptr;
        std::unique_ptr<Randomizer<double>>RandomDoubleNumber;
    public:
        std::mt19937& gen;
        std::unordered_map<Move, string>Moves;
        std::unordered_map<Move, double>Weights;
        std::map<double,Move>WeightsCumul; // map because this needs to be sorted!!!
        // values first and then the moves because then we can use built-in upper_bound
        Move CurrentMove; 
        double TIME_LIMIT; // ParameterValues 
        std::chrono::high_resolution_clock::time_point StartTime;
        std::chrono::high_resolution_clock::duration time_diff;
        int TimeTillBestSolution = 0;
        double current_obj;
        double best_obj;
        int it; // current iteration
        int it_accepted;
        int it_idle; // nr of iterations without improvement
        int it_idle_best = 0;
        bool STOP = false;
        bool CONVERGED = false;
        bool StartTimeSet = false;
        bool MAB = false; // Whether we update the selection probabilities with multi-armed bandit or not
        bool NewBestSolutionFound = false;
        bool PERTURB = false; // ParameterValues

        int LowerBound = -1;
        double LowerBoundGap;
        bool LocalOptimum = false; // whether we hit one or not

        long MaxIt; // ParameterValues
        int MAX_IT; // ParameterValues

        unordered_map<Move, int>NrChosen; // Nr of times a move was chosen in total
        unordered_map<Move, int>NrAccepted; // Nr of times a move was chosen in total
        // unordered_map<Move, int>NrChosenT; // Nr of times a move was chosen at current temperature
        unordered_map<Move, double>Reward; // total contribution of a move to improvement
        unordered_map<Move, int>NrImprov; // Nr of times a move found an improvement  
        unordered_map<Move, int>NrImprovBestObj; // Nr of times this move found a better best solution
        unordered_map<Move, vector<int>>Improv; // stores the improvements: to make boxplots
        unordered_map<Move, vector<std::chrono::microseconds>>ExecutionTimes;

        unordered_map<Move, double>SelectionProbabilityMAB;

        vector<vector<HA>>BestOrientation;
        vector<vector<int>>BestTeamColorOpp;

        int CurrentTimeStampIndex = 0;
        vector<int>TimeStamps;
        unordered_map<int,int>TimeStampSolution; // Best solution after certain time (in seconds)

        MetaBase(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g);
        // pass moves and weights to the object and initialize Moves and Weights with these

        // ----------------------------- Virtual ------------------------------ //

        virtual ~MetaBase() = default;

        // Function to update any parameters such as current_obj, best_obj, it_idle,.. etc after doing a move
        virtual bool Update(Solution& sol, const int obj) = 0; // CUSTOMIZE

        // After doing a move, we may need to change the state of the algorithm, i.e. reinitialize the history length, do a perturbation move..
        // This is handled by this function
        virtual void Reconfigure(Solution& sol) {}; // CUSTOMIZE (but optional, i.e. Hill Climbing: no reconfiguration)

        // ----------------------------- Virtual ------------------------------ //

        void SetExecutor(MoveExecutor* exec) {
            executor = exec;
        }

        // Every Metaheuristic does this: we do moves until a stopping criterion is met
        // If not overridden, any child class will use this

        virtual void solve(Input& in, Solution& sol){
            if (executor == nullptr) {
                throw std::runtime_error("Critical Error: MoveExecutor was never set before calling solve()!");
            }
            do {
                CurrentMove = SelectNB();
                executor->DoMove(); // do a move of CurrentMove
                Reconfigure(sol); // check internal state of the algorithm 
                if (current_obj < 0){
                    cout << "current_obj is negative!" << endl;
                    std::abort();
                }
            }
            while(!STOP);
            SaveBestSolution(sol);
            sol.validate();
        }

        void Initialize(Solution& sol);

        void UpdateBest(Solution& sol, const int obj); // Check if we need to update the best solution

        void UpdateBestSolution(Solution& sol);

        void SaveBestSolution(Solution& sol);

        void Reset(){
            it = 0;
            it_accepted = 0;
            it_idle = 0;
            it_idle_best = 0;
            best_obj = INT_MAX;
            current_obj = INT_MAX;
            STOP=false;
        }

        void print_solution(){
            if (current_obj < best_obj){
                std::cout << Moves.at(CurrentMove) << ": " << "\033[32m" << current_obj << "\033[0m" << std::endl;
            }
            /*
            else{
                std::cout << Moves.at(CurrentMove) << ": " << "\033[31m" << current_obj << "\033[0m" << std::endl;
            }
            */
        }

        void setStartTime(std::chrono::high_resolution_clock::time_point start_time){
            if (!StartTimeSet){
                StartTime = start_time;
                StartTimeSet = true;
            }
        }

        template <typename T = std::chrono::seconds>
        long long getTimeDiff() {
            time_diff = std::chrono::high_resolution_clock::now() - StartTime;
            return std::chrono::duration_cast<T>(time_diff).count();
        }

        array<std::chrono::microseconds,3>MinMaxMeanExecutionTime(Move move){
            auto Min = ExecutionTimes.at(move)[0];
            auto Max = ExecutionTimes.at(move)[0];
            std::chrono::microseconds Mean{0};
            for (auto dur: ExecutionTimes.at(move)){
                if (dur > Max){
                    Max = dur;
                }
                else if (dur < Min){
                    Min = dur;
                }
                Mean += dur;
            }
            Mean /= ExecutionTimes.at(move).size();
            return {Min, Max, Mean};
        }

        void AddLowerBound(const int LB){LowerBound = LB;};
        void AddLowerBoundStoppingCriterion(const double LowerBoundStoppingCriterion){
            if (LowerBoundStoppingCriterion < 1.0){
                cout << "LowerBoundStoppingCriterion should be bigger or equal to 1.0" << endl;
                cout << "Criterion is now set to 1.0" << endl;
                LowerBoundGap = 1.0;
            }
            else{
                LowerBoundGap = LowerBoundStoppingCriterion;
            }
        };

        void SaveResultsMoves(std::string file_path_results_base, int instance, int seed);

        void SetTimeStamps(vector<int>& TS){
            TimeStamps = TS;
        }

        void UpdateTimeStamps(){
            if (!TimeStamps.empty() && (int)getTimeDiff() > TimeStamps.at(CurrentTimeStampIndex)){
                TimeStampSolution[TimeStamps.at(CurrentTimeStampIndex)] = best_obj;
                if (CurrentTimeStampIndex+1 < TimeStamps.size()){
                    CurrentTimeStampIndex++;
                }
            }
        }

        void SaveSolutionsTimeStamps(std::ofstream& output_file){
            output_file << "Time,Value \n";
            for (auto&[TimeStamp, Solution]: TimeStampSolution){
                output_file << TimeStamp << "," << Solution << "\n";
            }
#ifdef PRINT
#if PRINT == 1
            cout << "Total time = " << (int)this->getTimeDiff() << endl;
#endif
#endif
            output_file << "Final, " << this->best_obj << "," << (int)this->getTimeDiff() << "\n" << endl;
        }

        // ------------------------------- MAB ----------------------------------- //

        double epsilon = 1.0; // default, select neighborhoods according to their preset weight (uniform)

        Move SelectNB(){
            Move BestMove;
            double rnd = RandomDoubleNumber->Sample();
            auto iterator = WeightsCumul.upper_bound(rnd);
            BestMove = iterator->second;
            return BestMove;
        }

        Move SelectNB_MAB(){ // Multi Armed Bandit
            Move BestMove;
            double BestValue = -1;
            // select the move with the best reward
            // cout << "------------------------" << endl;
            // cout << "Selection probabilities:" << endl;
            for (auto& [move, value]: SelectionProbabilityMAB){
                // cout << Moves.at(move) << ": " << value << endl;
                if (NrChosen.at(move) == 0){
                    BestValue = value;
                    BestMove = move;
                    break;
                }
                else if (value > BestValue){
                    BestValue = value;
                    BestMove = move;
                }
            }
            // cout << "------------------------" << endl;
            // cout << Moves.at(BestMove) << " selected" << endl;
            return BestMove;
        }

        void UpdateSelectionProbabilities(Move move){
            double exploration_term = 2*std::sqrt((double)std::log(it)/(double)NrChosen.at(move));
            SelectionProbabilityMAB.at(move) = Reward.at(move) / (double)NrChosen.at(move) + exploration_term;
            // cout << "NrChosen = " << NrChosen.at(move) << endl;
		    // cout << "Reward = " << Reward.at(move) << endl;
            //cout << "exploration_term = " << exploration_term << endl;
        }

        // ------------------------------- MAB ----------------------------------- //
};

// TODO TODO Whenever you use the template for a class, initialize at the bottom of Meta.cpp!
template<typename Move>
class LAHC: public MetaBase<Move>{ // Late Acceptancy Hill Climbing
    private:
        // Everything public for easy access
    public:

        int HistoryLength; // ParameterValues
        double HistoryMultiplier; // ParameterValues
        bool DynamicHL = false;
        double PerturbeValue_INITIAL; // ParameterValues
        double PerturbeValue; // MetaFactory
        double PerturbeIncrease; // ParameterValues

        static constexpr int MAX_HL = 100000; // CONST
        // after MAX_HL iterations, we shrink the list length again (at this point, it is more a random walk then anything else)
        array<int, MAX_HL>HistoricValues;

        LAHC(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){
        }

        void ResetList();

        void SetHistoryLength(const int l){
            HistoryLength = l;
        }

        void SetPerturbeIncrease(const double p){
            PerturbeIncrease = p;
        }

        void InitializeHistoricValues(const int Lb, const int Ub, const int HistoryLength){
            if (Lb != Ub){
                std::uniform_int_distribution<>dist_HL_value(Lb,Ub);
                for (int v = 0; v < HistoryLength; ++v){
                    HistoricValues[v] = dist_HL_value(this->gen);
                }
            }
            else{
                for (int v = 0; v < HistoryLength; ++v){
                    HistoricValues[v] = Lb;
                }
            }
        }

        void MakeHistoryLengthDynamic(){
            DynamicHL = true;
        }

        bool Update(Solution& sol, const int obj) ;
        void Reconfigure(Solution& sol)override;
};

// TODO TODO Whenever you use the template for a class, initialize at the bottom of Meta.cpp!
template<typename Move>
class SA: public MetaBase<Move>{ // Simulated Annealing
    private:
        // Everything public for easy access

    public:

        // default parameters described in paper Li et al:
        double T_begin; // initial temperature
        double T_end; // final temperature
        double cooling_rate; 
        int I_temp; // number of moves before cooling
        int I_accept; // number of moves accepted before cooling
        double T_last_improv; // last temperature where we found a better solution
        double T; // current temperature
        bool Reheat;

        SA(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){
            this->it_idle = 0; // for reheating
        }

        // pass moves and weights to the object and initialize Moves and Weights with these
        ~SA(){};

        void Reconfigure(Solution& sol) override;

        bool Update(Solution& sol, const int obj);
};

template<typename Move>
class HC: public MetaBase<Move>{ // Hill Climbing
    private:
        // Everything public for easy access
    public:

        HC(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){
        }

        bool Update(Solution& sol, const int obj);

};

template<typename Move>
class ILS: public MetaBase<Move>{ // Hill Climbing
    private:
        // Everything public for easy access
    public:

        std::map<double,Move>WeightsCumulPerturb;

        int it_idle_current = 0;
        
        int IT_MAX_PERT; // ParameterValues
        int it_accepted_perturbation = 0;

        ILS(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, const std::unordered_map<Move, double>& weights_perturb, std::mt19937& g): MetaBase<Move>(moves, weights, g){
                double sum = 0;
                for (const auto& [move, weight]: weights_perturb){
                    sum += weight;
                    WeightsCumulPerturb[sum] = move;

                    cout << "Cumulative PERTURB weight of " << this->Moves.at(WeightsCumulPerturb.at(sum)) << " = " << sum << endl;
                }
        }

        bool Update(Solution& sol, const int obj);

        void Reconfigure(Solution& sol)override;

        Move SelectPerturbNB(){
            double rnd = this->RandomDoubleNumber->Sample();
            auto iterator = WeightsCumulPerturb.upper_bound(rnd);
            return iterator->second;
        }

};

template<typename Move>
class VNS: public MetaBase<Move>{ // Variable Neighborhood Search
    private:
        const vector<string>OrderedMoves = {"MinCost_BM", "NC", "C", "PRS", "TS", "Random_BM", "iPTS_Random_PR", "Random_M_Random_PR"};
        vector<Move>OrderedPresentMoves;
        int previous_best_obj = INT_MAX;
    public:
        bool perturb = false;

        VNS(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){
            // insert moves in order
            int it = 0;
            for (string move_name1: OrderedMoves){
                for (auto& [move, weight]: this->Weights){
                    if (this->Moves.at(move) == move_name1){
                        OrderedPresentMoves.push_back(move);
                        cout << "add move " << move_name1 << " as the " << ++it << "'th neighborhood" << endl;
                        break;
                    }
                }
            }
        }

        bool Update(Solution& sol, const int obj);

        void solve(Input& in, Solution& sol)override;
};

template<typename Move> 
class MetaFactory{
    public:
        static std::unique_ptr<MetaBase<Move>> create(const MetaHeuristic M, const int obj, const ParameterValues& ParamV, const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, const std::unordered_map<Move, double>& weights_perturb, std::mt19937& g){
            std::unique_ptr<MetaBase<Move>> MetaH;
            if (M == MetaHeuristic::HC){
                MetaH = std::make_unique<HC<Move>>(moves, weights, g);
            }
            else if (M == MetaHeuristic::SA){
                auto sa_ptr = std::make_unique<SA<Move>>(moves, weights, g);
                sa_ptr->T_begin = ParamV.T_begin; // initial temperature
                sa_ptr->T_end = ParamV.T_end; // final temperature
                sa_ptr->cooling_rate = ParamV.cooling_rate; 
                sa_ptr->I_temp = ParamV.I_temp; // number of moves before cooling
                sa_ptr->I_accept = ParamV.I_accept;
                sa_ptr->T = sa_ptr->T_begin;
                sa_ptr->T_last_improv = sa_ptr->T_begin;
                sa_ptr->Reheat = ParamV.IncludeReheating;
                MetaH = std::move(sa_ptr);
            }
            else if (M == MetaHeuristic::ILS){
                auto ils_ptr = std::make_unique<ILS<Move>>(moves, weights, weights_perturb, g);
                ils_ptr->IT_MAX_PERT = ParamV.IT_MAX_PERT;
                MetaH = std::move(ils_ptr);
            }
            else if (M == MetaHeuristic::LAHC){
                auto lahc_ptr = std::make_unique<LAHC<Move>>(moves, weights, g);
                lahc_ptr->HistoryLength = ParamV.HistoryLength; 
                if (!ParamV.HistoryLengthProvided){
                    lahc_ptr->MakeHistoryLengthDynamic();
                }
                lahc_ptr->PerturbeIncrease = ParamV.PerturbeIncrease; 
                lahc_ptr->HistoryMultiplier = ParamV.HistoryMultiplier; 
                lahc_ptr->PerturbeValue_INITIAL = ParamV.PerturbeValue_INITIAL;
                lahc_ptr->InitializeHistoricValues(obj, obj, lahc_ptr->HistoryLength);
                MetaH = std::move(lahc_ptr);
            }
            else if (M == MetaHeuristic::VNS){
                auto vns_ptr = std::make_unique<VNS<Move>>(moves, weights, g);
                MetaH = std::move(vns_ptr);
            }
            else{
                throw std::invalid_argument("Unknown heuristic type");
            }
            MetaH->TIME_LIMIT = ParamV.TIME_LIMIT; // TimeLimit
            MetaH->MaxIt = ParamV.MaxIt; // Maximum number of iterations (I only do time based)
            MetaH->MAX_IT = ParamV.MAX_IT;
            return MetaH;
        }
};

#endif
