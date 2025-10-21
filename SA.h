#ifndef SA_H  
#define SA_H

#include <filesystem>
#include <iostream>
#include <fstream>
#include <map>
#include <chrono>
#include <random>
#include "assert.h"
#include "Solution.h"
#include "Input.h"
#include "Algo.h"

// this way, it works with any type of Moves

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

        MetaBase(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g): 
          Moves(moves), Weights(weights), gen(g) {
            double sum = 0;
            for (const auto& [move, string_name]: Moves){
                sum += Weights.at(move);
                if (string_name != "Initial"){
                    WeightsCumul[sum] = move;
                }
                NrImprov[move] = 0;
                NrChosen[move] = 0;
                NrImprovBestObj[move] = 0; 
                Reward[move] = 0;
                NrAccepted[move] = 0;
                Improv[move]; // vector is default constructed
                ExecutionTimes[move];
            }
            assert(0.99 < sum && sum < 1.01);
            it = 0;
            it_accepted = 0;
            it_idle = 0;
            best_obj = INT_MAX;
            current_obj = INT_MAX;
        }
        // pass moves and weights to the object and initialize Moves and Weights with these
        ~MetaBase(){};

        virtual void solve(Input& in, Solution& sol) = 0; // CUSTOMIZE

        void UpdateBestSolution(Solution& sol){
            if (BestSequenceMatches.empty()){
                BestSequenceMatches = vector<vector<pair<int,int>>>(sol.getNrRounds(), vector<pair<int,int>>(sol.getNrTeams()/2));
            }
            int m, j, h, a;
            for (int r = 0; r < sol.getNrRounds(); ++r){
                vector<bool>TeamSeen(sol.getNrTeams(), false);
                m = 0;
                for (int i = 0; i < sol.getNrTeams(); ++i){
                    if (TeamSeen[i]){
                        continue;
                    }
                    j = sol.TeamColorOpp[i][r];
                    assert(sol.isEligible(i,j));
                    if (sol.Orientation[i][r] == HA::H){
                        assert(sol.Orientation[j][r] == HA::A);
                        h = i, a = j;
                    }
                    else{
                        assert(sol.Orientation[j][r] == HA::H);
                        assert(sol.Orientation[i][r] == HA::A);
                        h = j, a = i;
                    }
                    TeamSeen[h] = true;
                    TeamSeen[a] = true;
                    BestSequenceMatches[r][m++] = {h,a};
                }
            }
        }

        void SaveBestSolution(Solution& sol){
            int h,a;
            if (BestSequenceMatches.empty()){
                cout << "Failed to find a solution, abort.." << endl;
                std::exit(0);
            }
            // first, clear everything
            sol.clear();
            for (int r = 0; r < sol.getNrRounds(); ++r){
                assert(BestSequenceMatches[r].size() == sol.getNrTeams()/2);
                for (auto& pair: BestSequenceMatches[r]){
                    h = pair.first, a = pair.second;
                    assert(sol.isEligible(h,a));
                    SetValueCircleMethod(h, a, r, sol);
                    sol.Orientation[h][r] = HA::H; // the orientation in this solution is also different!
                    sol.Orientation[a][r] = HA::A;
                }
            }
            assert(sol.validate());
        }

        void UpdateValues(Solution& sol, const int obj){
            NrAccepted.at(CurrentMove)++; // this is about whether the move is accepted, not the solution generated by the move
            assert(best_obj <= current_obj);
            if (obj < best_obj){
                best_obj = obj;
                cout << "NEW BEST SOLUTION = " << best_obj << endl;
                NrImprovBestObj.at(CurrentMove)++;
                UpdateBestSolution(sol); // We need to store the best solution somewhere
                TimeTillBestSolution = (int)getTimeDiff();
            }
            if (obj <= current_obj){
                // cout << "Accept solution" << endl;
                if (obj < current_obj){
                    Reward.at(CurrentMove) += (current_obj-obj);
                    NrImprov.at(CurrentMove)++;
                    Improv.at(CurrentMove).push_back(current_obj-obj);
                }
#ifdef PRINT
#if PRINT == 1
                print_solution();
#endif
#endif
            }
        }

        virtual bool Update(Solution& sol, const int obj) = 0; // CUSTOMIZE, acceptance function

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
            else{
                std::cout << Moves.at(CurrentMove) << ": " << "\033[31m" << current_obj << "\033[0m" << std::endl;
            }
        }

        void setTimeLimit_meta(const double TL){
            this->TIME_LIMIT = TL;
        }

        void setStartTime(std::chrono::high_resolution_clock::time_point start_time){
            StartTime = start_time;
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

        void SaveResultsMoves(std::string file_path_results_base, int instance, int seed){
            std::string file_path_results_performance = file_path_results_base + std::string(PATHSEP) + "Performance" + std::string(PATHSEP) + to_string(instance) + "_" + to_string(seed) + ".txt";
            std::ofstream file(file_path_results_performance);
            file << "Move, NrChosen, NrAccepted, % Times improvement found, Average improvement found, % Times contribution to best objective, Instance, Seed\n";
            // cout << "Move, NrChosen, NrAccepted, % Improvement found, Average improvement found, % contribution to best objective" << endl;
            for (const auto& [move, string_name]: Moves){
                if (string_name == "Initial"){
                    continue;
                }
                // cout << string_name << ", " << NrChosen.at(move) << ", " << NrAccepted.at(move) << ", " << (double) NrImprov.at(move) / (double) NrChosen.at(move) << ", " << (double) Reward.at(move) / (double) NrChosen.at(move) << ", " << (double) NrImprovBestObj.at(move) / (double) NrChosen.at(move)  << endl;
                file << string_name << ",";
                file << NrChosen.at(move) << ",";
                file << NrAccepted.at(move) << ",";
                file << (double) NrImprov.at(move) / (double) NrChosen.at(move) << ",";
                file << (double) Reward.at(move) / (double) NrImprov.at(move) << ",";
                file << (double) NrImprovBestObj.at(move) / (double) NrChosen.at(move) << ",";
                file << instance << "," << seed << "\n";
            }
            file.close();
   
            std::string file_path_results_time= file_path_results_base + std::string(PATHSEP) + "ExecutionTime" + std::string(PATHSEP) + to_string(instance) + "_" + to_string(seed) + ".txt";
            std::ofstream file2(file_path_results_time);
            file2 << "Move,Min,Max,Mean,Instance,Seed\n";
            // cout << "Move, NrChosen, NrAccepted, % Improvement found, Average improvement found, % contribution to best objective" << endl;
            for (const auto& [move, string_name]: Moves){
                if (string_name == "Initial" || ExecutionTimes.at(move).empty()){
                    continue;
                }
                array<std::chrono::microseconds,3>MinMaxMean = MinMaxMeanExecutionTime(move);
                file2 << MinMaxMean[0].count() << ",";
                file2 << MinMaxMean[1].count() << ",";
                file2 << MinMaxMean[2].count() << ",";
                file2 << instance << "," << seed << "\n";
                /*
                for (auto dur: ExecutionTimes.at(move)){ // store these as well to make boxplots later?
                    file2 << dur.count() << "\n";
                }
                */
            }
            file2.close();
        }
};

