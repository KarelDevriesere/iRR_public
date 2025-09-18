#ifndef SA_H  
#define SA_H

#include <iostream>
#include <fstream>
#include <map>
#include <chrono>
#include <random>
#include "assert.h"
#include "Solution.h"
#include "Input.h"
#include "Algo.h"

template<typename Move> // this way, it works with any type of Moves
class SA{
    private:
        // Everything public for easy access

    public:
        std::unordered_map<Move, string>Moves;
        std::unordered_map<Move, double>Weights;
        std::map<double,Move>WeightsCumul; // map because this needs to be sorted!!!
        // values first and then the moves because then we can use built-in upper_bound

        Move CurrentMove; 

        // default parameters described in paper Miao:
        const double TIME_LIMIT = 60.0;
        const double T_begin = 83.0; // initial temperature
        const double T_end = 1.0; // final temperature
        const double cooling_rate = 0.99; 
        const int I_temp = 20; // number of moves before cooling
        const int I_accept = 4; // number of moves accepted before cooling
        std::chrono::high_resolution_clock::time_point StartTime;
        std::chrono::high_resolution_clock::duration time_diff;
        int TimeTillBestSolution = 0;

        double T;
        double current_obj;
        double best_obj;
        int it; // current iteration
        int it_accepted; // nr of moves accepted since last time we used this as cutoff
        bool STOP = false;

        double MinWeight = 0.01; // for dynamic updates
        double lambda = 0.093; // for dynamic updates

        std::random_device rd; // generate random numbers anywhere
        std::mt19937 gen;
        std::uniform_real_distribution<> dis;
        std::default_random_engine engine;

        unordered_map<Move, int>NrChosen; // Nr of times a move was chosen in total
        unordered_map<Move, int>NrAccepted; // Nr of times a move was chosen in total
        // unordered_map<Move, int>NrChosenT; // Nr of times a move was chosen at current temperature
        unordered_map<Move, double>Reward; // total contribution of a move to improvement
        unordered_map<Move, int>NrImprov; // Nr of times a move found an improvement  
        unordered_map<Move, int>NrImprovBestObj; // Nr of times this move found a better best solution
        unordered_map<Move, vector<int>>Improv; // stores the improvements: to make boxplots
        unordered_map<Move, vector<std::chrono::milliseconds>>ExecutionTimes;

        vector<vector<pair<int,int>>>BestSequenceMatches;

        SA(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, const int seed): 
          Moves(moves), Weights(weights), gen(seed), dis(0.0, 1.0), engine(seed) {
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
            T = T_begin;
            it = 0;
            it_accepted = 0;
            best_obj = INT_MAX;
            current_obj = INT_MAX;
        }
        // pass moves and weights to the object and initialize Moves and Weights with these
        ~SA(){};

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
        }

        double RandomNumber(){
            return dis(gen);
        }

        void print_solution(){
            if (current_obj <= best_obj){
                // best_obj = current_obj;  is set somewhere else
                std::cout << Moves.at(CurrentMove) << ": " << "\033[32m" << current_obj << "\033[0m" << std::endl;
            }
            else{
                std::cout << Moves.at(CurrentMove) << ": " << "\033[31m" << current_obj << "\033[0m" << std::endl;
            }
        }

        bool Update(Solution& sol, const double obj){
            // cout << "prev_obj = " << current_obj << ", new obj = " << obj << endl;
            NrAccepted.at(CurrentMove)++;
            assert(best_obj <= current_obj);
            if (obj < best_obj){
                best_obj = obj;
                // cout << "best obj = " << best_obj << endl;
                NrImprovBestObj.at(CurrentMove)++;
                UpdateBestSolution(sol); // We need to store the best solution somewhere
                TimeTillBestSolution = (int)getTimeDiff<>();
            }
            if (obj <= current_obj){
                // cout << "Accept solution" << endl;
                if (obj < current_obj){
                    Reward.at(CurrentMove) += (current_obj-obj);
                    NrImprov.at(CurrentMove)++;
                    Improv.at(CurrentMove).push_back(current_obj-obj);
                }
                current_obj = obj; 
                it_accepted++;
#ifdef PRINT
#if PRINT == 1
                print_solution();
#endif
#endif
            }
            else{
                double rnd = dis(gen);
                if (rnd <= exp(-(obj-current_obj)/T)){ // Metropolis acceptance
                    // cout << "Accept solution with prob " << exp(-(obj-current_obj)/T) << ", prev_obj = " << current_obj << ", new obj = " << obj << endl;
                    current_obj = obj;
                    it_accepted++;
#ifdef PRINT
#if PRINT == 1
                    print_solution();
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
            if (++it % I_temp == 0){ 
                T *= cooling_rate;
            }
            else if (it_accepted % I_accept == 0){
                T *= cooling_rate;
                it_accepted = 0;
            }
            if (getTimeDiff<>() > TIME_LIMIT || T < T_end){
                STOP = true;
            }
            return true;
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
                Mean = Mean / ExecutionTimes.at(move).size();
                Mean /= 1000;
                file2 << string_name << ",";
                file2 << Min.count() << ",";
                file2 << Max.count() << ",";
                file2 << Mean.count() << ",";
                file2 << instance << "," << seed << "\n";
                for (auto dur: ExecutionTimes.at(move)){ // store these as well to make boxplots later?
                    file2 << dur.count() << "\n";
                }
            }
            file2.close();
        }

        void setStartTime(std::chrono::high_resolution_clock::time_point start_time){
            StartTime = start_time;
        }

        template <typename T = std::chrono::seconds>
        long long getTimeDiff() {
            time_diff = std::chrono::high_resolution_clock::now() - StartTime;
            return std::chrono::duration_cast<T>(time_diff).count();
        }

        void UpdateSelectionProbabilities(){
            // Not used yet TODO
            double nrm = 0;
            for (const auto& [move, string_name]: Moves){
                cout << "reward / NrChosen = " << Reward.at(move) << " /= " << NrChosen.at(move) << endl;
                if (NrChosen.at(move) > 0){
                    Reward.at(move) /= NrChosen.at(move);
                }
                else{
                    assert(Reward.at(move) == 0);
                }
                nrm += Reward.at(move);
            }
            double sum = 0;
            for (const auto& [move, string_name]: Moves){
                cout << "weight = max((1.0-" << lambda << ")*" <<  Weights.at(move) << " + " << lambda << "*" << Reward.at(move)  << "/" << nrm << "), " << MinWeight << ")" << endl;
                if (nrm > 0){
                    Weights.at(move)  = std::max((1.0-lambda)*Weights.at(move)  + lambda*(Reward.at(move) /nrm), MinWeight);
                    sum += Weights.at(move);
                    WeightsCumul.at(move)  = sum;
                    cout << "Weight " << string_name << " = " << Weights.at(move)  << endl;
                }
                Reward.at(move)  = 0;
                NrChosen.at(move)  = 0;
            }
            cout << "sum = " << sum << endl;
            if (nrm > 0){
                assert(0.99 <= sum && sum <= 1.01);
            }
        }
};

#endif