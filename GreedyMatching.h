#include "GurSolver.h"
#include "Input.h"
#include "Operators.h"
#include "Meta.h"

class GreedyMatching: public HC<Move>{
    private:
        void MakeForbiddenEdges(const int l, const int r, Solution& sol);
        int team1; // if InterClubSwap, IntraClubSwap or RandomSwap, the haps of team1 and team2 are switched
        int team2;
        int hap_index1;
        int hap_index2;

        int round1; // HomeAwaySwap
        int round2; 

        vector<vector<int>>Opponents; // Opponents[i][r] = j means that j is the opponent of j in round r

        int RandomIntegerNumber(const int a, const int b){ // TODO
            std::uniform_int_distribution<>dist = std::uniform_int_distribution<>(a,b);
            return dist(gen);
        }

    protected:
        Solution& sol;

    public:
        GreedyMatching(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, const int NrRounds, std::mt19937& g, Solution& current_sol);
        ~GreedyMatching();

        int NrInfeasibleMatchings = 0;
        bool InitialOnly = false;
        bool InitialSolutionGiven = false;
        int CurrentLeague = 0;

        int NrSuccesfullMatchings = 0;

        vector<int>Rounds;

        vector<pair<int,int>> MWPBM(const int r, const int l);

        bool HomeAwaySwap(); // our own neighborhood
        bool InterClubSwap();
        bool IntraClubSwap();
        bool RandomSwap();
        bool ComplementInsertion();
        bool SchedulePhase();
        void Reset(); // delete all opponents
        void ReverseMove();
        void SetAllOpponents();
        void SetOpponentsCurrentLeague();
        void solve(Input& in, Solution& sol) override;
        void DoMove();
};
