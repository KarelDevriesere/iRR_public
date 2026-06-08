#include "Meta.h"
#include "FO.h"
#include "GreedyMatching.h"

template <typename Move>
MetaBase<Move>::MetaBase(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, std::mt19937& g): 
          Moves(moves), Weights(weights), gen(g) {
            double sum = 0;
#ifdef PRINT
#if PRINT == 1
            cout << "------ Cumulative Weights ------" << endl;
#endif 
#endif
            for (const auto& [move, weight]: weights){
                sum += weight;
                WeightsCumul[sum] = move;
#ifdef PRINT
#if PRINT == 1
                cout << "Cumulative weight of " << Moves.at(WeightsCumul.at(sum)) << " = " << sum << endl;
#endif 
#endif
                
                NrImprov[move] = 0;
                NrChosen[move] = 0;
                NrImprovBestObj[move] = 0; 
                Reward[move] = 0;
                NrAccepted[move] = 0;
                SelectionProbabilityMAB[move] = 0;
                Improv[move]; // vector is default constructed
                ExecutionTimes[move];
            }
#ifdef PRINT
#if PRINT == 1
            cout << "-------------------------------" << endl;
#endif 
#endif
            assert(0.99 < sum && sum < 1.01);
            it = 0;
            it_accepted = 0;
            it_idle = 0;
            best_obj = INT_MAX;
            current_obj = INT_MAX;

            RandomDoubleNumber = make_unique<Randomizer<double>>(0,1,g);
        }

template <typename Move>
void MetaBase<Move>::Initialize(Solution& sol){
    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
    setStartTime(start_time);
    BestOrientation = vector<vector<HA>>(sol.getNrTeams(), vector<HA>(sol.getNrRounds()));
    BestTeamColorOpp = vector<vector<int>>(sol.getNrTeams(), vector<int>(sol.getNrRounds()));
    Reset();
    best_obj = sol.ComputeTotalCost();
    current_obj = best_obj;
    UpdateBestSolution(sol);
}

template <typename Move>
void MetaBase<Move>::UpdateBestSolution(Solution& sol){
            int j,h,a;
            for (int r = 0; r < sol.getNrRounds(); ++r){
                for (int i = 0; i < sol.getNrTeams(); ++i){
                    j = sol.TeamColorOpp[i][r];
                    if (i < j){
                        continue;
                    }
                    if (!sol.ViolationEligibleOpponents_allowed){
                        assert(sol.isEligible(i,j));
                    }
                    if (sol.Orientation[i][r] == HA::H){
                        assert(sol.Orientation[j][r] == HA::A);
                        h = i, a = j;
                    }
                    else{
                        assert(sol.Orientation[j][r] == HA::H);
                        assert(sol.Orientation[i][r] == HA::A);
                        h = j, a = i;
                    }
                    if (BestOrientation.empty()){
                        BestOrientation = vector<vector<HA>>(sol.getNrTeams(), vector<HA>(sol.getNrRounds()));
                        BestTeamColorOpp = vector<vector<int>>(sol.getNrTeams(), vector<int>(sol.getNrRounds()));
                    }
                    BestOrientation[h][r] = HA::H;
                    BestOrientation[a][r] = HA::A;
                    BestTeamColorOpp[h][r] = a;
                    BestTeamColorOpp[a][r] = h;
                }
            }
            // cout << "New best solution value = " << sol.ComputeTotalCost() << endl;
        }


template <typename Move>
void MetaBase<Move>::SaveBestSolution(Solution& sol){
            int h,a,j;
            // first, clear everything
            sol.clear();
            for (int r = 0; r < sol.getNrRounds(); ++r){
                for (int i = 0; i < sol.getNrTeams(); ++i){
                    j = BestTeamColorOpp[i][r];
                    if (i < j){
                        continue;
                    }
                    if (BestOrientation[i][r] == HA::H){
                        assert(BestOrientation[j][r] == HA::A);
                        h = i, a = j;
                    }
                    else{
                        assert(BestOrientation[i][r] == HA::A);
                        assert(BestOrientation[j][r] == HA::H);
                        h = j, a = i;
                    }
                    sol.SetColorMatch(h, a, r);
                    sol.Orientation[h][r] = HA::H; 
                    sol.Orientation[a][r] = HA::A;
                }
            }
            assert(sol.validate());
            // cout << "saved solution" << endl;
        }

