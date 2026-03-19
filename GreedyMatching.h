#include "GurSolver.h"
#include "Input.h"
#include "Operators.h"
#include "Algo.h"
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

    public:
        GreedyMatching(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, const int NrRounds, std::mt19937& g);
        ~GreedyMatching();

        int NrInfeasibleMatchings = 0;
        bool InitialOnly = false;
        bool InitialSolutionGiven = false;
        int CurrentLeague = 0;

        int NrSuccesfullMatchings = 0;

        vector<int>Rounds;

        vector<pair<int,int>> MWPBM(const int r, std::mt19937& gen, const int l, Solution& sol);

        bool HomeAwaySwap(Solution& sol); // our own neighborhood
        bool InterClubSwap(Solution& sol);
        bool IntraClubSwap(Solution& sol);
        bool RandomSwap(Solution& sol);
        bool ComplementInsertion(Solution& sol);
        bool SchedulePhase(Solution& sol);
        void ReAssignHAPs(Solution& sol);
        void Reset(Solution& sol); // delete all opponents
        void ReverseMove(Solution& sol);
        void SetAllOpponents(Solution& sol);
        void SetOpponentsCurrentLeague(Solution& sol);

        // Custom functions already declared in SA:
        void solve(Input& in, Solution& sol) override;

        void SolveGivenSeqeuence(Input& in, Solution& sol);

        
};
