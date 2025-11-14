#include "GurSolver.h"
#include "Input.h"
#include "Operators.h"
#include "Algo.h"
#include "Meta.h"

class MiaoAlgo: public SA<Move>{
    private:
        void MakeForbiddenEdges(const int l, const int r, Solution& sol);
        // see page 216 in Miao
        // The weighs change depending on the instance

        int team1; // if InterClubSwap, IntraClubSwap or RandomSwap, the haps of team1 and team2 are switched
        int team2;
        int hap_index1;
        int hap_index2;

    public:
        MiaoAlgo(const std::unordered_map<Move, string>& moves, const std::unordered_map<Move, double>& weights, const int NrRounds, std::mt19937& g);
        ~MiaoAlgo();

        int NrInfeasibleMatchings = 0;
        bool InitialOnly = false;
        bool InitialSolutionGiven = false;

        vector<int>Rounds;

        bool InterClubSwap(Solution& sol);
        bool IntraClubSwap(Solution& sol);
        bool RandomSwap(Solution& sol);
        bool ComplementInsertion(Solution& sol);
        bool SchedulePhase(Solution& sol);
        void ReAssignHAPs(Solution& sol);
        void Reset(Solution& sol); // delete all opponents
        void SaveBestSequenceMatches(Solution& sol);
        void ReverseMove(Solution& sol);

        // Custom functions already declared in SA:
        void solve(Input& in, Solution& sol) override;

        void SolveGivenSeqeuence(Input& in, Solution& sol);
};
