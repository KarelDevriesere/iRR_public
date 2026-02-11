#ifndef META_H  
#define META_H

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
#include "Algo.h"

// this way, it works with any type of Moves


// TODO TODO Whenever you use the template for a class, iniitialize at the bottom of Meta.cpp!
template<typename Move> 
class MetaBase{ // Everything that can be used for all metaheuristics
    private:
        // Everything public for easy access
    public:
        std::unordered_map<Move, string>Moves;
        std::unordered_map<Move, double>Weights;
        std::map<double,Move>WeightsCumul; // map because this needs to be sorted!!!
        // values first and then the moves because then we can use built-in upper_bound

        Move CurrentMove; 
        double TIME_LIMIT = 70.0; 
        std::chrono::high_resolution_clock::time_point StartTime;
        std::chrono::high_resolution_clock::duration time_diff;
        int TimeTillBestSolution = 0;
        double current_obj;
        double best_obj;
        int it; // current iteration
        int it_accepted;
        int it_idle; // nr of iterations without improvement
        bool STOP = false;
        bool StartTimeSet = false;

        int LowerBound = -1;
        double LowerBoundGap;

        std::mt19937& gen;
        std::uniform_real_distribution<> dist_real;
        std::uniform_int_distribution<> dist_int;

        unordered_map<Move, int>NrChosen; // Nr of times a move was chosen in total
        unordered_map<Move, int>NrAccepted; // Nr of times a move was chosen in total
        // unordered_map<Move, int>NrChosenT; // Nr of times a move was chosen at current temperature
        unordered_map<Move, double>Reward; // total contribution of a move to improvement
        unordered_map<Move, int>NrImprov; // Nr of times a move found an improvement  
        unordered_map<Move, int>NrImprovBestObj; // Nr of times this move found a better best solution
        unordered_map<Move, vector<int>>Improv; // stores the improvements: to make boxplots
        unordered_map<Move, vector<std::chrono::microseconds>>ExecutionTimes;

        vector<vector<pair<int,int>>>BestSequenceMatches;

        int CurrentTimeStampIndex = 0;
        vector<int>TimeStamps;
        unordered_map<int,int>TimeStampSolution; // Best solution after certain time (in seconds)

        MetaBase(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g);
        // pass moves and weights to the object and initialize Moves and Weights with these
        ~MetaBase(){};

        virtual void solve(Input& in, Solution& sol) = 0; // CUSTOMIZE

        void UpdateBestSolution(Solution& sol);

        void SaveBestSolution(Solution& sol);

        void UpdateValues(Solution& sol, const int obj);

        virtual bool Update(Solution& sol, const int obj) = 0; // CUSTOMIZE, acceptance function

        void Reset(){
            it = 0;
            it_accepted = 0;
            it_idle = 0;
            best_obj = INT_MAX;
            current_obj = INT_MAX;
            STOP=false;
        }

        double RandomDoubleNumber(const double a, const double b){
            dist_real = std::uniform_real_distribution<>(a,b);
            return dist_real(gen);
        }

        int RandomIntegerNumber(const int a, const int b){
            dist_int = std::uniform_int_distribution<>(a,b);
            return dist_int(gen);
        }

        void print_solution(){
            if (current_obj <= best_obj){
                std::cout << Moves.at(CurrentMove) << ": " << "\033[32m" << current_obj << "\033[0m" << std::endl;
            }
            /*
            else{
                std::cout << Moves.at(CurrentMove) << ": " << "\033[31m" << current_obj << "\033[0m" << std::endl;
            }
            */
        }

        void setTimeLimit_meta(const double TL){
            this->TIME_LIMIT = TL;
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
            cout << "Total time = " << (int)this->getTimeDiff() << endl;
            output_file << "Final, " << this->best_obj << "," << (int)this->getTimeDiff() << "\n" << endl;
        }
};

// TODO TODO Whenever you use the template for a class, iniitialize at the bottom of Meta.cpp!
template<typename Move>
class LAHC: public MetaBase<Move>{ // Late Acceptancy Hill Climbing
    private:
        // Everything public for easy access
    public:

        vector<int>HistoricValues;
        int HistoryLength = 1; // default: Hill Climbing

        long MAX_IT = 100000;
        bool DynamicHL = false;
        double PerturbeValue = 1;
        double PerturbeIncrease = 0.005;
        bool ResetSolutionAfterMove = false;

        LAHC(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){

        }

        void SetHistoryLength(const int l){
            HistoryLength = l;
        }

        void SetPerturbeIncrease(const double p){
            PerturbeIncrease = p;
        }

        void InitializeHistoricValues(const int Lb, const int Ub){
            HistoricValues = vector<int>(HistoryLength);
            std::uniform_int_distribution<>dist_HL_value = std::uniform_int_distribution<>(Lb,Ub);
            for (int v = 0; v < HistoryLength; ++v){
                if (Lb != Ub){
                    HistoricValues[v] = dist_HL_value(this->gen);
                }
                else{
                    HistoricValues[v] = Lb;
                }
            }
        }

        void SetMaxIt(const int limit){
            MAX_IT = limit;
        }

        void MakeHistoryLengthDynamic(){
            DynamicHL = true;
        }

        bool Update(Solution& sol, const int obj) ;

};

// TODO TODO Whenever you use the template for a class, iniitialize at the bottom of Meta.cpp!
template<typename Move>
class SA: public MetaBase<Move>{ // Simulated Annealing
    private:
        // Everything public for easy access

    public:

        // default parameters described in paper Miao:
        const double T_begin = 83.0; // initial temperature
        const double T_end = 1.0; // final temperature
        const double cooling_rate = 0.99; 
        int I_temp = 20; // number of moves before cooling
        int I_accept = 4; // number of moves accepted before cooling

        double T;

        double MinWeight = 0.01; // for dynamic updates
        double lambda = 0.093; // for dynamic updates

        vector<vector<pair<int,int>>>BestSequenceMatches;

        SA(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){
            T = T_begin;
        }

        // pass moves and weights to the object and initialize Moves and Weights with these
        ~SA(){};

        bool Update(Solution& sol, const int obj);

        void set_I_temp(const int nr){
            I_temp = nr;
        }

        void set_I_accept(const int nr){
            I_accept = nr;
        }
};

#endif