template <typename Move>
        void MetaBase<Move>::SaveResultsMoves(std::string file_path_results_base, int instance, int seed){
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


template<typename Move>
void MetaBase<Move>::UpdateBest(Solution& sol, const int obj){
    if (obj < best_obj){
        best_obj = obj;
        UpdateBestSolution(sol);
        it_idle_best = 0;
        NewBestSolutionFound = true;
    }
    else{
        ++it_idle_best;
    }
}

template<typename Move>
bool LAHC<Move>::Update(Solution& sol, const int obj) {
           // code based on Burke (2017) "The late acceptance Hill-Climbing Heuristic"

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
            cout << "Found obj = " << obj << ", current_obj = " << this->current_obj << ", it_idle = " << this->it_idle << endl;
            */

            if (obj >= this->current_obj){
                /*
                if (PerturbeValue > 1){
                    cout << "obj = " << obj << endl;
                    cout << "current_obj = " << this->current_obj << endl;
                }
                */
                this->it_idle++;
            }
            else{
                // cout << "Reset idle to 0" << endl;
                this->it_idle = 0;
            }
            bool diff = false;
            if (obj <= this->current_obj || (!this->LocalOptimum && obj < HistoricValues.at(v))){

                if (obj != this->current_obj){
                    diff = true;
                }

                this->current_obj = obj; 
                this->it_accepted++;
                SolutionAccepted = true;
                this->NrAccepted.at(this->CurrentMove)++;
                // cout << "Accept solution" << endl;
            }
            if (obj < HistoricValues.at(v)){
                HistoricValues.at(v) = obj;
            }

            this->UpdateTimeStamps();
#ifdef PRINT
#if PRINT == 1
            if (SolutionAccepted && diff){
                this->print_solution();
            }
#endif
#endif
            ++this->it;
            return SolutionAccepted;

}

template<typename Move>
void LAHC<Move>::ResetList() {
    HistoryLength = 10;
    // it_same_HL = 0;
    this->it = 0;
    int lb = this->best_obj;
    int ub = PerturbeValue*this->best_obj;
    // cout << "reinitialize list with values between " << lb << " and " << ub << endl;
    InitializeHistoricValues(lb, ub, HistoryLength);
}

template<typename Move>
void LAHC<Move>::Reconfigure(Solution& sol) {

        if (DynamicHL && this->NewBestSolutionFound){
            PerturbeValue = PerturbeValue_INITIAL;
            ResetList();
            this->NewBestSolutionFound = false;
        }

        ++this->it;
        if (this->getTimeDiff() > this->TIME_LIMIT || (this->it_idle > this->MAX_IT)){
            if (this->getTimeDiff() > this->TIME_LIMIT){
#ifdef PRINT
#if PRINT == 1
                cout << "Time limit hit" << endl;
#endif
#endif
                this->STOP = true;
            }
            else{
#ifdef PRINT
#if PRINT == 1
                // cout << "max it_idle hit: " << this->it_idle << endl;
#endif
#endif
                if (!DynamicHL){
                    this->STOP = true;
                }
                else{
#ifdef PRINT
#if PRINT == 1
                    // cout << "-------------------------" << endl;
#endif  
#endif                  

                    if (HistoryLength*HistoryMultiplier < MAX_HL){
                        HistoryLength *= HistoryMultiplier; 
                        ++HistoryLength;
                        /*
                        I manually tried different things here, i.e. x2, additively add HL (+10 everytime), gradually increase HL (+10 till HL = 100, +100 till HL = 1000, +1000 til HL = 10000) on 3 instances, but they did not directly seem to result in better solutions
                        I similarly tried different things with PerturbeIncrease (e.g. trying the same HL but with different values for PerturbeIncrease, but this seemed too slow). 
                        */
#ifdef PRINT
#if PRINT == 1
                        // cout << "idle = " << this->it_idle << endl;
                        // cout << "current objective = " << this->current_obj << ", best obj = " << this->best_obj << endl;
                        cout << "New HistoryLength = " << HistoryLength << endl;
#endif
#endif
                    }
                    else{
                        // reinitialize
                        this->SaveBestSolution(sol);
                        this->current_obj = this->best_obj;
                        // std::uniform_real_distribution<>dist_pert = std::uniform_real_distribution<>(PerturbeValue_INITIAL,2.0); // guarantee some randomness
                        // PerturbeValue = dist_pert(this->gen);
                        PerturbeValue = PerturbeValue_INITIAL;
                        ResetList();
                    }
                    
#ifdef PRINT
#if PRINT == 1
                    // cout << "Previous HL = " << HistoryLength << endl;
                    // cout << "New HL = " << HistoryLength << endl;
                    // cout << "Previous PerturbValue = " << PerturbeValue << endl;
#endif
#endif

#ifdef PRINT
#if PRINT == 1
                    // cout << "New PerturbValue = " << PerturbeValue << endl;
                    // cout << "Best obj = " << this->best_obj << endl;
                    // cout << "Initialize list with " << PerturbeValue*this->best_obj << endl;
#endif
#endif                  
                    PerturbeValue += PerturbeIncrease;
                    // cout << "current_obj = " << this->current_obj << ", TotalCost = " << sol.ComputeTotalCost() << endl;
                    this->SaveBestSolution(sol);
                    this->current_obj = this->best_obj;
                    // cout << "current_obj = " << this->current_obj << ", TotalCost = " << sol.ComputeTotalCost() << endl;
                    int Lb = this->current_obj;
                    int Ub = PerturbeValue*this->current_obj;
                    InitializeHistoricValues(Lb, Ub, HistoryLength);
#ifdef PRINT
#if PRINT == 1
                    /*
                    cout << "---- Historic values ----" << endl;
                    for (auto& v: HistoricValues){
                        cout << v << ", ";
                    }
                    cout << endl;
                    cout << "-------------------------" << endl;
                    */

                    /*
                    cout << "Nr times Neighborhoods are selected" << endl;
                    for (const auto& [move, nr]: this->NrChosen){
                        cout << this->Moves.at(move) << " is chosen " << nr << " times, but accepted only " << this->NrAccepted.at(move) << " times" << endl;
                    }
                    cout << "-------------------------" << endl;
                    */
#endif
#endif
                    // cin.get();

                    this->it_idle = 0;
                    this->it = 0;

                    // this->LocalOptimum = true;
                    // cin.get();
                }
            }
        }
        // cin.get();
        return;
}

template<typename Move>
bool SA<Move>::Update(Solution& sol, const int obj){
            // cout << "prev_obj = " << current_obj << ", new obj = " << obj << endl;

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

            if (obj >= this->best_obj){
                ++this->it_idle;
            }
            else{
                T_last_improv = T;
                this->it_idle = 0;
            }

            bool SolutionAccepted = false;
            if (obj <= this->current_obj){
                this->current_obj = obj; 
                this->it_accepted++;
                SolutionAccepted = true;
            }
            else{
                double rnd = this->RandomDoubleNumber->Sample();
                // cout << "exp(-(" << obj << " - " << this->current_obj << ") / " << T << ") = " << exp(-(obj-this->current_obj)/T) << endl;
                if (rnd <= exp(-(obj-this->current_obj)/T)  && obj < sol.getCostTTPViolation()-100){ // Metropolis acceptance
                    // cout << "Accept solution with prob " << exp(-(obj-this->current_obj)/T) << ", prev_obj = " << this->current_obj << ", new obj = " << obj << endl;
                    this->current_obj = obj;
                    this->it_accepted++;
                    SolutionAccepted = true;
                }
                else{
                    // reverse move
                    // We only need to reverse the haps since the matchings are thrown away anyway
                    // cout << "Reject solution with prob " << exp(-(obj-this->current_obj)/T) << endl;
                    SolutionAccepted = false; // CUSTOMIZE WHAT HAPPENS AFTERWARDS
                }
            }
            if (SolutionAccepted){
#ifdef PRINT
#if PRINT == 1
                    this->print_solution();
#endif
#endif
            }

            this->UpdateTimeStamps();

            if (++this->it % I_temp == 0 || (this->it_accepted - I_accept >= 0)){ 
                T *= cooling_rate;
#ifdef PRINT
#if PRINT == 1
                cout << "New T = " << T << endl;
#endif
#endif
                this->it_accepted = 0;
                this->it = 0;
                // this->LocalOptimum = true;
            }
            return SolutionAccepted;
}

template<typename Move>
void SA<Move>::Reconfigure(Solution& sol){
    // Reheating: if too many iterations without finding a better objective, we may be stuck in a local optimum
    // Does not directly seem to work..
    if (!Reheat){
        return;
    }
    if (this->it_idle > 50*I_temp){
        T = T_last_improv;
        // cout << this->it_idle << " iterations without finding a better value, new T = " << T << endl;
        this->it_idle = 0;
        this->SaveBestSolution(sol);
        this->current_obj = this->best_obj;
    }
    return;
}


template<typename Move>
bool HC<Move>::Update(Solution& sol, const int obj){
            
            // cout << "prev_obj = " << this->current_obj << ", new obj = " << obj << endl;

            if (obj >= this->current_obj){
                this->it_idle++;
            }
            else{
                this->it_idle = 0;
                #ifdef PRINT
#if PRINT == 1
                    this->print_solution();
#endif
#endif
            }

            if (this->getTimeDiff() > this->TIME_LIMIT){
                cout << "Time limit of " << this->TIME_LIMIT << " hit" << endl;
                this->STOP = true;
            }
            else if (this->it_idle > this->MAX_IT){
                cout << "it_idle = " << this->it_idle << ", MAX_IT = " << this->MAX_IT << endl;
                this->STOP = true;
            }

            bool SolutionAccepted = false;
            if (obj <= this->current_obj){
                this->current_obj = obj; 
                this->it_accepted++;
                SolutionAccepted = true;
            }

            this->UpdateTimeStamps();

            return SolutionAccepted;
}

template<typename Move>
bool ILS<Move>::Update(Solution& sol, const int obj){

            bool SolutionAccepted = false;

            if (this->current_obj < obj && !this->PERTURB){
                ++this->it_idle;
            }
            if (this->current_obj >= obj || (this->PERTURB && obj < sol.getCostTTPViolation()-100)){
                this->current_obj = obj;
                SolutionAccepted = true;

                if (this->PERTURB){
                    it_accepted_perturbation++;
                }
#if PRINT == 1
                    this->print_solution();
#endif
            }

            if (this->getTimeDiff() > this->TIME_LIMIT){
                cout << "Time limit of " << this->TIME_LIMIT << " hit" << endl;
                this->STOP = true;
            }

            this->UpdateTimeStamps();

            return SolutionAccepted;
}

template<typename Move>
void ILS<Move>::Reconfigure(Solution& sol){
    if (this->it_idle < this->MAX_IT){
        return;
    }
    // cout << "--------------------" << endl;
    // cout << "Perturbe" << endl;
    // cout << "Current obj = " << this->current_obj << endl;
    // cout << "Best obj = " << this->best_obj << endl;
    this->PERTURB = true;
    it_accepted_perturbation = 0;
    this->SaveBestSolution(sol);
    this->current_obj = this->best_obj;
    int MaxPerturbations = this->gen()%IT_MAX_PERT+1;
    while (it_accepted_perturbation < MaxPerturbations){
        this->CurrentMove = SelectPerturbNB();
        this->executor->DoMove();
    }
    it_accepted_perturbation = 0;
    this->it_idle = 0;
    this->it_idle_best = 0;
    this->PERTURB = false;
    // cout << "New current obj = " << this->current_obj << " - " << sol.ComputeTotalCost() << endl;
    // cout << "--------------------" << endl;
    return;
}

template<typename Move>
bool VNS<Move>::Update(Solution& sol, const int obj){

            bool SolutionAccepted = false;

            if (this->current_obj <= obj){
                ++this->it_idle;
            }
            else{
                this->it_idle = 0;
            }
            if (this->current_obj >= obj || (this->PERTURB && obj < sol.getCostTTPViolation()-100)){
                this->current_obj = obj;
                SolutionAccepted = true;
                ++it_accepted_perturbation;
#if PRINT == 1
                    this->print_solution();
#endif
            }

            if (this->getTimeDiff() > this->TIME_LIMIT){
                cout << "Time limit of " << this->TIME_LIMIT << " hit" << endl;
                this->STOP = true;
            }

            this->UpdateTimeStamps();

            return SolutionAccepted;
}

template<typename Move>
void VNS<Move>::solve(Input& in, Solution& sol){
    // Works with a fixed hierarchy for the moves: based on current CPU time
    // The order is also theoretically motivated
    int k = 0;
    int k_last = OrderedPresentMoves.size();
    while (!this->STOP){
        this->CurrentMove = OrderedPresentMoves[k];
        // cout << "CurrentMove = " << this->Moves.at(this->CurrentMove) << endl;
        while (this->it_idle < this->MAX_IT){
            this->executor->DoMove();
            if (this->Moves.at(this->CurrentMove) == "MinCost_BM" && this->it_idle > 2*sol.getNrRounds()){
                break; // makes no sense to do more moves
            }
            else if (this->Moves.at(this->CurrentMove) == "NC" && this->it_idle > 1){
                break; // break early when LG does not have a negative cycle
            }
        }
        this->UpdateBest(sol, this->current_obj);
        if (this->current_obj < previous_best_obj){
            previous_best_obj = this->current_obj;
            assert(previous_best_obj == this->best_obj);
            k = 0;
        }
        else{
            ++k;
            if (k >= k_last){
                this->SaveBestSolution(sol); // return to best found solution
                this->current_obj = this->best_obj;
                cout << "best objective = " << sol.ComputeTotalCost() << " = " << this->best_obj << endl;
                std::shuffle(OrderedPresentMoves.begin(), OrderedPresentMoves.end(), this->gen);
                k = 0;
                while (this->Moves.at(OrderedPresentMoves[k]) == "MinCost_BM" || this->Moves.at(OrderedPresentMoves[k]) == "NC"){
                    ++k; // makes no sense to do perturbation with minimum cost moves
                }
                this->PERTURB = true;
                it_accepted_perturbation = 0;
                int MaxPerturbations = this->gen()%IT_MAX_PERT+1;
                cout << "Max nr of perturbations = " << MaxPerturbations << endl;
                while (it_accepted_perturbation < MaxPerturbations){
                    this->CurrentMove = OrderedPresentMoves[k];
                    this->executor->DoMove();
                }
                this->PERTURB = false;
                cout << "new objective = " << sol.ComputeTotalCost() << endl;
                k = 0;
            }
        }
        this->it_idle = 0;
    }
}


// Explicit instantiations
template class MetaBase<Move>;
template class LAHC<Move>;
template class SA<Move>;
template class HC<Move>;
template class ILS<Move>;
template class VNS<Move>;

template class MetaBase<FO_move>;
template class LAHC<FO_move>;
template class SA<FO_move>;
template class HC<FO_move>;
template class ILS<FO_move>;
template class VNS<FO_move>;