template<typename Move>
class LAHC: public MetaBase<Move>{ // Late Acceptancy Hill Climbing
    private:
        // Everything public for easy access
    public:

        vector<int>HistoricValues;
        int HistoryLength = 1; // default: Hill Climbing
        int CurrentTimeStampIndex = 0;
        vector<int>TimeStamps;
        unordered_map<int,int>TimeStampSolution; // Best solution after certain time (in seconds)

        LAHC(const std::unordered_map<Move, string>& moves, // moves, weights and in are defined in main
           const std::unordered_map<Move, double>& weights, std::mt19937& g): MetaBase<Move>(moves, weights, g){

        }

        void SetTimeStamps(vector<int>& TS){
            TimeStamps = TS;
        }

        void SetHistoryLength(const int l){
            HistoryLength = l;
        }

        void InitializeHistoricValues(const int obj){
            HistoricValues = vector<int>(HistoryLength, obj);
        }

        void SaveSolutionsTimeStamps(const string FilePath, const string config){
            cout << "Save file as " << FilePath << endl;
            std::ofstream output_file(FilePath);
            output_file << config << "\n";
            output_file << "Name,NrChosen,NrAccepted,NrImprove,AvgReward,MinT,MaxT,AvgT \n";
            for (const auto& [move, string_name]: this->Moves){
                if (string_name == "Initial"){
                    continue;
                }
                output_file << string_name << ",";
                output_file << this->NrChosen.at(move) << ",";
                output_file << this->NrAccepted.at(move) << ",";
                output_file << (double) this->NrImprov.at(move) << ",";
                output_file << (double) this->Reward.at(move) / (double) this->NrImprov.at(move) << ",";
                array<std::chrono::microseconds,3>MinMaxMean = this->MinMaxMeanExecutionTime(move);
                output_file << MinMaxMean[0].count() << ",";
                output_file << MinMaxMean[1].count() << ",";
                output_file << MinMaxMean[2].count() << "\n";
            }
            for (auto&[TimeStamp, Solution]: TimeStampSolution){
                output_file << TimeStamp << "," << Solution << "\n";
            }
            output_file.close();
        }

        bool Update(Solution& sol, const int obj) override {
            // code based on Burke (2017) "The late acceptance Hill-Climbing Heuristic"
            this->UpdateValues(sol, obj);
            bool SolutionAccepted = false;
            int v = this->it % HistoryLength;
            /*
            cout << "L = ";
            for (int w = 0; w < this->HistoricValues.size(); ++w){
                if (w != v){
                    cout << w << ") " << this->HistoricValues.at(w) << " ";
                }
                else{
                    cout << w << ")** " << this->HistoricValues.at(w) << " ";
                }
            }
            cout << endl;
            */
            // cout << "Found obj = " << obj << ", current_obj = " << this->current_obj << ", it_idle = " << this->it_idle << endl;
            if (obj >= this->current_obj){
                this->it_idle++;
            }
            else{
                this->it_idle = 0;
            }
            if (obj <= this->current_obj || obj < HistoricValues.at(v)){
                this->current_obj = obj; 
                this->it_accepted++;
                SolutionAccepted = true;
                // cout << "Accept solution" << endl;
            }
            if (obj < HistoricValues.at(v)){
                HistoricValues.at(v) = obj;
            }
            ++this->it;

            if ((int)this->getTimeDiff() > TimeStamps.at(CurrentTimeStampIndex)){
                TimeStampSolution[TimeStamps.at(CurrentTimeStampIndex)] = this->best_obj;
                if (CurrentTimeStampIndex+1 < TimeStamps.size()){
                    CurrentTimeStampIndex++;
                }
            }

            if (this->getTimeDiff() > this->TIME_LIMIT /*|| this->it_idle > this->it*0.02*/){
                this->STOP = true;
                if (this->getTimeDiff() > this->TIME_LIMIT){
                    cout << "Time limit hit" << endl;
                }
                else{
                    cout << "max it_idle hit" << endl;
                }
            }

            // cin.get();
            return SolutionAccepted;
        }

};

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

        bool Update(Solution& sol, const int obj) override {
            // cout << "prev_obj = " << current_obj << ", new obj = " << obj << endl;
            this->UpdateValues(sol, obj);
            bool SolutionAccepted = false;
            if (obj <= this->current_obj){
                this->current_obj = obj; 
                this->it_accepted++;
                SolutionAccepted = true;
            }
            else{
                this->dist_real = std::uniform_real_distribution<>(0.0, 1.0);
                double rnd = this->dist_real(this->gen);
                if (rnd <= exp(-(obj-this->current_obj)/T)){ // Metropolis acceptance
                    // cout << "Accept solution with prob " << exp(-(obj-current_obj)/T) << ", prev_obj = " << current_obj << ", new obj = " << obj << endl;
                    this->current_obj = obj;
                    this->it_accepted++;
                    SolutionAccepted = true;
#ifdef PRINT
#if PRINT == 1
                    this->print_solution();
#endif
#endif
                }
                else{
                    // reverse move
                    // We only need to reverse the haps since the matchings are thrown away anyway
                    // cout << "Reject solution with prob " << exp(-(obj-current_obj)/T) << endl;
                    return false; // CUSTOMIZE WHAT HAPPENS AFTERWARDS
                }
            }
            if (++this->it % I_temp == 0 || this->it_accepted % I_accept == 0){ 
                T *= cooling_rate;
                cout << "New T = " << T << endl;
                this->it_accepted = 0;
                this->it = 0;
            }
            if (this->getTimeDiff() > this->TIME_LIMIT || T < T_end){
                this->STOP = true;
                if (this->getTimeDiff() > this->TIME_LIMIT){
                    cout << "Time limit hit" << endl;
                }
                else{
                    cout << "Final temperature hit after " << this->getTimeDiff() << " seconds" << endl;
                }
                // cin.get();
            }
            return SolutionAccepted;
        }

        void set_I_temp(const int nr){
            I_temp = nr;
        }

        void set_I_accept(const int nr){
            I_accept = nr;
        }
};

#endif